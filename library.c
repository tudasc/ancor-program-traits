#include "library.h"

#define _GNU_SOURCE

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

int trait_evaluation_callback(struct dl_phdr_info *info, size_t size, void *data) {
    struct trait_results *trait = data;

    /* ElfW is a macro that creates proper typenames for the used system architecture
    * (e.g. on a 32 bit system, ElfW(Dyn*) becomes "Elf32_Dyn*") */

    const char *lib_name = strlen(info->dlpi_name) == 0 ? "(main binary)" : info->dlpi_name;
    printf("Read Library %d: %s\n", library_count, lib_name);

    uintptr_t vdso = (uintptr_t) getauxval(AT_SYSINFO_EHDR); // the address of the vdso (see the vdso manpage)
    if (info->dlpi_addr == vdso) {
        assert(library_count == 1 && "You dont have the vdso as the first library???");
        // do not analyze the vDSO (virtual dynamic shared object), it is part of the linux kernel and always present in every process
        // TODO the manpage says that it is a fully fledged elf, but it segfaults when i try to read its symbol table
        library_count++;
        return 0;
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


                        printf("%s\n\t\t%s\n",sym_name,trait->marker_to_look_for);
                        if (strcmp(sym_name, trait->marker_to_look_for)==0) {
                            //marker found
                            trait->is_true = TRUE;
                            printf("Library %d: %s: Has the Trait\n", library_count, lib_name);
                            library_count++;
                            return 0;
                        }
                    }
                }
                /* move pointer to the next entry */
                dyn++;
            }
        }
    }

    trait->is_true = FALSE;
    printf("Library %d: %s: DEOS NOT have the Trait\n", library_count, lib_name);

    library_count++;

    return 1;
    // nonzero ABORTs reading in the other libraries
}

void evaluate_trait(trait_handle_type trait) {
    assert(g_ptr_array_find(all_traits, trait, NULL));
    assert(!trait->is_evluated);
    printf("evaluate trait: %s\n", trait->options.name);
    dl_iterate_phdr(&trait_evaluation_callback, trait);


    trait->is_evluated = true;
}


trait_handle_type register_trait(struct trait_options *options) {


    printf("register trait: %s\n", options->name);

    if (all_traits == NULL) {
        // allocate new
        all_traits = g_ptr_array_new();
    }

    struct trait_results *handle = malloc(sizeof(struct trait_results));
    // copy in options
    memcpy(handle, options, sizeof(struct trait_options));

    // deepcopy
    handle->options.name = malloc(strlen(options->name) + 1);
    strcpy(handle->options.name, options->name);


    // initialize other fields
    handle->marker_to_look_for = malloc(
            strlen(MARKER_INTEGER_NAME_PREFIX_STR) + strlen(options->name) + 1);// 1 for null terminator
    strcpy(handle->marker_to_look_for, MARKER_INTEGER_NAME_PREFIX_STR);
    strcat(handle->marker_to_look_for, options->name);

    handle->is_evluated = false;
    handle->is_true = false;

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
    free(trait);
}
