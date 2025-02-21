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
            if lib_path.endswith(".so") and magic.from_file(lib_path, mime=True)=="application/x-sharedlib":
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
    return "-l"+lib_name[3:-3]

# Generate test C program
def generate_test_program(output="test_code"):
    lib_func_map = select_libs_and_funcs()

    code = """#include <stdio.h>\n"""

    # if several libs offer the same func: use only one of them
    funcs_used=set()

    # Add function declarations
    for lib, funcs in lib_func_map.items():
        for func in funcs:
            if func not in funcs_used:
                # pre declare a function so that the linker can resolve this symbol
                code += f"extern void {func}();\n"
                funcs_used.add(func)

    # Main function
    code += """\nint main() {\n\n"""
    # TODO add my main code here


    for func in funcs_used:
        code += f"if (&{func}!=NULL) {func}();  // Dummy call to force linking\n"

    #TODO any code after main
    code += """    printf("Test complete.\\n");\n    return 0;\n}\n"""

    # Save to file
    with open(output+".c", "w") as f:
        f.write(code)

    print("Generated test program using:")
    for lib, funcs in lib_func_map.items():
        print(f"  {lib}: {', '.join(funcs)}")

    print("Try compiling ")

    flags = [get_lib_link_flag(l) for l in lib_func_map]
    command ="gcc "+output+".c -o"+output+".exe " +" ".join(flags)
    if (subprocess.call(command.split())==0):
        exit(0)
    else:
        "print fail compilation: trying again"
        generate_test_program(output=output)






if __name__ == "__main__":
    generate_test_program()