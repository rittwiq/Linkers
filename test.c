#include "lnkr.h"
int main(int argc, char **argv) {
	if(argc!=2) {
		return -1;
	}
	FILE *file=fopen(argv[1], "rb");
	if(!file) {
		perror("fopen:");
		exit(-1);
	}
	int i;
	Elf64_Ehdr *header=lnkGetElfHeader(file);
	Elf64_Shdr *section=lnkGetSectionHeaders(header, file);
	struct lnkrAddrAlloc addr= lnkrAllocateAddress(section, header->e_shnum, file);
	printf("%7s %7s %7s %7s %7s %s\n", "text","data", "bss","dec", "hex","filename");
	printf("%7d %7d %7d %7d %7x %s\n", addr.ntext, addr.ndata, addr.nbss, addr.nbss+addr.ndata+addr.ntext, addr.nbss+addr.ndata+addr.ntext, argv[1]);
	printf("%07x %07x %07x %0d %7x %s\n", addr.start_text, addr.start_data, addr.start_bss, addr.nbss+addr.ndata+addr.ntext, addr.nbss+addr.ndata+addr.ntext, argv[1]);
	printf("\n\nRelocation Info:\n");
	printf("%7s %7s %7s %7s %7s %7s %s\n", ".text", ".data" , ".bss", ".symtab" , ".strtab", ".shstrtab","filename");
	printf("%07x %07x %07x %07x %07x %07x %s\n", addr.start_text, addr.start_data, addr.start_bss, addr.start_sym_tab, addr.start_str_tab, addr.start_sh_str_tab, argv[1]);
	
	printf("Section  Headers start from: %d\n", addr.start_sect);
/*	printf(".text section\n");
	printf("%d\n", addr.end_text-addr.start_text);
	printf(".data section\n");
	printf("%d\n", addr.end_data-addr.start_data);
	printf(".bss  section\n");
	printf("%d\n", addr.end_bss-addr.start_bss);*/
}
