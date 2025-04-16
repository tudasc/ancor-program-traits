# Embedding Program Traits

Embedding of marker symbols for dynamic detection of program traits

# Build

* Build with cmake:
  `mkdir build && cd build && cmake .. && make` cmake options:
    * `HOOK_DLOPEN`: disable hooking of `dlopen` (default On)
    * `HOOK_MPROTECT`: disable hooking of `mprotect` (default On)
  * `BUILD_MPI_EXAMPLES`: enables building of MPI examples. This builds MPICH MPI implementation and a wrapper
    showcasing how MPI could use the trait embedding mechanism with MPI Mini-Apps (default Off)
* run tests with `ctest`

# Usage

## Include Markers

In the program/library that one wants to include a trait marker for `my_trait`

* include `markers.h`
* use `marker(my_trait)` macro, one can include multiple trait markers.

## Analyze Traits

The program or library that analyzes if the trait `my_trait` holds for all parts of the program uses the
`libancor_programm_traits` library to inquire about the traits:

* allocate a `trait_options` struct
* Set up the data structure with the relevant settings:
    * `char* name`: name of the marker (the same one that is used in the marker marco)
    * `char** symbols_require_trait`: list of symbols that require this trait or `NULL` if the trait always needs to be
      present. This means that a trait is considered to be irrelevant if none of these symbols are found.
        * `unsigned int num_symbols_require_trait`: number of entries in that list
    * `bool skip_main_binary`: True if the trait should not be checked for the main binary and only the linked
      libraries, false otherwise.
    * `bool check_for_dlopen`: Continuously monitor if the trait is still valid after loading additional libraries with
      `dlopen`, print an error message and exit if a trait violation is found.
    * `bool check_for_mprotect`: Continuously monitor if the trait is still valid after it is marked as executable with
      `mprotect`, print an error message, and exit if trait violation is found. The current implementation does not
      actually check for trait violation but assumes one on marking memory as executable.
    * `char** libraies_to_skip`: path to `.so` files to skip analysis of (and assume trait is fulfilled). E.g. if the
      library currently checking for the traits has `dlopen` to load some implementation on demand, one could skip
      checking of this `dlopen`
        * `unsigned int num_libraies_to_skip`:number of entries in that list
* call `register_trait()` with the `trait_options` data structure as the argument. This will return a handle, the
  options
  may be freed at that point
* use `check_trait()` with the handle as an argument to check if the trait is fulfilled.
* `remove_trait()` on this handle will free all resources used and remove the checking for `dlopen` and `mprotect` for
  this trait.

# Evaluation

Evaluation scripts are in the `performance-evaluation` directory.
Run the Python script `evaluation_runner.py` in order to collect performance data.
This will generate and execute test programs that link to randomly installed libraries (no library is actually called).
These results can be visualized with the `visualize_results.py` script.

