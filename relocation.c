#include<stdio.h>
#include "lnkr.h"
#include <sys/stat.h>
char *section_type[]={"NULL", "PROGBITS", "SYMTAB", "STRTAB", "RELA", "HASH", "DYNAMIC", "NOTE", "NOBITS", "REL", "SHLIB", "DYNSYM", "LOPROC", "HIPROC", "LOUSER", "HIUSER"};
int str_size=0, str_off=0;
void swap(Elf64_Shdr *section, int shnum, Elf64_Sym *local_sym, int sym_ent, char **sym_names, char **sh_name, int temp, int type, int flags) {
	int i, j;
	for(i=0;i<shnum;i++) {
		if(section[i].sh_type==type && section[i].sh_flags&flags) {
			Elf64_Shdr tmp=section[i];		// SWAPPING SECTIONS
			section[i]=section[temp];
			section[temp]=tmp;
			
			char *shstr=(char *)malloc(strlen(sh_name[i])*sizeof(char)); // SWAPPING ENTRIES IN SECTION HEADERS STRING TABLE
			strcpy(shstr, sh_name[i]);
			strcpy(sh_name[i], sh_name[temp]);
			strcpy(sh_name[temp], shstr);
			
			int section_sym=0;				// SWAPPING SECTION ENTRIES IN LOCAL SYMBOL TABLE
			for(j=0;j<sym_ent;j++) {
				if(local_sym[j].st_info==STT_SECTION && local_sym[j].st_shndx==i)
					section_sym=j;
				if(local_sym[j].st_shndx==i) 
					local_sym[j].st_shndx=100;// 100 has no importance just need a large number which won't affect symbol table
			}
			for(j=0;j<sym_ent;j++) {
				if(local_sym[j].st_shndx==temp) 
					local_sym[j].st_shndx=i;
			}
			for(j=0;j<sym_ent;j++) {
				if(local_sym[j].st_shndx==100) 
					local_sym[j].st_shndx=temp;
			}
			Elf64_Sym a=local_sym[section_sym];
			local_sym[section_sym]=local_sym[temp];
			local_sym[temp]=a;
			
			char *str=(char *)malloc(strlen(sym_names[section_sym])*sizeof(char)); // SWAPPING ENTRIES IN STRING TABLE
			strcpy(str, sym_names[section_sym]);
			strcpy(sym_names[section_sym], sym_names[temp]);
			strcpy(sym_names[temp], str);
			return;
		}
	}
}

/*
Relocation
Coping data from the object file into the loadable object file
*/

int reloc(int argc, char **argv) {
	FILE *input =fopen(argv[2], "rb");		// INPUT OBJECT FILE
	FILE *output=fopen(argv[1], "wb");		// OUTPUT LOADABLE OBJECT FILE
	if(fseek(input, 0, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
	Elf64_Ehdr *elf_headr=lnkGetElfHeader(input);	// Extracting Elf header
	Elf64_Shdr *section=lnkGetSectionHeaders(elf_headr, input);	// Extracting Section headers STORES IN SECTION
	struct lnkrAddrAlloc addr= lnkrAllocateAddress(section, elf_headr->e_shnum, input); // Storage/ADDRESS Alloaction SEE: address_allocate.c
	int i, shnum=elf_headr->e_shnum, sym_ent, j;
	Elf64_Sym *local_sym=lnkGetSymTab(section, shnum, input, &sym_ent); // Extracts symbol table
	
	int shstridx=lnkGetStringTableP(section, shnum, 0); // GETS THE INDEX FOR THE SECTION HEADER STRING TABLE IN THE SECTION
	int symidx=lnkGetSymTableSection(section, shnum, 0); // GETS THE INDEX FOR THE SECTION HEADER STRING TABLE
	int stridx=section[symidx].sh_link;// THE STRING TABLE FOR SYMBOL TABLE IS LINKED, CAN BE EXTRACTED FROM SECTION HEADERS
	
	char **sym_name=(char **)malloc((3+sym_ent)*sizeof(char *)); // EXTRACTING SYMBOL NAMES FROM INPUT FILES TO BE REARRANGED 
	for(i=0;i<sym_ent;i++) {
		char *temp=(char *)malloc(20*sizeof(char));
		if(fseek(input, section[stridx].sh_offset+local_sym[i].st_name, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
		fread(temp, 1, 20*sizeof(char), input);
		sym_name[i]=(char *)malloc(strlen(temp)*sizeof(char));
		strcpy(sym_name[i], temp);
	}
	
	sym_name[i]=(char *)malloc(12*sizeof(char)); // ADDING THE THREE EXTRA ENTRIES TO SYMBOL TABLE
	strcpy(sym_name[i]," __bss_start");
	sym_name[i+1]=(char *)malloc(7*sizeof(char));
	strcpy(sym_name[i+1], "_edata");
	sym_name[i+2]=(char *)malloc(5*sizeof(char));
	strcpy(sym_name[i+2], "_end");
	
	
	char **sh_name=(char **)malloc((shnum)*sizeof(char *)); // EXTRACTING SYMBOL NAMES FROM INPUT FILES TO BE REARRANGED 
	for(i=0;i<shnum;i++) {
		if(!(section[i].sh_type==SHT_RELA || section[i].sh_type==SHT_REL)) { // AS WE DO NOT COPY ANY RELOCATIONAL INFORMATION WE IGNORE
			char *temp=(char *)malloc(20*sizeof(char));
			if(fseek(input, section[shstridx].sh_offset+section[i].sh_name, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
			fread(temp, 1, 20*sizeof(char), input);
			sh_name[i]=(char *)malloc(strlen(temp)*sizeof(char));
			strcpy(sh_name[i], temp);
		}
	}
	
	
	swap(section, shnum, local_sym, sym_ent, sym_name, sh_name, 1 , SHT_PROGBITS, SHF_EXECINSTR);	// THE TEXT SECTION MUST BE AT 1
	swap(section, shnum, local_sym, sym_ent, sym_name, sh_name, 2 , SHT_PROGBITS, SHF_WRITE);	// THE DATA SECTION MUST BE AT 2
	swap(section, shnum, local_sym, sym_ent, sym_name, sh_name, 3 , SHT_NOBITS, 1);			// THE BSS  SECTION MUST BE AT 3
	
	unsigned char *text=(char *)malloc(sizeof(char) * section[1].sh_size);		// COPY THE TEXT SECTION
	if(fseek(input, section[1].sh_offset, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
	for(i=0;i<section[1].sh_size;i++)
		fread(&text[i], 1, sizeof(char), input);
	
	printf("Text Segment: \n");
	for(i=0;i<section[1].sh_size;i++) {
		printf("%02x", text[i]);
		if(i%4==0 && i) printf(" ");
		if(i%16==0 && i) printf("\n");
	}
	printf("\n");
	
	unsigned char *data=(char *)malloc(sizeof(char) * section[2].sh_size);		// COPY THE DATA SECTION
	if(fseek(input, section[2].sh_offset, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
	for(i=0;i<section[2].sh_size;i++)
		fread(&data[i], 1, sizeof(char), input);
	
	printf("Data Segment: \n");
	for(i=0;i<section[2].sh_size;i++) {
		printf("%02x", data[i]);
		if(i%4==0 && i) printf(" ");
		if(i%16==0 && i) printf("\n");
	}
	printf("\n");
	
	if(section[3].sh_type==SHT_NOBITS) {		// COPY THE BSS SECTION |  BSS NEED NOT BE IN THE OBJECT FILE
		unsigned char *bss=(char *)malloc(sizeof(char) * section[3].sh_size);
		if(fseek(input, section[3].sh_offset, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
		for(i=0;i<section[3].sh_size;i++)
			fread(&bss[i], 1, sizeof(char), input);
	
		printf("BSS Segment: \n");
		for(i=0;i<section[3].sh_size;i++) {
			printf("%02x", bss[i]);
			if(i%4==0 && i) printf(" ");
			if(i%16==0 && i) printf("\n");
		}
		printf("\n");
	}
	
	/**
	Relocation_Section:
		This will link the address of the entry in the data/ bss  section into 	the text section
	*/
	int sz=section[shnum-1].sh_size/section[shnum-1].sh_entsize;	// WE NEED TO MAKE CHANGES ON THE TEXT SECTION BEFORE SENDING
	Elf64_Rela *rela=lnkGetRela(section[shnum-1], shnum, input);
	for(i=0;i<sz;i++) {
		char res=0;
		char vr=0, b=0;
		if( ELF64_R_TYPE (rela[i].r_info)==11 || ELF64_R_TYPE (rela[i].r_info)==10 ) {	// 32 BIT AND 32 BIT SHORT RELOCATION TYPE
			if( ELF64_R_SYM(rela[i].r_info)==3) {
				res=addr.start_data+rela[i].r_addend; 	// res address is the start of the DATA section with the offset provided 
				vr=0x60;				// by addend ; vr is the virtual memory address
			}
			else if(ELF64_R_SYM(rela[i].r_info)==4) {
				res=addr.start_bss+rela[i].r_addend; 	// res address is the start of the BSS section with the offset provided 
				vr=0x60;				// by addend ; vr is the virtual memory address
			}
		}
		else {								// 64 BIT RELOCATION TYPE
			if( ELF64_R_SYM(rela[i].r_info)==2) {
				res=addr.start_data+rela[i].r_addend;	// res address is the start of the DATA section with the offset provided 
				vr=0x60;				// by addend ; vr is the virtual memory address
			}
			else if(ELF64_R_SYM(rela[i].r_info)==3) {
				res=addr.start_bss+rela[i].r_addend; 	// res address is the start of the BSS section with the offset provided 
				vr=0x60;				// by addend ; vr is the virtual memory address
			}
		}
		printf("Relocate %x  in %d to %x.\n", res, ELF64_R_SYM(rela[i].r_info), addr.start_text+rela[i].r_offset);
		printf("Changing %x  to %x.\n", text[rela[i].r_offset], res);
		printf("Changing %x  to %x.\n", text[rela[i].r_offset+2], vr);
		text[rela[i].r_offset]=res;
		if(ELF64_R_SYM(rela[i].r_info)==4 && ELF64_R_TYPE (rela[i].r_info)==11 || ELF64_R_TYPE (rela[i].r_info)==10 )
			text[rela[i].r_offset+1]=0x01;		// IF IT IS OF TYPE 32 BIT RELOCATION BSS MUST HAVE 1 at offset + 1
		text[rela[i].r_offset+2]=vr;
	}
	

	printf("Modified Text Segment: \n");
	for(i=0;i<section[1].sh_size;i++) {
		printf("%02x", text[i]);
		if(i%4==0 && i) printf(" ");
		if(i%16==0 && i) printf("\n");
	}
	printf("\n");
	
	
	section[1].sh_offset=addr.start_text;		// CHANGING THE SECTION HEADER OFFSET TO WHAT WE CALCULATED IN address_allocate.c
	section[1].sh_addr=addr.start_text+0x0000000000400000;	// CHANGING THE SECTION HEADER  VIRTUAL ADDR TO WHAT WE CALCULATED IN address_allocate.c
	section[1].sh_size=addr.ntext;		// CHANGING THE SECTION HEADER SIZE TO WHAT WE CALCULATED IN address_allocate.c
	
	section[2].sh_offset=addr.start_data;		// CHANGING THE SECTION HEADER OFFSET TO WHAT WE CALCULATED IN address_allocate.c
	section[2].sh_addr=addr.start_data+0x0000000000600000;	// CHANGING THE SECTION HEADER VIRTUAL ADDR TO WHAT WE CALCULATED IN address_allocate.c
	section[2].sh_size=addr.ndata;		// CHANGING THE SECTION HEADER SIZE TO WHAT WE CALCULATED IN address_allocate.c
	if(section[3].sh_type&SHT_NOBITS) {
		section[3].sh_offset=addr.start_bss;
		section[3].sh_addr=addr.start_bss+0x0000000000600000;
		section[3].sh_size=addr.nbss;
	}
	
	
	
	for(i=0;i<sym_ent;i++) {
		if(local_sym[i].st_shndx==1) local_sym[i].st_value=addr.start_text+0x400000; // CHANGING THE  VIRTUAL ADDRESS OF ALL TEXT SECTION ENTRIES
		else if(local_sym[i].st_shndx==2) local_sym[i].st_value=addr.start_data+0x600000; // CHANGING THE  VIRTUAL ADDRESS OF ALL DATA SECTION ENTRIES
		else if(local_sym[i].st_shndx==3) local_sym[i].st_value=addr.start_bss+0x600000; // CHANGING THE  VIRTUAL ADDRESS OF ALL BSS SECTION ENTRIES
	}
	// ENTRY DENOTING START OF BSS
	Elf64_Sym start_bss;
	start_bss.st_value=addr.start_bss+0x600000; // CALCULATED DURING STORAGE ALLOCATION
	start_bss.st_info=STB_GLOBAL;
	start_bss.st_other=STV_DEFAULT;
	start_bss.st_size=0;
	local_sym[sym_ent++]=start_bss;
	
	
	// ENTRY DENOTING END OF DATA
	Elf64_Sym end_data;
	end_data.st_value=addr.end_data+0x600000; // CALCULATED DURING STORAGE ALLOCATION
	end_data.st_shndx=2;
	end_data.st_info=STB_GLOBAL;
	end_data.st_other=STV_DEFAULT;
	end_data.st_size=0;
	local_sym[sym_ent++]=end_data;
	
	
	// ENTRY DENOTING END OF BSS
	Elf64_Sym end;
	end.st_value=addr.end_bss+0x600000+3;
	if(local_sym[3].st_shndx==3) end.st_shndx=3;
	else end.st_shndx=2;
	end.st_info=STB_GLOBAL;
	end.st_other=STV_DEFAULT;
	end.st_size=0;
	local_sym[sym_ent++]=end;
	
	// Changing section entries
	section[shstridx].sh_offset=addr.start_sh_str_tab;	// Calculated during address allocation of section header string table allocation
	section[shstridx].sh_size=addr.end_sh_str_tab - addr.start_sh_str_tab;
	
	section[symidx].sh_offset=addr.start_sym_tab;	// Calculated during address allocation of symbol table allocation
	section[symidx].sh_size=addr.end_sym_tab - addr.start_sym_tab;
	section[symidx].sh_entsize=0x18;
	
	
	section[stridx].sh_offset=addr.start_str_tab;	// Calculated during address allocation of string table allocation
	section[stridx].sh_size=addr.end_str_tab - addr.start_str_tab;
	
	// allocating the name nalue for the symbol table
	for(i=0;i<sym_ent;i++) {
		if(strlen(sym_name[i])) break;
		local_sym[i].st_name=0;
	}
	local_sym[i].st_name=1;
	i++;
	for(;i<sym_ent;i++)
		local_sym[i].st_name=strlen(sym_name[i-1])+local_sym[i-1].st_name+1;
	for(i=0;i<sym_ent;i++) {
		local_sym[i].st_name+=2;
	}
	
	shnum--;	// we do not need rela.text
	
	// allocating the name nalue for the section table
	for(i=0;i<shnum;i++) {
		if(strlen(sh_name[i])) break;
		section[i].sh_name=0;
	}
	section[i].sh_name=1;
	i++;
	for(;i<shnum;i++) 
		section[i].sh_name=strlen(sh_name[i-1])+section[i-1].sh_name+1;
	
	//for(i=0;i<shnum;i++) printf("%p %s\n", section[i].sh_name, sh_name[i]);
	
	printf("Text Offset : %016x\n", addr.start_text);	// Writing the text
	if(fseek(output, addr.start_text, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
	for(i=0;i<addr.ntext;i++) {
		char b;
//		fread(&b, 1, sizeof(char), input);
		fwrite(&text[i], 1, sizeof(unsigned char), output);
	}
	
	printf("Data Offset : %016x\n", addr.start_data);	// Writing the data
	if(fseek(output, addr.start_data, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
	for(i=0;i<addr.ndata;i++) {
		char b;
		//fread(&b, 1, sizeof(char), input);
		fwrite(&data[i], 1, sizeof(char), output);
	}

	printf("Symbol Table Offset : %016x\n", addr.start_sym_tab);	// Writing the symbol table
	if(fseek(output, addr.start_sym_tab, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
	for(i=0;i<sym_ent;i++) 
		fwrite(&local_sym[i], 1, sizeof(Elf64_Sym), output);
	
	printf("String Table Offset : %016x\n", addr.start_str_tab);	// Writing the string table
	if(fseek(output, addr.start_str_tab, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
	for(i=0;i<sym_ent;i++) {
		int j=0;
		do
			fwrite(&sym_name[i][j], 1, sizeof(char), output);
		while(sym_name[i][j++]);
	}
	//section[stridx].sh_size=ftell(output) - addr.start_str_tab;
	
	printf("Section Header Offset : %016x\n", addr.start_sh_str_tab);	// Writing the section header string table
	if(fseek(output, addr.start_sh_str_tab, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
	for(i=0;i<shnum;i++) {
		int j=0;
		do
			fwrite(&sh_name[i][j], 1, sizeof(char), output);
		while(sh_name[i][j++]);
	}


	printf("Section Offset : %016x\n", addr.start_sect);	// Writing the section table
	if(fseek(output, addr.start_sect, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
	for(i=0;i<shnum;i++)
		fwrite(&section[i], 1, sizeof(Elf64_Shdr), output);
		//printf
	
	/**
		The loader concerns itself on the program headers
		
		The program header tells the loader what part signifies what
	
	*/
	
	Elf64_Phdr program_header;
	program_header.p_type=PT_LOAD;	// Tells the loader to load the text section
	program_header.p_flags=PF_R | PF_X ;
	program_header.p_offset=0;
	program_header.p_vaddr=0x0000000000400000;
	program_header.p_paddr=0x0000000000400000;
	program_header.p_align=0x200000;
	program_header.p_memsz=addr.start_data-1;
	program_header.p_filesz=addr.start_data-1;
	
//	printf("%016x\n", program_header.p_memsz);
	
	Elf64_Phdr program_header2;
	program_header2.p_type=PT_LOAD;	// Tells the loader to load the data section
	program_header2.p_flags=PF_R | PF_W;
	program_header2.p_vaddr=0x0000000000600000+addr.start_data;
	program_header2.p_paddr=0x0000000000600000+addr.start_data;
	program_header2.p_align=0x200000;
	program_header2.p_offset=addr.start_data;
	program_header2.p_memsz=addr.end_data-addr.start_data;
	program_header2.p_filesz=addr.end_data-addr.start_data;
	if(fseek(output, 64, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
	
	fwrite(&program_header, 1, sizeof(Elf64_Phdr), output);	// writing the program headers
	fwrite(&program_header2, 1, sizeof(Elf64_Phdr), output);
	
	Elf64_Ehdr headr=*elf_headr;
	headr.e_phoff=64;
	headr.e_shoff=addr.start_sect;
	headr.e_type=ET_EXEC;
	headr.e_phentsize=56;
	headr.e_phnum=2;
	headr.e_shentsize=64;
	headr.e_shstrndx=shstridx;
	headr.e_ehsize=64;
	headr.e_shnum=shnum;
	headr.e_entry=0x4000b0;
	headr.e_flags=0;
	headr.e_flags=0;
	if(fseek(output, 0, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
	fwrite(&headr, 1, sizeof(Elf64_Ehdr), output);	// finally we write the elf header
	
	chmod(argv[1], 0777);		// making the output file executable
	return 1;
}
void help() {
	printf("Usage: ./relocation <.out file/ OUTPUT FILE> <.o file / OBJECT FILE>\n");
}
int main(int argc, char **argv) {
	if(argc<3) {
		help();
		printf("No filename mentioned\n");
		exit(-1);
	}
	reloc(argc, argv);
}
