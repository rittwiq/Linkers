#include<elf.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<stdio.h>
//#define _DEBUG_MODE_HASH_ON


#ifndef PARSE_ELF_C
#define PARSE_ELF_C
#include "elf_parser.c"
extern void symparse(FILE *file, Elf64_Shdr section);
extern int lnkIsValidELF(Elf64_Ehdr *header);
extern Elf64_Ehdr *lnkGetElfHeader(FILE *file);
extern Elf64_Shdr *lnkGetSectionHeadersP(FILE *file);
extern Elf64_Shdr *lnkGetSectionHeaders(Elf64_Ehdr *header, FILE *file);
extern Elf64_Shdr *lnkGetStringTable(Elf64_Shdr *section, int shnum);
#endif


#ifndef STORAGE_ALLOCATE_C
#define STORAGE_ALLOCATE_C
struct lnkrAddrAlloc {
	int start_text;
	int start_data;
	int start_bss;
	int start_sym_tab;
	int start_str_tab;
	int start_sh_str_tab;
	int start_sect;
	
	int end_text;
	int end_data;
	int end_bss;
	int end_sym_tab;
	int end_str_tab;
	int end_sh_str_tab;
	int end_sect;
	
	int ntext;
	int ndata;
	int nbss;
	int nsym;
	int nstr;
};
#define LNKR_TEXT_OFFSET 0xb0
#include "storage_allocate.c"
#include "address_allocate.c"
extern struct lnkrAddrAlloc lnkrAllocateStorage(Elf64_Shdr *section, int shnum);
extern struct lnkrAddrAlloc lnkrAllocateAddress(Elf64_Shdr *, int, FILE *);
#endif


#if 0
#ifndef HASH_ELF_C
#define HASH_ELF_C
#define NBUCKET 100
struct _sym {
	struct _sym *next;
	int fullhash;
	struct Elf64_Sym *entry;
};
#include "hash.c"
void lnkInsertHash(Elf64_Sym *,struct _sym **);
struct _sym **lnkCreateHash(int size);
#endif
#endif

#ifndef SYMBOL_MANAGEMENT
#define SYMBOL_MANAGEMENT
//void insert(struct _glosym **symbol_table, Elf64_Sym symbol, char *name);
//void first_pass(char **file_names, int n, struct _glosym ***global_symbol_tabl, struct _module_entry **module_tabl);
struct _module_entry {
	FILE *file;
	char *filename;
	char *local_sym_name;
	Elf64_Ehdr *header;
	int symseg_offset;
	Elf64_Sym *symbols;
	int string_size;
	Elf64_Shdr *strings;
	struct relocation_info *textrel;
	struct relocation_info *datarel;
	struct lnkrAddrAlloc *allocated_address;
};
struct _glosym {
	struct _glosym *link;
	char *name;
	long value;
	struct nlist *refs;
	int max_common_size;
	char defined;
	char referenced;
	unsigned char multiply_defined;
};
#include "symbol_management.c"
#endif
