#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "program_traits.h"
#include "markers.h"

marker(evaluation_trait)
marker(evaluation_trait_required)

#define USE_TRAITS

// variables for library calls
double result;
double input_sin;

void call_libraries() {
    memset(&input_sin,0,sizeof(double));
    // add calls to dynamically linked libraries here, so that the dynamic linker actually has something to do
    // and compiler will not be tempted to just remove this part altogether
    result = sin(input_sin);
}

void dlopen_libraries() {
    // add libraries with dlopen here
}

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
