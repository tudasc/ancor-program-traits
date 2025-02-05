#define _GNU_SOURCE

#include "program_traits.h"

#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <sys/auxv.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <link.h>
#include <elf.h>

#include <glib.h>

#include <sys/mman.h>

#include "plthook.h"

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

static uint32_t GetNumberOfSymbolsFromGnuHash(const Elf64_Addr gnuHashAddress) {
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
#ifdef HOOK_DLOPEN
int install_dlopen_plt_hook(struct dl_phdr_info *info);
#endif
#ifdef HOOK_MPROTECT
int install_mprotect_plt_hook(struct dl_phdr_info *info);
#endif

// from: https://stackoverflow.com/questions/4031672/without-access-to-argv0-how-do-i-get-the-program-name
char *get_program_path() {
    char *path = malloc(PATH_MAX);
    if (path != NULL) {
        if (readlink("/proc/self/exe", path, PATH_MAX) == -1) {
            free(path);
            path = NULL;
        }
    }
    return path;
}

// return 0 if program executable has the desired trait
int check_static_symbol_table_of_main_binary(struct trait_results *trait) {
    char *program_path = get_program_path();
    if (program_path == NULL) {
        assert(trait->is_true==FALSE);
        return 1;
    }
    // file exists
    if (access(program_path, F_OK) != 0) {
        free(program_path);
        return 1;
    }

    FILE *file = fopen(program_path, "rb");
    if (!file) {
        free(program_path);
        assert(trait->is_true==FALSE);
        return 1;
    }
    ElfW(Ehdr) ehdr;
    // Read ELF header
    if (fread(&ehdr, 1, sizeof(ElfW(Ehdr)), file) != sizeof(ElfW(Ehdr))) {
        // failed to read elf header
        free(program_path);
        assert(trait->is_true==FALSE);
        return 1;
    }
    // Check ELF magic bytes
    if (ehdr.e_ident[EI_MAG0] != ELFMAG0 ||
        ehdr.e_ident[EI_MAG1] != ELFMAG1 ||
        ehdr.e_ident[EI_MAG2] != ELFMAG2 ||
        ehdr.e_ident[EI_MAG3] != ELFMAG3) {
        //Not a valid ELF file
        free(program_path);
        assert(trait->is_true==FALSE);
        return 1;
    }


    // Seek to section header table
    fseek(file, ehdr.e_shoff, SEEK_SET);

    ElfW(Shdr) shdr;
    ElfW(Shdr) symtab, strtab;
    int found_symtab = 0, found_strtab = 0;

    // Find the symbol table and string table sections
    for (unsigned int i = 0; i < ehdr.e_shnum; i++) {
        fread(&shdr, 1, sizeof(ElfW(Shdr)), file);
        if (shdr.sh_type == SHT_SYMTAB) {
            symtab = shdr;
            found_symtab = 1;
            // found the symbol table, need to get the associated string table for the names
            fseek(file, ehdr.e_shoff + sizeof(ElfW(Shdr)) * symtab.sh_link, SEEK_SET);
            fread(&strtab, 1, sizeof(ElfW(Shdr)), file);
            if (strtab.sh_type == SHT_STRTAB) {
                found_strtab = 1;
            }
        }
    }

    if (!found_symtab || !found_strtab) {
        // did not find the symbol table or the string table with the names
        assert(trait->is_true==FALSE);
        free(program_path);
        return 1;
    }

    // Read the symbol table
    fseek(file, symtab.sh_offset, SEEK_SET);
    int num_symbols = symtab.sh_size / symtab.sh_entsize;
    ElfW(Sym) *symbols = malloc(symtab.sh_size);

    fread(symbols, 1, symtab.sh_size, file);

    // Read the string table
    fseek(file, strtab.sh_offset, SEEK_SET);
    char *strtab_data = malloc(strtab.sh_size);
    fread(strtab_data, 1, strtab.sh_size, file);

    // read symbol names
    for (int i = 0; i < num_symbols; i++) {
        char *sym_name = &strtab_data[symbols[i].st_name];
        printf("%s\n", sym_name);
        if (strcmp(sym_name, trait->marker_to_look_for) == 0) {
            //marker found
            printf("Library %d: %s: Found Marker (in static symbol table)\n", library_count, "(main Binary)");
            trait->is_true = TRUE;
            free(symbols);
            free(strtab_data);
            free(program_path);

            return 0;
        }
    }

    free(symbols);
    free(strtab_data);
    free(program_path);
    assert(trait->is_true==FALSE);
    printf("Library: %s: DOES NOT have the Trait (even in static symbol table)\n", "(main Binary)");
    return 1;
}


bool is_same_lib(const char *lib_name_a, const char *lib_name_b) {
    struct stat stat_lib_a = {0};
    stat(lib_name_a, &stat_lib_a);
    struct stat stat_lib_b = {0};
    stat(lib_name_b, &stat_lib_b);
    return (stat_lib_a.st_dev == stat_lib_b.st_dev &&
            stat_lib_a.st_ino == stat_lib_b.st_ino);
}

bool is_libc(const char *lib_name) {
    return is_same_lib(lib_name, LIBC_LOCATION);
}

bool is_libdl(const char *lib_name) {
    return is_same_lib(lib_name, LIBDL_LOCATION);
}

bool check_skip_library(struct dl_phdr_info *info, struct trait_results *trait) {
    const char *lib_name = strlen(info->dlpi_name) == 0 ? "(main binary)" : info->dlpi_name;

    // skip vdso
    uintptr_t vdso = (uintptr_t) getauxval(AT_SYSINFO_EHDR); // the address of the vdso (see the vdso manpage)
    if (info->dlpi_addr == vdso) {
        assert(library_count == 2 && "You dont have the vdso as the first library???");
        // do not analyze the vDSO (virtual dynamic shared object), it is part of the linux kernel and always present in every process
        // TODO the manpage says that it is a fully fledged elf, but it segfaults when i try to read its symbol table
        return true;
    }

    //TODO make this an option
    // skip self
    if (is_same_lib(lib_name, LIB_PROGRAM_TRAITS_LOCATION)) {
        return true;
    }
    // skip libplthook (as it is part of self)
    if (is_same_lib(lib_name, LIB_PLTHOOK_LOCATION)) {
        return true;
    }

    // skip main binary if indicated
    if (strlen(info->dlpi_name) == 0) {
        if (trait->options.skip_main_binary) {
            // skip main binary
            return true;
        } else {
            return false;
        }
    }

    // check if it is in list of skipped binaries
    for (unsigned int i = 0; i < trait->options.num_libraies_to_skip; ++i) {
        if (is_same_lib(lib_name, trait->options.libraies_to_skip[i])) {
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
        printf("Library %d: %s: skip\n", library_count, lib_name);
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
                        //printf("%s\n", sym_name);

                        //parse_symbol_table_name
                        if (strcmp(sym_name, trait->marker_to_look_for) == 0) {
                            //marker found
                            trait->is_true = TRUE;
                            printf("Library %d: %s: Found Marker\n", library_count, lib_name);
                            has_marker = TRUE;
                        }
                        if (trait->options.check_for_dlopen && strcmp(sym_name, "dlopen") == 0 && !is_libdl(lib_name)) {
#ifdef HOOK_DLOPEN

                            int status = install_dlopen_plt_hook(info);
                            if (status != 0) {
                                printf("Library %d: %s: Found dlopen, Failed to install plthook\n", library_count,
                                       lib_name);
                                trait->is_true = FALSE;
                                return 1; // abort
                            }
#else
                            printf("Library %d: %s: Found dlopen\n", library_count, lib_name);
                            trait->is_true = FALSE;
                            return 1; // abort
#endif
                        }
                        if (trait->options.check_for_mprotect && strcmp(sym_name, "mprotect") == 0 && !
                            is_libc(lib_name)) {
#ifdef HOOK_MPROTECT
                            int status = install_mprotect_plt_hook(info);
                            if (status != 0) {
                                printf("Library %d: %s: Found mprotect, Failed to install plthook\n", library_count,
                                       lib_name);
                                trait->is_true = FALSE;
                                return 1; // abort
                            }
#else

                            printf("Library %d: %s: Found mprotect\n", library_count, lib_name);
                            trait->is_true = FALSE;
                            return 1; // abort
#endif
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
            if (strlen(info->dlpi_name) == 0) {
                // is main binary
                //sometimes the marker symbol is not in the dynamic symbol table, check if we find it in the static one
                return check_static_symbol_table_of_main_binary(trait);
                //will set trait->is_true appropriately
            } else {
                trait->is_true = FALSE;
                printf("Library: %s: DOES NOT have the Trait\n", lib_name);
                // found violation: abort
                return 1;
            }
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


void evaluate_trait(trait_handle_type trait) {
    assert(g_ptr_array_find(all_traits, trait, NULL));
    assert(!trait->is_evluated);
    printf("evaluate trait: %s\n", trait->options.name);

    // reset library_count
    library_count = 0;
    // check all libraries
    dl_iterate_phdr(&trait_evaluation_callback, trait);


#ifndef HOOK_DLOPEN
    if (trait->options.check_for_dlopen && trait->found_dlopen) {
        printf("Found Use of dlopen, cannot analyze trait, must assume it does not hold anymore after dlopen usage\n");
        assert(trait->is_true == false);
    }
    // #ifdef HOOK_DLOPEN : the hook will be installed while iterating over all libraries
#endif
#ifndef HOOK_MPROTECT
    if (trait->options.check_for_mprotect && trait->found_mprotect) {
        printf(
            "Found Use of mprotect , cannot analyze trait, must assume it does not hold anymore after mprotect usage\n");
        assert(trait->is_true == false);
    }
    // #ifdef HOOK_MPROTECT : the hook will be installed when iterating over all libraries
#endif
    trait->is_evluated = true;
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
    if (options->num_libraies_to_skip > 0) {
        handle->options.num_libraies_to_skip = options->num_libraies_to_skip;
        handle->options.libraies_to_skip = malloc(options->num_libraies_to_skip * sizeof(char *));
        for (unsigned int i = 0; i < options->num_libraies_to_skip; ++i) {
            char *new_buf = malloc(strlen(options->libraies_to_skip[i]) + 1); // +1 for null terminator
            strcpy(new_buf, options->libraies_to_skip[i]);
            handle->options.libraies_to_skip[i] = new_buf;
        }
    }

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
    if (trait->options.num_libraies_to_skip > 0) {
        for (unsigned int i = 0; i < trait->options.num_libraies_to_skip; ++i) {
            free(trait->options.libraies_to_skip[i]);
        }
        free(trait->options.libraies_to_skip);
    }

    free(trait);

    // if last trait removed: free all ressources
    if (all_traits->len == 0) {
        g_ptr_array_free(all_traits, true);
        all_traits = NULL;
    }
}

#ifdef HOOK_DLOPEN
void evaluate_trait_on_dlopen(void *trait_data, void *filename) {
    trait_handle_type trait = (trait_handle_type) trait_data;
    if (check_trait(trait)) {
        // if it was false from the beginning: nothing to do
        if (trait->options.check_for_dlopen) {
            trait->is_evluated = false;
            trait->is_true = false;
            // re-evaluate if still holds with added library
            evaluate_trait(trait);
        }
        if (!trait->is_true) {
            printf("Trait violation found after dlopen (%s not satisfied anymore after loading %s)\n",
                   trait->options.name, (char *) filename);
            printf("Aborting...\n");
            exit(EXIT_FAILURE);
        }
    }
}


__attribute((weak)) void *dlopen(const char *filename, int flag);

typedef void *(*dlopen_fnptr_t)(const char *, int);

dlopen_fnptr_t original_dlopen = NULL;

// Our replacement dlopen
static void *our_dlopen(const char *filename, int flags) {
    printf("Intercept Loading library: %s\n", filename);

    // Call original dlopen
    assert(original_dlopen!=NULL);
    void *handle = original_dlopen(filename, flags);

    if (handle) {
        // Load was successful, need to analyze if program with additional loaded library still satisfies all the traits
        g_ptr_array_foreach(all_traits, evaluate_trait_on_dlopen, filename);
    }

    return handle;
}


int install_dlopen_plt_hook(struct dl_phdr_info *info) {
    assert(dlopen != NULL);
    plthook_t *plthook;
    printf("Installing plthook for dlopen\n");
    if (original_dlopen == NULL) {
        original_dlopen = dlopen;
    }
    if (strlen(info->dlpi_name) ==0){
        // main binary
        //info->dlpi_addr may point to NULL but relocation may be done,
        // need default plthook_open to open main binary
        if (plthook_open(&plthook, NULL) != 0) {
            printf("plthook_open error: %s\n", plthook_error());
            return 1;
        }

    }
    else {
        if (plthook_open_by_address(&plthook, info->dlpi_addr) != 0) {
            printf("plthook_open error: %s\n", plthook_error());
            return 1;
        }
    }
    if (plthook_replace(plthook, "dlopen", (void *) our_dlopen, NULL) != 0) {
        printf("plthook_replace error: %s\n", plthook_error());
        plthook_close(plthook);
        return -1;
    }
    plthook_close(plthook);
    return 0; // success
}
#endif


#ifdef HOOK_MPROTECT

__attribute((weak)) int mprotect(void *addr, size_t size, int prot);

typedef int (*mprotect_fnptr_t)(void *, size_t, int);

mprotect_fnptr_t original_mprotect = NULL;

int our_mprotect(void *addr, size_t size, int prot) {
    if (prot & PROT_EXEC) {
        //check if there is still a trait around that requires protection from mprotect
        bool need_to_check_mprotect = false;
        for (unsigned int i = 0; i < all_traits->len; ++i) {
            trait_handle_type trait = all_traits->pdata[i];
            if (trait->options.check_for_mprotect) {
                need_to_check_mprotect = true;
                break;
            }
        }
        if (need_to_check_mprotect) {
            printf("Making Memory Executable could cause a Trait violation\n");
            printf("Aborting...\n");
            exit(EXIT_FAILURE);
        }
    }

    assert(original_mprotect != NULL);
    return original_mprotect(addr, size, prot);
}

int install_mprotect_plt_hook(struct dl_phdr_info *info) {
    assert(mprotect != NULL);
    plthook_t *plthook;
    printf("Installing plthook for mprotect\n");
    if (original_mprotect == NULL) {
        original_mprotect = mprotect;
    }
    if (plthook_open_by_address(&plthook, info->dlpi_addr) != 0) {
        printf("plthook_open error: %s\n", plthook_error());
        return 1;
    }
    if (plthook_replace(plthook, "mprotect", (void *) our_dlopen, NULL) != 0) {
        printf("plthook_replace error: %s\n", plthook_error());
        plthook_close(plthook);
        return -1;
    }
    plthook_close(plthook);
    return 0; // success
}


#endif
