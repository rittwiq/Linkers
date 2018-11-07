void insert(struct _glosym **symbol_table, Elf64_Sym symbol, char *name) {
	struct _glosym *node=symbol_table[name[1]+name[0]];
//	printf("%s\n", name);
	if(node!=NULL) {
		while(node->link!=NULL)
			node=node->link;
		if(strcmp(name, node->name)==0) {
			if(symbol.st_size>node->value)
				return;
			node->referenced++;
			if(node->defined==0)
				node->defined=symbol.st_shndx;
			else 
				node->multiply_defined++;
			return;
		}
	}
	struct _glosym *entry=(struct _glosym *)malloc(sizeof(struct _glosym));
	entry->name=(char *)malloc(20*sizeof(char));
	strcpy(entry->name, name);
	entry->value=name[1]+name[0];
	entry->max_common_size=symbol.st_size;
	entry->defined=symbol.st_shndx;
	entry->referenced=1;
	entry->multiply_defined=1;
	if(node==NULL) {
		symbol_table[name[1]+name[0]]=(struct _glosym *)malloc(sizeof(struct _glosym));
		symbol_table[name[1]+name[0]]=entry;
	}
	else
		node->link=entry;
}
int first_pass(char **file_names, int n, struct _glosym ***global_symbol_tabl, struct _module_entry **module_tabl) {
	FILE *file[n];
	int i, j, k=0;
	struct _glosym **global_symbol_table = (struct _glosym **)malloc(256*sizeof(struct _glosym *));
	struct _module_entry *module_table = (struct _module_entry *)malloc(1028*sizeof(struct _module_entry));
	for(i=0;i<n;i++) {
		file[i]=fopen(file_names[i], "rb");
		FILE *fp2=fdopen(dup(fileno(file[i])), "rb");
		if(!file[i]) { perror("fopen:"); exit(-1); }
		Elf64_Ehdr *header=lnkGetElfHeader(file[i]);
		Elf64_Shdr *section=lnkGetSectionHeaders(header, file[i]);
		//str_idx=lnkGetStringTableP(section, header->e_shnum, str_idx+1);
		int idx=0;
		while(idx!=-1) {
			idx=lnkGetSymTableSection(section, header->e_shnum, idx+1);
			int str_idx =lnkGetStringTableP(section, header->e_shnum, idx);
			int sym_num=section[idx].sh_size/24;
		//	printf("%d\n", sym_num);
			Elf64_Sym symbol_tab[sym_num];
			struct lnkrAddrAlloc addr= lnkrAllocateStorage(section, header->e_shnum);
			if(fseek(file[i], section[idx].sh_offset, SEEK_SET)==-1) {perror("fseek:"); exit(-1);}
			for(j=0;j<sym_num;j++) {
				if(fread(&symbol_tab[j], 1, sizeof(Elf64_Sym), file[i])==-1) { perror("ELF file corrupt:"); exit(-2); }
				if(ELF64_ST_BIND(symbol_tab[j].st_info)==STB_GLOBAL) {
					char name[20];
					if(fseek(fp2, symbol_tab[j].st_name+section[str_idx].sh_offset, SEEK_SET)==-1) {perror("fseek:"); exit(-2);}
					if(fread(&name, 1, 20, fp2)==-1) { perror("ELF file corrupt:"); exit(-2); }
					if(strcmp(name, "printf")==0) printf("LOL");
					insert(global_symbol_table, symbol_tab[j], name);
			//		printf("%s\n", name);
				}
			}
			module_table[k].file=file[i];
			module_table[k].filename=(char *)malloc(50*sizeof(char));
			strcpy(module_table[k].filename, file_names[i]);
			module_table[k].header=header;
			module_table[k].symseg_offset=0;
			module_table[k].symbols=symbol_tab;
			module_table[k].string_size=section[str_idx].sh_size;
			module_table[k].strings=&section[str_idx];
			module_table[k].allocated_address=&addr;
			k++;
		}
	}
	*global_symbol_tabl=global_symbol_table;
	*module_tabl=module_table;
	return k;
}

#ifdef DEBUG_SYMBOL_TABLE
int main(int argc, char **argv) {
	if(argc<2) {
		printf("No filename mentioned\n");
		exit(-1);
	}
	int i, n=argc-1;
	char **file_names=(char **)malloc(n*sizeof(char *));
	struct _glosym **global_symbol_table;
	struct _module_entry *module_table;
	for(i=0;i<n;i++) {
		file_names[i]=(char *)malloc(50*sizeof(char));
		strcpy(file_names[i], argv[i+1]);
	}
	first_pass(file_names, n, &global_symbol_table, &module_table);
	printf("Unresolved global symbols:\n");
	for(i=0;i<256;i++)
		if(global_symbol_table[i]!=NULL) {
			struct _glosym *node=global_symbol_table[i];
			while(node!=NULL) {
				if(!node->defined)
					printf("%s\n", node->name);
				node=node->link;
			}
		}
}
#endif
