#include<malloc.h>
#define STORAGE_ALLOCATE_C
//#include "lnkr.h"
/**
Storage Allocation

* Allocate storage based on sizes.

* To accomodate space for stack and heap, text section starts at LNKR_TEXT_OFFSET usually 0x100 after program header

*/
struct lnkrAddrAlloc lnkrAllocateStorage(Elf64_Shdr *section, int shnum) {
	// Allocating an address to the text and data section
	
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
	
	return addr;
}
