#define _GNU_SOURCE

#include "program_traits.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <sys/auxv.h>

#include <link.h>
#include <elf.h>

#include <glib.h>

struct trait_results {
    struct trait_options options;
    char *marker_to_look_for;
    bool is_evluated;
    bool is_true;
    bool found_dlopen;
    bool found_mprotect;
};

int library_count = 0;

int this_is_an_example;

GPtrArray *all_traits;
// from:
// https://stackoverflow.com/questions/15779185/how-to-list-on-the-fly-all-the-functions-symbols-available-in-c-code-on-a-linux
// only works for 64 bit
static_assert(sizeof(ElfW(Addr)) == sizeof(Elf64_Addr));

static uint32_t GetNumberOfSymbolsFromGnuHash(Elf64_Addr gnuHashAddress) {
    // See https://flapenguin.me/2017/05/10/elf-lookup-dt-gnu-hash/ and
    // https://sourceware.org/ml/binutils/2006-10/msg00377.html
    typedef struct {
        uint32_t nbuckets;
        uint32_t symoffset;
        uint32_t bloom_size;
        uint32_t bloom_shift;
    } Header;

    Header *header = (Header *) gnuHashAddress;
    const void *bucketsAddress = (uint8_t *) gnuHashAddress + sizeof(Header) + (sizeof(uint64_t) * header->bloom_size);

    // Locate the chain that handles the largest index bucket.
    uint32_t lastSymbol = 0;
    uint32_t *bucketAddress = (uint32_t *) bucketsAddress;
    for (uint32_t i = 0; i < header->nbuckets; ++i) {
        uint32_t bucket = *bucketAddress;
        if (lastSymbol < bucket) {
            lastSymbol = bucket;
        }
        bucketAddress++;
    }

    if (lastSymbol < header->symoffset) {
        return header->symoffset;
    }

    // Walk the bucket's chain to add the chain length to the total.
    const void *chainBaseAddress = (uint8_t *) bucketsAddress + (sizeof(uint32_t) * header->nbuckets);
    for (;;) {
        uint32_t *chainEntry = (uint32_t *) ((uint8_t *) chainBaseAddress +
                                             (lastSymbol - header->symoffset) * sizeof(uint32_t));
        lastSymbol++;

        // If the low bit is set, this entry is the end of the chain.
        if (*chainEntry & 1) {
            break;
        }
    }

    return lastSymbol;
}

bool check_skip_library(struct dl_phdr_info *info, struct trait_results *trait) {
    const char *lib_name = strlen(info->dlpi_name) == 0 ? "(main binary)" : info->dlpi_name;

    // skip vdso
    uintptr_t vdso = (uintptr_t) getauxval(AT_SYSINFO_EHDR); // the address of the vdso (see the vdso manpage)
    if (info->dlpi_addr == vdso) {
        assert(library_count == 2 && "You dont have the vdso as the first library???");
        // do not analyze the vDSO (virtual dynamic shared object), it is part of the linux kernel and always present in every process
        // TODO the manpage says that it is a fully fledged elf, but it segfaults when i try to read its symbol table
        printf("Library %d: %s: skip\n", library_count, lib_name);
        return true;
    }

    // skip main binary if indicated
    if (trait->options.skip_main_binary && strlen(info->dlpi_name) == 0) {
        // skip main binary
        printf("Library %d: %s: skip\n", library_count, lib_name);
        return true;
    }

    // check if it is in list of skipped binaries

    for (unsigned int i = 0; i < trait->options.num_libraies_to_skip; ++i) {
        if (strcmp(trait->options.libraies_to_skip[i], lib_name) == 0) {
            printf("Library %d: %s: skip\n", library_count, lib_name);
            return true;
        }
    }

    return false;
}

int trait_evaluation_callback(struct dl_phdr_info *info, size_t size, void *data) {
    struct trait_results *trait = data;

    library_count++;
    /* ElfW is a macro that creates proper typenames for the used system architecture
    * (e.g. on a 32 bit system, ElfW(Dyn*) becomes "Elf32_Dyn*") */

    const char *lib_name = strlen(info->dlpi_name) == 0 ? "(main binary)" : info->dlpi_name;
    //printf("Read Library %d: %s\n", library_count, lib_name);
    if (strlen(info->dlpi_name) == 0) {
        assert(library_count == 1 && "The Main Binary is not the first one to be analyzed??");
    }

    if (check_skip_library(info, trait)) {
        return 0; // skip
    }

    // we need to check again if it still holds for this library
    trait->is_true = FALSE;
    bool has_marker = false;
    bool has_required_symbol = FALSE;
    if (trait->options.num_symbols_require_trait == 0) {
        // trait is always important
        has_required_symbol = TRUE;
    }


    char *strtab = 0;
    char *sym_name = 0;
    ElfW(Word) sym_cnt = 0;

    for (ElfW(Half) header_index = 0; header_index < info->dlpi_phnum; header_index++) {
        /* Further processing is only needed if the dynamic section is reached */
        if (info->dlpi_phdr[header_index].p_type == PT_DYNAMIC) {
            /* Get a pointer to the first entry of the dynamic section.
             * It's address is the shared lib's address + the virtual address */
            ElfW(Dyn *)dyn = (ElfW(Dyn) *) (info->dlpi_addr + info->dlpi_phdr[header_index].p_vaddr);

            /* Iterate over all entries of the dynamic section until the
             * end of the symbol table is reached. This is indicated by
             * an entry with d_tag == DT_NULL.
             *
             * Only the following entries need to be processed to find the
             * symbol names:
             *  - DT_HASH   -> second word of the hash is the number of symbols
             *  - DT_STRTAB -> pointer to the beginning of a string table that
             *                 contains the symbol names
             *  - DT_SYMTAB -> pointer to the beginning of the symbols table
             */
            while (dyn->d_tag != DT_NULL) {
                if (dyn->d_tag == DT_HASH) {
                    /* Get a pointer to the hash */
                    ElfW(Word *)hash = (ElfW(Word *)) dyn->d_un.d_ptr;

                    /* The 2nd word is the number of symbols */
                    sym_cnt = hash[1];
                } else if (dyn->d_tag == DT_GNU_HASH && sym_cnt == 0) {
                    sym_cnt = GetNumberOfSymbolsFromGnuHash(dyn->d_un.d_ptr);
                } else if (dyn->d_tag == DT_STRTAB) {
                    /* Get the pointer to the string table */
                    strtab = (char *) dyn->d_un.d_ptr;
                } else if (dyn->d_tag == DT_SYMTAB) {
                    /* Get the pointer to the first entry of the symbol table */
                    ElfW(Sym *)sym = (ElfW(Sym *)) dyn->d_un.d_ptr;


                    /* Iterate over the symbol table */
                    for (ElfW(Word) sym_index = 0; sym_index < sym_cnt; sym_index++) {
                        /* get the name of the i-th symbol.
                         * This is located at the address of st_name
                         * relative to the beginning of the string table. */
                        sym_name = &strtab[sym[sym_index].st_name];
                        //printf("%d\n", sym_index);

                        //parse_symbol_table_name
                        if (strcmp(sym_name, trait->marker_to_look_for) == 0) {
                            //marker found
                            trait->is_true = TRUE;
                            printf("Library %d: %s: Found Marker\n", library_count, lib_name);
                            has_marker = TRUE;
                        }
                        if (trait->options.check_for_dlopen && strcmp(sym_name, "dlopen") == 0 && strcmp(
                                lib_name, LIBDL) != 0) {
                            printf("Library %d: %s: Found dlopen\n", library_count, lib_name);
                            trait->is_true = FALSE;
                            return 1; // abort
                        }
                        if (trait->options.check_for_mprotect && strcmp(sym_name, "mprotect") == 0 && strcmp(
                                lib_name, LIBC) != 0) {
                            printf("Library %d: %s: Found mprotect\n", library_count, lib_name);
                            trait->is_true = FALSE;
                            return 1; // abort
                        }

                        if (!has_required_symbol) {
                            assert(trait->options.symbols_require_trait != NULL);
                            for (unsigned int i = 0; i < trait->options.num_symbols_require_trait; ++i) {
                                if (strcmp(sym_name, trait->options.symbols_require_trait[i]) == 0) {
                                    has_required_symbol = true;
                                    break; // no need to check if other required symbols are also present
                                }
                            }
                        }
                        // end parse symbol in table
                    } // end for each symbol table entry
                }
                /* move pointer to the next elf section */
                dyn++;
            }
        }
    }
    if (!has_marker) {
        if (has_required_symbol) {
            trait->is_true = FALSE;
            printf("Library: %s: DOES NOT have the Trait\n", lib_name);
            // found violation: abort
            return 1;
        } else {
            // has none of the symbols that require the trait
            printf("Library %d: %s: trait not required\n", library_count, lib_name);
            trait->is_true = TRUE;
            return 0;
        }
    }
    return 0;

    // nonzero ABORTs reading in the other libraries
}

// from dlfcn.h but with weak linkeage to check for its presence at runtime
__attribute__((weak)) void *dlopen(const char *filename, int flag);

void evaluate_trait(trait_handle_type trait) {
    assert(g_ptr_array_find(all_traits, trait, NULL));
    assert(!trait->is_evluated);
    printf("evaluate trait: %s\n", trait->options.name);

    // check all libraries
    dl_iterate_phdr(&trait_evaluation_callback, trait);

    if (trait->options.check_for_dlopen && trait->found_dlopen) {
        printf("Found Use of dlopen, cannot analyze trait, must assume it does not hold anymore after dlopen usage\n");
        assert(trait->is_true == false);
        if (trait->options.check_for_mprotect && trait->found_mprotect) {
            printf(
                "Found Use of mprotect , cannot analyze trait, must assume it does not hold anymore after mprotect  usage\n");
            assert(trait->is_true == false);
        }
        trait->is_evluated = true;
    }
}

trait_handle_type register_trait(struct trait_options *options) {
    printf("register trait: %s\n", options->name);

    if (all_traits == NULL) {
        // allocate new
        all_traits = g_ptr_array_new();
    }

    struct trait_results *handle = malloc(sizeof(struct trait_results));
    // copy in options (deepcopy)
    handle->options.name = malloc(strlen(options->name) + 1);
    strcpy(handle->options.name, options->name);

    if (options->num_symbols_require_trait > 0) {
        handle->options.num_symbols_require_trait = options->num_symbols_require_trait;
        handle->options.symbols_require_trait = malloc(sizeof(char *) * options->num_symbols_require_trait);
        for (unsigned int i = 0; i < options->num_symbols_require_trait; ++i) {
            char *new_buf = malloc(strlen(options->symbols_require_trait[i]) + 1); // 1 for null terminator
            strcpy(new_buf, options->symbols_require_trait[i]);
            handle->options.symbols_require_trait[i] = new_buf;
        }
    }
    handle->options.skip_main_binary = options->skip_main_binary;

    handle->options.check_for_dlopen = options->check_for_dlopen;
    handle->options.check_for_mprotect = options->check_for_mprotect;

    // initialize other fields
    handle->marker_to_look_for = malloc(
        strlen(MARKER_INTEGER_NAME_PREFIX_STR) + strlen(options->name) + 1); // 1 for null terminator
    strcpy(handle->marker_to_look_for, MARKER_INTEGER_NAME_PREFIX_STR);
    strcat(handle->marker_to_look_for, options->name);

    handle->is_evluated = false;
    handle->is_true = false;
    handle->found_dlopen = false;
    handle->found_mprotect = false;

    g_ptr_array_add(all_traits, handle);
    return handle;
}

// check if the trait is present
bool check_trait(trait_handle_type trait) {
    printf("check trait: %s\n", trait->options.name);
    assert(g_ptr_array_find(all_traits, trait, NULL));

    if (!trait->is_evluated) {
        evaluate_trait(trait);
    }
    return trait->is_true;
}


void remove_trait(trait_handle_type trait) {
    printf("remove trait: %s\n", trait->options.name);
    assert(g_ptr_array_find(all_traits, trait, NULL));
    g_ptr_array_remove(all_traits, trait);
    free(trait->marker_to_look_for);
    free(trait->options.name);


    if (trait->options.num_symbols_require_trait > 0) {
        for (unsigned int i = 0; i < trait->options.num_symbols_require_trait; ++i) {
            free(trait->options.symbols_require_trait[i]);
        }
        free(trait->options.symbols_require_trait);
    }
    free(trait);

    // if last trait removed: free all ressources
    if (all_traits->len == 0) {
        g_ptr_array_free(all_traits, true);
        all_traits = NULL;
    }
}


/*
// this approach of intercepting dlopen would work if one LD_Preloads our library
typedef void *(*dlopen_fnptr_t)(const char *, int);

dlopen_fnptr_t real_dlopen = NULL;
dlopen_fnptr_t fake_dlopen = NULL;

void *dlopen(const char *filename, int flag) {
    // fake dlopen as a handler to handle dlopen calls
    if (real_dlopen == NULL) {
        assert(fake_dlopen == NULL);
        fake_dlopen = &dlopen;
        real_dlopen = dlsym(RTLD_NEXT, "dlopen");
    }
    void* result_handle = real_dlopen(filename, flag);
    printf("Intercepted dlopen\n");
    return result_handle;
}
 */
