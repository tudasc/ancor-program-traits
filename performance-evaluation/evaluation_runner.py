import subprocess
import pandas as pd
from timeit import timeit
from generate_test_code import generate_test_program
import os
from tqdm import tqdm

# setup
number_experiments = 50
repetitions = 100

output_filename = "test_program"


def get_lib_usage_info(program):
    try:
        # Run ldd to get the linked libraries
        result = subprocess.run(["ldd", program], capture_output=True, text=True, check=True)

        total_size = 0
        lib_count = 0

        for line in result.stdout.splitlines():
            # Ignore vdso
            if "vdso" in line:
                continue

            # Extract the library path
            parts = line.split(" => ")
            if len(parts) == 2:
                lib_path = parts[1].split(" (")[0].strip()
            else:
                lib_path = parts[0].split(" (")[0].strip()

            if os.path.exists(lib_path):
                lib_count += 1
                total_size += os.path.getsize(lib_path)

        return lib_count, total_size

    except subprocess.CalledProcessError as e:
        print(f"Error running ldd: {e}")
        return None


def main():
    results = []

    for i in tqdm(range(number_experiments)):
        generate_test_program(output=output_filename, hide_output=True)
        count, size = get_lib_usage_info(output_filename + "_without.exe")

        for j in range(repetitions):
            timing_without = timeit(
                stmt=f"subprocess.call('./{output_filename}_without.exe', stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)",
                setup="import subprocess",
                number=1)
            timing_with = timeit(
                stmt=f"subprocess.call('./{output_filename}_with.exe', stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)",
                setup="import subprocess",
                number=1)
            # print([count, size, timing_with, timing_without])
            results.append([count, size, timing_with, timing_without])

    df = pd.DataFrame(results, columns=["num_libs", "combined_libs_size", "timing_with", "timing_without"])
    df.to_csv("evaluation_results.csv")

    print("Summary:")
    print(df.describe())


if __name__ == "__main__":
    main()
