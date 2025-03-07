import os
import random
import subprocess
import magic
import argparse
import re

# cmake will configure this part
compiler = "@CMAKE_C_COMPILER@"
source_dir = "@CMAKE_SOURCE_DIR@"
build_dir = "@CMAKE_BINARY_DIR@"

shared_lib_list = None


def get_ld_library_paths():
    """Extract system library search paths from `ld --verbose` and include `LD_LIBRARY_PATH`."""
    search_dirs = []

    # Get paths from `LD_LIBRARY_PATH`
    ld_library_path = os.environ.get("LD_LIBRARY_PATH", "")
    if ld_library_path:
        search_dirs.extend(ld_library_path.split(":"))  # Split by `:` as multiple paths can be set

    try:
        # Run `ld --verbose` to get default search paths
        result = subprocess.run(["ld", "--verbose"], capture_output=True, text=True, check=True)

        # Extract paths from SEARCH_DIR("path") entries using regex
        system_dirs = re.findall(r'SEARCH_DIR\("([^"]+)"\)', result.stdout)
        search_dirs.extend([e[1:] for e in system_dirs])  # remove = from output

    except subprocess.CalledProcessError as e:
        print("Error running `ld --verbose`:", e)

    # Remove duplicates and return paths
    return sorted(set(search_dirs))


def discover_shared_libs():
    libs = []
    for d in get_ld_library_paths():
        try:
            for f in os.listdir(d):
                file = os.path.join(d, f)
                if file.endswith(".so") and magic.from_file(file, mime=True) == "application/x-sharedlib":
                    libs.append(file)
        except FileNotFoundError as e:
            pass  # skip non-existent ones
    return libs


def get_shared_libs():
    global shared_lib_list
    if shared_lib_list is None:
        shared_lib_list = discover_shared_libs()
    return shared_lib_list


# Get functions from a shared library using nm
def get_library_functions(lib_path):
    try:
        result = subprocess.run(["nm", "-D", lib_path], capture_output=True, text=True)
        funcs = []
        for line in result.stdout.splitlines():
            parts = line.split()
            if len(parts) >= 3 and parts[1] in ("T"):  # "T" = exported function, "W" = weak symbol
                funcs.append(parts[2])  # Function name
        return funcs
    except:
        return []


# Select random libraries and functions from them
def select_libs_and_funcs(num_libs=3, num_funcs_per_lib=2):
    libs = get_shared_libs()
    selected_libs = random.sample(libs, min(num_libs, len(libs)))

    lib_func_map = {}
    for lib in selected_libs:
        funcs = get_library_functions(lib)
        if funcs:
            lib_func_map[lib] = random.sample(funcs, min(num_funcs_per_lib, len(funcs)))

    return lib_func_map


def get_lib_link_flag(lib):
    lib_name = os.path.basename(lib)
    return "-l" + lib_name[3:-3]


# Generate test C program
def generate_test_program(compilation_tries=10,
                          num_libs=3, num_funcs_per_lib=2,
                          output="test_code",
                          use_dlopen=False, use_weak_symbols=False,
                          hide_output=False):
    lib_func_map = select_libs_and_funcs(num_libs, num_funcs_per_lib)

    if compilation_tries == 0:
        if not hide_output:
            print("Failure: Reached maximum compilation attempts")
        return

    code = """
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifdef USE_TRAITS
#include "program_traits.h"
#include "markers.h"

marker(evaluation_trait)
#endif
//forward declaration
void call_libraries();
void dlopen_libraries();

int main(int argc, char **argv) {
#ifdef USE_TRAITS
    struct trait_options trait_options;
    memset(&trait_options,0,sizeof(struct trait_options));
    trait_options.check_for_dlopen = true;
    trait_options.check_for_mprotect = true;
    trait_options.name = "evaluation_trait";
    trait_options.num_symbols_require_trait = 1;
    trait_options.symbols_require_trait = malloc(sizeof(char *));
    trait_options.symbols_require_trait[0] = "register_trait";
    // this is the worst case in terms of performance,
    // as it only finds the trait in main binary and need to analyze all libraries fully to understand that trait is not required for them
    trait_handle_type handle = register_trait(&trait_options);
    free(trait_options.symbols_require_trait);

    bool result = check_trait(handle) || argc<3;
#else
    bool result = argc<3;
#endif

    dlopen_libraries();
#ifdef USE_TRAITS
    remove_trait(handle);
#endif

    if (result) {
        // abort program and dont actually call the libraries
        exit(0);
    } else { call_libraries(); }
}
"""

    # if several libs offer the same func: use only one of them
    funcs_used = set()

    # Add function declarations
    for lib, funcs in lib_func_map.items():
        for func in funcs:
            if func not in funcs_used:
                # pre declare a function so that the linker can resolve this symbol
                code += f"extern void {func}();\n"
                funcs_used.add(func)

    # calling the libraries
    code += """\nvoid call_libraries() {\n"""

    for func in funcs_used:
        code += f"if (&{func}!=NULL) {func}();  // Dummy call to force linking\n"

    code += """}\n"""

    # load symbols with dlopen if necessary
    code += """\nvoid dlopen_libraries() {\n"""
    if use_dlopen:
        # TODO implement
        pass
    code += """}\n"""

    # Save to file
    with open(output + ".c", "w") as f:
        f.write(code)

    # print("Generated test program using:")
    # for lib, funcs in lib_func_map.items():
    #    print(f"  {lib}: {', '.join(funcs)}")

    # print("Try compiling ")

    flags = [get_lib_link_flag(l) for l in lib_func_map]
    command = f"{compiler} {output}.c -o{output}_without.exe "
    command = command + " ".join(flags)

    if hide_output:
        redirect = subprocess.DEVNULL
    else:
        # keep default
        redirect = None

    if subprocess.call(command.split(), stdout=redirect, stderr=redirect) == 0:
        # successful
        command = f"{compiler} {output}.c -o{output}_with.exe "
        # with trait lib
        command = command + f"-DUSE_TRAITS=1 -I {source_dir} -I {build_dir} -L {build_dir} -lancor_programm_traits "
        # note that there are spaces in string at the end of the line to split the different args

        command = command + " ".join(flags)
        compilation_status = subprocess.call(command.split(), stdout=redirect, stderr=redirect)
        assert (compilation_status == 0)

        try:
            # check that no output is produced and no segfault occurs
            # as we link to random libraries, there may be ones, that do something in constructors/destructors
            # notably one that collect memory usage and print a summary
            # these programs actually perform some task, therefore we consider them invalid for our evaluation
            process_out_with = subprocess.check_output(f'./{output}_with.exe')
            process_out_without = subprocess.check_output(f'./{output}_without.exe')
            if len(process_out_with or process_out_without) > 0:
                if not hide_output:
                    print("fail compilation: trying again")
                generate_test_program(compilation_tries - 1, num_libs, num_funcs_per_lib, output, use_dlopen,
                                      use_weak_symbols, hide_output)

        except subprocess.CalledProcessError:
            generate_test_program(compilation_tries - 1, num_libs, num_funcs_per_lib, output, use_dlopen,
                                  use_weak_symbols, hide_output)

        if not hide_output:
            print(f"success, generated {output}.c and {output}_without.exe, {output}_with.exe")
        return lib_func_map
    else:
        if not hide_output:
            print("fail compilation: trying again")
        generate_test_program(compilation_tries - 1, num_libs, num_funcs_per_lib, output, use_dlopen, use_weak_symbols,
                              hide_output)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--compilation-tries", type=int, default=10, required=False)
    parser.add_argument("--output", type=str, default="test_code", required=False)
    parser.add_argument("--use-weak-symbols", action="store_true", required=False)
    parser.add_argument("--use-dlopen", action="store_true", required=False)
    parser.add_argument("--use_traits_lib", action="store_true", required=False)
    return parser.parse_args()


def main():
    args = parse_args()
    generate_test_program(compilation_tries=args.compilation_tries,
                          output=args.output,
                          use_dlopen=args.use_dlopen,
                          use_weak_symbols=args.use_weak_symbols)


if __name__ == "__main__":
    main()
