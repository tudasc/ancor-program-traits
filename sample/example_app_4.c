
#include <stdio.h>
#include <assert.h>
#include <dlfcn.h>

// content of "wrapper_lib.h" but with weak linkeage

__attribute__((weak)) void perform_operation();
typedef void (*func_ptr_type)();

//#define LIBRARY_TO_LOAD to the library that should be dlopened

#include "mpi_lib.h"

#ifndef TAG_TO_USE
#define TAG_TO_USE 42
#endif

int main(int argc, char **argv) {

    sample_mpi_init(NULL,NULL);

    printf("dlopen: %s\n",LIBRARY_TO_LOAD);

    // dynamically open the library
    void* handle = dlopen(LIBRARY_TO_LOAD, RTLD_LAZY);
    func_ptr_type perform = dlsym(handle, "perform_operation");

    sample_mpi_recv(TAG_TO_USE);

    if (perform != NULL) {
        perform();
    } else {
        printf("Failed to dlopen the correct library\n");
    }

    dlclose(handle);

    sample_mpi_finalize();
}
