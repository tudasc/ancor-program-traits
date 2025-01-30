#include "mpi_wrapper.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include <program_traits.h>

#define USE_WILDCARD
marker(no_wildcard)
// this wrapper lib has no wildcards
#include <mpi.h>


#ifndef _EXTERN_C_
#ifdef __cplusplus
#define _EXTERN_C_ extern "C"
#else /* __cplusplus */
#define _EXTERN_C_
#endif /* __cplusplus */
#endif /* _EXTERN_C_ */

#ifdef MPICH_HAS_C2F
_EXTERN_C_ void *MPIR_ToPointer(int);
#endif // MPICH_HAS_C2F

#ifdef PIC
/* For shared libraries, declare these weak and figure out which one was linked
   based on which init wrapper was called.  See mpi_init wrappers.  */
#pragma weak pmpi_init
#pragma weak PMPI_INIT
#pragma weak pmpi_init_
#pragma weak pmpi_init__
#endif /* PIC */

_EXTERN_C_ void pmpi_init(MPI_Fint *ierr);

_EXTERN_C_ void PMPI_INIT(MPI_Fint *ierr);

_EXTERN_C_ void pmpi_init_(MPI_Fint *ierr);

_EXTERN_C_ void pmpi_init__(MPI_Fint *ierr);


int allow_wildcard_usage = 1;
trait_handle_type no_wildcard_trait_handle;
int initialized = 0;

//TODO code duplication with modified_mpi/mpi_lib.c
void check_wildcard_usage_information() {
    // after initalization we ignore extra information
    if (!initialized) {
        printf("Check wildcard usage\n");
        struct trait_options options = {0};
        options.name = "no_wildcard";
        options.num_symbols_require_trait = 2;

        options.symbols_require_trait = malloc(2 * sizeof(char *));
        options.symbols_require_trait[0] = "MPI_Recv";
        options.symbols_require_trait[1] = "MPI_Irecv";
        // it does work with false as well
        options.skip_main_binary = false;
        options.check_for_dlopen = true;
        options.check_for_mprotect = true;
        options.num_libraies_to_skip = 1;
#ifdef LIBVERBS_LOCATION
        ++options.num_libraies_to_skip;
#endif
#ifdef LIBTLDL_LOCATION
        ++options.num_libraies_to_skip;
#endif
        options.libraies_to_skip = malloc(options.num_libraies_to_skip * sizeof(char *));
        // these libraries internal to MPICH use dlopen (we know that they do not dynamically load something that will interfere with wildcard matching)
        int i = 0;
        options.libraies_to_skip[i] = LIBMPI_LOCATION;
#ifdef LIBVERBS_LOCATION
        ++i;
        options.libraies_to_skip[i] = LIBVERBS_LOCATION;
#endif
        // TODO problem if the user uses libltdl to dlopen something (lt_dlopen), we would not catch it anymore
#ifdef LIBTLDL_LOCATION
        ++i;
        options.libraies_to_skip[i] = LIBTLDL_LOCATION;
#endif

        no_wildcard_trait_handle = register_trait(&options);
        free(options.symbols_require_trait);
        free(options.libraies_to_skip);


        if (check_trait(no_wildcard_trait_handle)) {
            // deactivate wildcards
            allow_wildcard_usage = 0;
        }
    }
}

/* ================== C Wrappers for MPI_Init ================== */
_EXTERN_C_ int PMPI_Init(int *argc, char ***argv);

_EXTERN_C_ int MPI_Init(int *argc, char ***argv) {
    assert(initialized == 0);
    // check if wildcards are needed
    check_wildcard_usage_information();
    initialized = 1;
    printf("MPI Initialized with wildcards %s\n", allow_wildcard_usage ? "ENabled" : "DISabled");

    return PMPI_Init(argc, argv);
}

/* ================== C Wrappers for MPI_Init_thread ================== */
_EXTERN_C_ int PMPI_Init_thread(int *argc, char ***argv, int required, int *provided);

_EXTERN_C_ int MPI_Init_thread(int *argc, char ***argv, int required, int *provided) {
    assert(initialized == 0);
    // check if wildcards are needed
    check_wildcard_usage_information();
    initialized = 1;
    printf("MPI Initialized with wildcards %s\n", allow_wildcard_usage ? "ENabled" : "DISabled");
    return PMPI_Init_thread(argc, argv, required, provided);
}

/* ================== C Wrappers for MPI_Recv ================== */
_EXTERN_C_ int PMPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                         MPI_Comm comm, MPI_Status *status);

_EXTERN_C_ int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                        MPI_Comm comm, MPI_Status *status) {
    assert(initialized);
    if (source == MPI_ANY_SOURCE || tag == MPI_ANY_TAG) {
        assert(allow_wildcard_usage);
        printf("Allowed Wildcard usage\n");
    }
    return PMPI_Recv(buf, count, datatype, source, tag, comm, status);
}

/* ================== C Wrappers for MPI_IRecv ================== */
_EXTERN_C_ int PMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                          MPI_Comm comm, MPI_Request *request);

_EXTERN_C_ int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                         MPI_Comm comm, MPI_Request *request) {
    assert(initialized);
    if (source == MPI_ANY_SOURCE || tag == MPI_ANY_TAG) {
        assert(allow_wildcard_usage);
        printf("Allowed Wildcard usage\n");
    }

    return PMPI_Irecv(buf, count, datatype, source, tag, comm, request);
}
