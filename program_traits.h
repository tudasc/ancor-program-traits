#ifndef ANCOR_PROGRAMM_TRAITS_PROGRAM_TRAITS_H
#define ANCOR_PROGRAMM_TRAITS_PROGRAM_TRAITS_H

#include <stdbool.h>
#include <glib.h>

#include "markers.h"

struct trait_options{
    char* name;// name of the marker (provided to the marker marco)
    unsigned int num_symbols_require_trait;
    char** symbols_require_trait; // name of the symbols that require the trait to be present
    // this means that the trait is present if NO marker was found but the given symbols are not used either
    bool skip_main_binary;// skips the main binary for analysis, as the symbols from the main binary are often removed from the dynamic symbol table by the compiler
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

#endif //ANCOR_PROGRAMM_TRAITS_PROGRAM_TRAITS_H
