#ifndef ANCOR_PROGRAMM_TRAITS_PROGRAM_TRAITS_H
#define ANCOR_PROGRAMM_TRAITS_PROGRAM_TRAITS_H

#include <stdbool.h>

#include "markers.h"

struct trait_options{
    char* name;
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
