#include<malloc.h>
#include "lnkr.h"
#include <stdio.h>
void lnkHelp() {
	printf("Usage: lnkr elf-file(s)\n");
}
void lnkInit(FILE **file, int N) {
	int i, j, z=0, x=0;
	Elf64_Shdr *total_section=(Elf64_Shdr *)malloc(200*sizeof(Elf64_Shdr));
	for(i=0;i<N;i++) {
		Elf64_Ehdr *header=lnkGetElfHeader(file[i]);
		Elf64_Shdr *section=lnkGetSectionHeaders(header, file[i]);
		Elf64_Sym *symbol_tab=(Elf64_Sym *)malloc(sizeof(Elf64_Sym));
		struct _sym **module_table=lnkCreateHash(NBUCKET);
		symbol_tab=lnkGetSymTab(section, header->e_shnum, file[i], &x);
		for(j=0;j<x;j++) 
			lnkInsertHash(symbol_tab, module_table);
		for(j=0;j<header->e_shnum;j++)
			total_section[j+z]=section[j];
		z+=header->e_shnum;
	}
	struct lnkrAddrAlloc addr=lnkrAllocateStorage(total_section, z);
}
int main(int argc, char **argv) {
	if(argc<2) {
		lnkHelp();
		return -1;
	}
	int i;
	FILE **file=(FILE **)malloc((argc-1)*sizeof(FILE *));
	for(i=0;i<argc-1;i++) {
		file[i]=fopen(argv[i+1], "rb");
		if(!file[i]) {
			perror("fopen:");
			exit(-1);
		}
	}
	lnkInit(file, argc-1);
}
