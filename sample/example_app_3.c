
#include <stdio.h>
#include <assert.h>
#include <dlfcn.h>

// content of "wrapper_lib.h" but with weak linkeage
__attribute__((weak)) void init_library();

__attribute__((weak)) void perform_operation();

__attribute__((weak)) void finish_library();

typedef void (*func_ptr_type)();

//#define LIBRARY_TO_LOAD to the library that should be dlopened

int main(int argc, char **argv) {

    printf("dlopen: %s\n",LIBRARY_TO_LOAD);

    // dynamically open the library
    void* handle = dlopen(LIBRARY_TO_LOAD, RTLD_NOW);
    func_ptr_type init = dlsym(handle, "init_library");
    func_ptr_type perform = dlsym(handle, "perform_operation");
    func_ptr_type fini = dlsym(handle, "finish_library");


    if (init != NULL) {
        assert(perform != NULL);
        assert(fini != NULL);
        init();
        perform();
        fini();
    } else {
        printf("Failed to dlopen the correct library\n");
    }

    dlclose(handle);
}
