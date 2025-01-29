
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "../../program_traits.h"

#define USE_WILDCARD
marker(no_wildcard)
// the usages of wildcard inside the MPI lib itself is permitted

#include "mpi_lib.h"
#include "../../program_traits.h"

int allow_wildcard_usage = 1;
trait_handle_type no_wildcard_trait_handle;
int initialized = 0;


void check_wildcard_usage_information() {
    // after initalization we ignore extra information
    if (!initialized) {

        printf("Check wildcard usage\n");
        struct trait_options options = {0};
        options.name = "no_wildcard";
        options.num_symbols_require_trait = 1;

        options.symbols_require_trait = malloc(sizeof(char *));
        options.symbols_require_trait[0] = "sample_mpi_recv";
        // it does work with false as well
        options.skip_main_binary = false;
        options.check_for_dlopen=true;
        options.check_for_mprotect=true;
        no_wildcard_trait_handle = register_trait(&options);
        free(options.symbols_require_trait);


        if (check_trait(no_wildcard_trait_handle)) {
            // deactivate wildcards
            allow_wildcard_usage = 0;
        }
    }
}

void sample_mpi_init(int *argc, char ***argv) {
    assert(initialized == 0);
    // check if wildcards are needed
    check_wildcard_usage_information();
    initialized = 1;
    printf("MPI Initialized with wildcards %s\n", allow_wildcard_usage ? "ENabled" : "DISabled");
}

void sample_mpi_recv(int tag) {
    assert(initialized);
    assert(tag != MATCHING_WILDCARD || allow_wildcard_usage);
    if (tag == MATCHING_WILDCARD) {
        printf("Allowed Wildcard usage\n");
    }
}

void sample_mpi_finalize() {
    assert(initialized);
    initialized = 0;

    remove_trait(no_wildcard_trait_handle);

}
