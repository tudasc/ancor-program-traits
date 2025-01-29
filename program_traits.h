#ifndef ANCOR_PROGRAMM_TRAITS_PROGRAM_TRAITS_H
#define ANCOR_PROGRAMM_TRAITS_PROGRAM_TRAITS_H

#include <stdbool.h>
#include "markers.h"

struct trait_options{
    char* name;// name of the marker (provided to the marker marco)
    unsigned int num_symbols_require_trait;
    char** symbols_require_trait; // name of the symbols that require the trait to be present
    // this means that the trait is present if NO marker was found but the given symbols are not used either
    bool skip_main_binary;// skips the main binary for analysis, as the symbols from the main binary are often removed from the dynamic symbol table by the compiler
    // these mechanisms can be used to introduce code to the binary after teh analysis of the trait was executed
    bool check_for_dlopen;
    bool check_for_mprotect;
    // libraries to excerpt from the analysis, e.g. if the own library has dlopen to load some implementation on demand
    unsigned int num_libraies_to_skip;
    char** libraies_to_skip;
};

struct trait_results;

typedef struct trait_results* trait_handle_type;

// TODO: Documentation
// registeres a trait
trait_handle_type register_trait(struct trait_options* options);

// check if the trait is present
bool check_trait(trait_handle_type trait);

// free ressources
void remove_trait(trait_handle_type trait);

//TODO this points to specific system libraries like libC
// this should be set at configure time
#define LIBC "/lib/x86_64-linux-gnu/libc.so.6"
#define LIBDL "/lib/x86_64-linux-gnu/libdl.so.2"

#endif //ANCOR_PROGRAMM_TRAITS_PROGRAM_TRAITS_H
