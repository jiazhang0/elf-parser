#include <iostream>
#include <cstdio>
#include <cstdlib>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <elf.h>

using namespace std;

Elf64_Ehdr *elfhdr;
Elf64_Shdr *shdr;

char *sname;
unsigned long base;

void parse_ehdr(void)
{
	printf("\n");

	elfhdr = (Elf64_Ehdr *)base;
	shdr = (Elf64_Shdr *)(base + elfhdr->e_shoff);
	sname = (char *)(base + shdr[elfhdr->e_shstrndx].sh_offset);
	
	printf("e_machine: %d\n", elfhdr->e_machine);
	printf("e_ident  : ");
	for (int i = 0; i < EI_NIDENT; i++)
		printf("%02hhx ", elfhdr->e_ident[i]);
	printf("\n");
	printf("Num of Program Header: %d\n", elfhdr->e_phnum);
	printf("Num of Section Header: %d\n", elfhdr->e_shnum);
	printf("String Section Index : %d\n", elfhdr->e_shstrndx);
}

void parse_sections(void)
{
	printf("\n");
	
	for (int i = 0; i < elfhdr->e_shnum; i++) {
		printf("Section(%d, type=0x%x): %s\n", i, shdr[i].sh_type, sname + shdr[i].sh_name);
	}
}

void parse_rela_sections(void)
{
	printf("\n");
	
	for (int i = 0; i < elfhdr->e_shnum; i++) {
		if (shdr[i].sh_type != SHT_RELA) 
			continue;

		printf("Rela Section: %s, sh_link=%d, sh_info=%d\n", 
				sname + shdr[i].sh_name, shdr[i].sh_link, shdr[i].sh_info);

		Elf64_Rela *rela = (Elf64_Rela *)(base + shdr[i].sh_offset);
		int num = shdr[i].sh_size / shdr[i].sh_entsize;

		for (int j = 0; j < num; j++) {
			printf("rela(%d): r_offset=%lx, r_info=%lx, r_addend=%ld\n",
					j, rela[j].r_offset, rela[j].r_info, rela[j].r_addend);
			printf("\tsym=%lu, type=%lu\n",
					ELF64_R_SYM(rela[j].r_info),
					ELF64_R_TYPE(rela[j].r_info));
		}
	}
}

void parse_symtab_sections(void)
{
	printf("\n");

	for (int i = 0; i < elfhdr->e_shnum; i++) {
		if (shdr[i].sh_type != SHT_SYMTAB)
			continue;
	
		printf("Symtab Section: %s, sh_link=%d, sh_info=%d\n",
				sname + shdr[i].sh_name, shdr[i].sh_link, shdr[i].sh_info);

		Elf64_Sym *sym = (Elf64_Sym *)(base + shdr[i].sh_offset);
		int num = shdr[i].sh_size / shdr[i].sh_entsize;
		char *sdata = (char *)base + shdr[shdr[i].sh_link].sh_offset;

		for (int j = 0; j < num; j++) {
			printf("sym(%d): st_name=%s, st_value=%lx, st_shndx=0x%x\n",
					j, sdata+sym[j].st_name, sym[j].st_value, sym[j].st_shndx);
			printf("\tst_info=%x, st_bind=%x, st_type=%x\n",
					sym[j].st_info,
					ELF64_ST_BIND(sym[j].st_info),
					ELF64_ST_TYPE(sym[j].st_info));
		}
	}
}

void parse_dynamic_sections(void)
{
	printf("\n");

	for (int i = 0; i < elfhdr->e_shnum; i++) {
		if (shdr[i].sh_type != SHT_DYNAMIC)
			continue;

		printf("Dynamic Section: %s, sh_link=%d, sh_info=%d\n",
				sname + shdr[i].sh_name, shdr[i].sh_link, shdr[i].sh_info);

		Elf64_Dyn *dyn = (Elf64_Dyn *)(base + shdr[i].sh_offset);
		int num = shdr[i].sh_size / shdr[i].sh_entsize;
		char *sdata = (char *)base + shdr[shdr[i].sh_link].sh_offset;

		for (int j = 0; j < num; j++) {
			printf("dynamic(%d): d_tag=%lx, d_val=%lx, d_ptr=%lx\n",
					j, dyn[j].d_tag, dyn[i].d_un.d_val, dyn[i].d_un.d_ptr);
		}
	}
}

void parse_dynsym_sections(void)
{
	printf("\n");

	for (int i = 0; i < elfhdr->e_shnum; i++) {
		if (shdr[i].sh_type != SHT_DYNSYM)
			continue;

		printf("Dynsym Section: %s, sh_link=%d, sh_info=%d\n",
				sname + shdr[i].sh_name, shdr[i].sh_link, shdr[i].sh_info);

		Elf64_Sym *sym = (Elf64_Sym *)(base + shdr[i].sh_offset);
		int num = shdr[i].sh_size / shdr[i].sh_entsize;
		char *sdata = (char *)base + shdr[shdr[i].sh_link].sh_offset;

		for (int j = 0; j < num; j++) {
			printf("dynsym(%d): st_name=%s, st_value=%lx\n",
					j, sdata+sym[j].st_name, sym[j].st_value);
		}
	}
}

void parse_livepatch_build_id(void)
{
	for (int i = 0; i < elfhdr->e_shnum; i++) {
		if (strcmp(sname + shdr[i].sh_name, ".note.gnu.build-id") != 0)
			continue;
		
		char *build_id = (char *)base + shdr[i].sh_offset;
		printf(".note.gnu.build-id: ");
		for (int j = 0; j < shdr[i].sh_size; j++)
			printf("%02hhx", build_id[j]);
		printf("\n");
	}
}

void parse_livepatch_depends(void)
{
	for (int i = 0; i < elfhdr->e_shnum; i++) {
		if (strcmp(sname + shdr[i].sh_name, ".livepatch.depends") != 0)
			continue;

		char *depends = (char *)base + shdr[i].sh_offset;
			printf(".livepatch_depends: ");
		for (int j = 0; j < shdr[i].sh_size; j++)
			printf("%02hhx", depends[j]);
		printf("\n");
	}
}

struct livepatch_patch_func {
	const char *name;
	void *new_addr;
	void *old_addr;
	uint32_t new_size;
	uint32_t old_size;
	uint8_t version;
	uint8_t opaque[31];  
};  

/* shdr[i].sh_entsize is 0 !!! */
void parse_livepatch_patch_func(void)
{
	for (int i = 0; i < elfhdr->e_shnum; i++) {
		if (strcmp(sname + shdr[i].sh_name, ".livepatch.funcs") != 0)
			continue;
		struct livepatch_patch_func *func = 
			(struct livepatch_patch_func *)(base + shdr[i].sh_offset);
		int sh_entsize = sizeof(struct livepatch_patch_func);
		int num = shdr[i].sh_size / sh_entsize;
		
		for (int j = 0; j < num; j++) {
			printf("func(%d): name(0x%016lx)=%s, new_addr=0x%016lx, old_addr=0x%016lx\n",
					j,
					(unsigned long)func[j].name,
					func[j].name,
					(unsigned long)func[j].new_addr, 
					(unsigned long)func[j].old_addr);
			printf("\tnew_size=%d, old_size=%d, version=%d\n",
					func[j].new_size, func[j].old_size, func[j].version);
			printf("\topaque=");
			for (int k = 0; k < 31; k++)
				printf("%02hhx ", func[j].opaque[k]);
			printf("\n");
		}
	}
}

void parse_livepatch_strings(void)
{
	for (int i = 0; i < elfhdr->e_shnum; i++) {
		if (strcmp(sname + shdr[i].sh_name, ".livepatch.strings") != 0)
			continue;

		char *str = (char *)base + shdr[i].sh_offset;
		printf("\n.livepatch.strings data:\n");
		for (int j = 0; j < shdr[i].sh_size; j++) {
			if (j % 16 == 0 && j != 0)
				printf("\n");
			printf("(%d)%c ", j, str[j]);
		}
		printf("\n");
	}
}

void parse_livepatch(void)
{
	printf("\nlivepatch info:\n");
	parse_livepatch_build_id();
	parse_livepatch_depends();
	parse_livepatch_patch_func();
	parse_livepatch_strings();
}

int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: ./parse_mmap filename\n");
		return -1;
	}

	int fd;
	struct stat fs;
	char *filename;
	int len;

	filename = argv[1];
	stat(filename, &fs);
	len = fs.st_size;

	fd = open(filename, O_RDWR, S_IRUSR|S_IWUSR);
	base = (unsigned long)mmap((void *)0, len, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if (base == (unsigned long)MAP_FAILED) {
		printf("failed to map elf file into memory\n");
		return -1;
	}

	parse_ehdr();
	parse_sections();
	parse_rela_sections();
	parse_dynsym_sections();
	parse_dynamic_sections();
	parse_symtab_sections();
	parse_livepatch();

	munmap((void *)base, len);
	close(fd);
}
