void symparse(FILE *file, Elf64_Shdr section) {
	if(fseek(file, section.sh_offset, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
	Elf64_Sym symheader;
	int size_=0, i;
	fread(&symheader, 1, sizeof(section.sh_entsize), file);
	printf("name:%d\n", symheader.st_name);
	printf("size:%d\n", symheader.st_size);
}
int lnkIsValidELF(Elf64_Ehdr *header) {
	if(header->e_ident[0] != 0x7f && header->e_ident[1] != 'E' && header->e_ident[2] != 'L' && header->e_ident[3] != 'F') {
		printf("Given object file is not an ELF file: \n");
		return 0;
	}
	if(header->e_ident[4] == ELFCLASSNONE && header->e_ident[4] == ELFCLASS32) {
		printf("Given object file class not supported: lnkr works only for 64bit machine\n");
		return 0;
	}
	if(header->e_ident[5] == ELFDATANONE) {
		printf("Given object file data not supported: Unknown data format\n");
		return 0;
	}
	if(header->e_ident[6] == EV_NONE) {
		printf("ELF version not supported: Invalid Version\n");
		return 0;
	}
	return 1;
}
Elf64_Ehdr *lnkGetElfHeader(FILE *file){
	Elf64_Ehdr *header=malloc(sizeof(Elf64_Ehdr));
	if(!file) {
		perror("fopen error:");
		exit(-1);
	}
	fread(header, 1, sizeof(Elf64_Ehdr), file);
	if(lnkIsValidELF(header)) 
		return header;
	return NULL;
}
Elf64_Shdr *lnkGetSectionHeadersP(FILE *file) {
	Elf64_Ehdr *header=lnkGetElfHeader(file);
	if(header==NULL) return NULL;
	int i;
	Elf64_Shdr *section=malloc(header->e_shnum*sizeof(Elf64_Shdr));
	if(fseek(file, header->e_shoff, SEEK_SET)==-1) {perror("fseek error at getSectionHeader:"); exit(-1);}
	for(i=0;i<header->e_shnum;i++)
		if(fread(&section[i], 1, sizeof(section[i]), file)==-1) {
			perror("ELF file corrupt:");
			return NULL;
		}
	return section;
}
Elf64_Shdr *lnkGetSectionHeaders(Elf64_Ehdr *header, FILE *file) {
	if(header==NULL) return NULL;
	int i;
	Elf64_Shdr *section=malloc(header->e_shnum*sizeof(Elf64_Shdr));
	if(fseek(file, header->e_shoff, SEEK_SET)==-1) {perror("fseek error at getSectionHeader:"); exit(-1);}
	for(i=0;i<header->e_shnum;i++)
		if(fread(&section[i], 1, sizeof(section[i]), file)==-1) {
			perror("ELF file corrupt:");
			return NULL;
		}
	return section;
}
Elf64_Shdr *lnkGetStringTable(Elf64_Shdr *section, int shnum) {
	if(section==NULL) return NULL;
	int i;
	for(i=0;i<shnum;i++) {
		if(section[i].sh_type==SHT_STRTAB)
			return &section[i];
	}
}
int lnkGetStringTableP(Elf64_Shdr *section, int shnum, int idx) {
	if(section==NULL) return -1;
	int i;
	for(i=idx;i<shnum;i++) {
		if(section[i].sh_type==SHT_STRTAB)
			return i;
	}
	return -1;
}
int lnkGetSymTableSection(Elf64_Shdr *section, int shnum, int idx) {
	if(section==NULL) return -1;
	int i;
	for(i=idx;i<shnum;i++) {
		if(section[i].sh_type==SHT_SYMTAB || section[i].sh_type==SHT_DYNSYM) 
			return i;
	}
	return -1;
}
int lnkGetStrTableIdx(Elf64_Shdr *section, int shnum) {
	if(section==NULL) return -1;
	int i;
	for(i=0;i<shnum;i++) {
		if(section[i].sh_type==SHT_STRTAB && section[i-1].sh_type==SHT_SYMTAB) 
			return i;
	}
	return -1;
}

Elf64_Sym *lnkGetSymTab(Elf64_Shdr *section, int shnum, FILE *file, int *size) {
	int i=0, j=0, k;
	Elf64_Sym *per_file_symbol_tab=(Elf64_Sym *)malloc(sizeof(Elf64_Sym)*1000);
	while(i!=-1) {
		int idx=lnkGetSymTableSection(section, shnum, i+1);
		if(idx==-1) break;
		i=idx;
		if(fseek(file, section[i].sh_offset, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
		for(k=0;k<section[i].sh_size/24;k++) {;
			if(fread(&per_file_symbol_tab[j++], 1, sizeof(Elf64_Sym), file)==-1) {
				perror("ELF file corrupt:");
				exit(-2);
			}
		}
	}
	*size=j;
	return per_file_symbol_tab;
}
Elf64_Rela *lnkGetRela(Elf64_Shdr section, int shnum, FILE *file) {
	if(!file) {
		perror("fopen error:");
		exit(-1);
	}
	
	int sz=section.sh_size/section.sh_entsize, i;
	Elf64_Rela *rela=(Elf64_Rela *)malloc(sz*sizeof(Elf64_Rela));
	if(fseek(file, section.sh_offset, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
	for(i=0;i<sz;i++) {
		if(fread(&rela[i], 1, sizeof(Elf64_Rela), file)==-1) {
			perror("ELF file corrupt:");
			return NULL;
		}
		printf("REL: %p %d %d %d\n", rela[i].r_offset, ELF64_R_TYPE(rela[i].r_info), ELF64_R_SYM(rela[i].r_info), rela[i].r_addend);
	}
	return rela;
}
