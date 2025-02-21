import os
import random
import subprocess
import magic


# Get all shared libraries installed
def get_shared_libs():
    result = subprocess.run(["ldconfig", "-p"], capture_output=True, text=True)
    libs = []
    for line in result.stdout.splitlines():
        lib_path = line.split()[-1]
        try:
            if lib_path.endswith(".so") and magic.from_file(lib_path, mime=True) == "application/x-sharedlib":
                libs.append(lib_path)
        except FileNotFoundError as e:
            # this is the info line telling us how many libraries are found
            pass

    return libs


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
    print("selected:")
    print(selected_libs)

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
def generate_test_program(compilation_tries=10, output="test_code", use_traits_lib=True, use_dlopen=False,
                          use_weak_symbols=False):
    lib_func_map = select_libs_and_funcs()

    if compilation_tries == 0:
        return

    code = """
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#ifdef USE_TRAITS
#include "program_traits.h"
#include "markers.h"

marker(evaluation_trait)
marker(evaluation_trait_required)
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
    trait_options.symbols_require_trait[0] = "evaluation_trait_required";
    // this is the worst case in terms of performance,
    // as it only finds the trait in main binary and need to analyze all libraries fully to understand that trait is not required for them
    trait_handle_type handle = register_trait(&trait_options);
    free(trait_options.symbols_require_trait);

    bool result = check_trait(handle);
#else
    bool result = argc>3
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
    if use_traits_lib:
        code = "#define USE_TRAITS 1\n" + code

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

    code += """\nvoid dlopen_libraries() {\n"""
    if use_dlopen:
        # TODO implement
        pass
    code += """}\n"""

    # Save to file
    with open(output + ".c", "w") as f:
        f.write(code)

    print("Generated test program using:")
    for lib, funcs in lib_func_map.items():
        print(f"  {lib}: {', '.join(funcs)}")

    print("Try compiling ")

    flags = [get_lib_link_flag(l) for l in lib_func_map]
    # TODO cmake should configure this script
    command = f"gcc {output}.c -o{output}.exe "
    if use_traits_lib:
        command = command + ("-I /home/tim/ancor-programm-traits "
                             "-I /home/tim/ancor-programm-traits/cmake-build-debug "
                             "-L /home/tim/ancor-programm-traits/cmake-build-debug "
                             "-lancor_programm_traits ")
        # note that there are spaces in string at the end of each line to split the diffferent args

    command = command + " ".join(flags)
    if (subprocess.call(command.split()) == 0):
        exit(0)
    else:
        print("fail compilation: trying again")
        generate_test_program(compilation_tries - 1, output, use_traits_lib, use_dlopen, use_weak_symbols)


if __name__ == "__main__":
    generate_test_program(10)
