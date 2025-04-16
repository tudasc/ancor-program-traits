# Embedding programm traits

embedding of Marker symbols for dynamic detection of Program traits

# Build

# Usage

## Include Markers

In the program/library that one wants to include a trait marker for `my_trait`

* include `markers.h`
* use `marker(my_trait)` macro, one can include multiple trait markers

## analyze traits

The Program or library that analyzes if the trait `my_trait` holds for all parts of the program uses the
`libancor_programm_traits` library to inquire about the traits:

* allocate a `trait_options` struct
* Set up the datastructure with the relevant settings:
    * `char* name`: name of the marker (the same one that is used in to the marker marco)
    * `char** symbols_require_trait`: list of symbols that require this trait or `NULL` if the trait always needs to be
      present. This means that a trait is considered to be irrellevant if none of these symbols are found.
        * `unsigned int num_symbols_require_trait`: number of entries in that list
    * `bool skip_main_binary`: True if the trait should not be checked for the main binary and only the linked
      libraries, false otherwise.
    * `bool check_for_dlopen`: Continuously monitor if the trait is still valid after loading additional libraries with
      `dlopen`, print an error message and exit if trait violation is found.
    * `bool check_for_dlopen`: Continuously monitor if the trait is still valid after is marked as executable with
      `mprotect`, print an error message and exit if trait violation is found. (current implementation does not actually
      check for trait violation but assumes one on marking memory as executable)
    * `char** libraies_to_skip`: path to `.so` files to skip analysis of (and assume trait is fulfilled). E.g. if the
      library currently checking for the traits has `dlopen` to load some implementation on demand, one could skip
      checking of this `dlopen`
        * `unsigned int num_libraies_to_skip`:number of entries in that list
* call `register_trait()` with the `trait_options` datastructure as the argument. This will return a handle, the options
  may be freed at that point
* use `check_trait()` with the handle as an argument to check if the trait is fulfilled.
* `remove_trait()` on this handle will free all ressources used and remove the checking for `dlopen` and `mprotect` for
  this trait.