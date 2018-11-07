#include<malloc.h>
//#include "lnkr.h"
/**
Storage Allocation

* Allocate storage based on sizes.

* To accomodate space for stack and heap, text section starts at LNKR_TEXT_OFFSET usually 0x100 after program header

Input: All the sections from all files
Output: struct lnkrAddrAlloc Containing all the addresses to allocate
Algorithm: 
	data <- 0
	text <- 0
	bss  <- 0
	traverse through the sections
	if section.sh_type == SHT_PROGBITS AND section[i].sh_flags&1 (ie its writeable)
		it belongs to data
		data <- data + section size
	else if section.sh_type == SHT_PROGBITS AND section[i].sh_flags&4 (ie its executable instruction)
		it belongs to text
		text <- text + section size
	else if section.sh_type == SHT_NOBITS (ie its bss)
		it belongs to bss
		bss <- bss + section size
	
	text section always begins at 0xb0
	data section always succeeds text
	and bss at the end 
	
	WILL STORE SECTION data, LOCAL, GLOBAL etc 
	symbol_table is after data // bss is not added to the executable file
	as we add 3 more entries to the symbol table, it ends after the (n+3) * size of SYMBOL TABLE
	
	string table is after symbol table
	find the size of the string table and add 24 ( the three added entries are __bss_start\0(12) , _edata\0(7) , _end(5) )
	
	section header string table is after string table
	As we do not need the rela.text in the executable, we subtract 11
	
	sections are finally 5 places after section header string table
*/
struct lnkrAddrAlloc lnkrAllocateAddress(Elf64_Shdr *section, int shnum, FILE *input) {
	// Allocating an address to the text, data and bss section
	
	struct lnkrAddrAlloc addr;
	if(section==NULL) return addr;
	int i;
	int text=0; // .text will be stored from addr.text[0]
	int data=0; // .data will be stored after .text
	int bss=0; // .bss will be stored in the end
	addr.ntext=0;
	addr.ndata=0;
	addr.nbss =0;
	/** Getting size of each required section*/
	for(i=0;i<shnum;i++) {
		if(section[i].sh_type==SHT_PROGBITS && section[i].sh_flags&1)
			data+=section[i].sh_size;
		else if(section[i].sh_type==SHT_PROGBITS && section[i].sh_flags&4)
			text+=section[i].sh_size;
		else if(section[i].sh_type==SHT_NOBITS) {
			bss+=section[i].sh_size*8;
		}
	}
	addr.ntext=text;
	addr.ndata=data;
	addr.nbss =bss;
	addr.start_text=LNKR_TEXT_OFFSET;
	addr.end_text=addr.start_text; 
	addr.end_text+=text;// assigning space based on size
	addr.start_data=addr.end_text+1;
	addr.end_data=addr.start_data;
	addr.end_data+=data;
	addr.start_bss=addr.end_data;
	addr.end_bss=addr.start_bss;
	addr.end_bss+=bss;
	
	// Symbol Table is just after the bss
	addr.start_sym_tab=addr.end_data+3;
	int sym_ent;
	int shstridx=lnkGetStringTableP(section, shnum, 0);
	int symidx=lnkGetSymTableSection(section, shnum, 0);
	Elf64_Sym *local_sym=lnkGetSymTab(section, shnum, input, &sym_ent);
	int stridx=section[symidx].sh_link;
	addr.end_sym_tab=addr.start_sym_tab+sizeof(Elf64_Sym)*(sym_ent+3);
	addr.start_str_tab=addr.end_sym_tab;
	int strsize=0;
	for(i=0;i<sym_ent;i++) {
		char *sym_name=(char *)malloc(20*sizeof(char));
		if(fseek(input, section[stridx].sh_offset+local_sym[i].st_name, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
		fread(sym_name, 1, 20*sizeof(char), input);
		if(sym_name[0]!='\0') {
			printf("%s\n", sym_name);
			strsize+=strlen(sym_name);
		}
	}
	
	//int strsize=section[stridx].sh_size;
	addr.end_str_tab=addr.start_str_tab+strsize+24;
	addr.start_sh_str_tab=addr.end_str_tab;
	addr.end_sh_str_tab=addr.start_sh_str_tab+section[shstridx].sh_size-11;
	addr.start_sect=addr.end_sh_str_tab+5;
	
	return addr;
}
