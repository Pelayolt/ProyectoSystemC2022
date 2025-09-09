#include "coreRiscV.h"
#include "structsRV.h"


int coreRiscV::leeELF(FILE* elf) {

	unsigned int I;
	unsigned short S;
	unsigned char B, tmp[4096]; 
	int j, k; 
	struct f_header FH;
	struct p_header *PH; 
	struct s_header *SH;

	// el código está escrito para 32 bits y little endian


	fread(&FH, 1, 0x34, elf);

	if (FH.magic!= 0x464c457f)	// ELF control
		return -1;

	if (FH.e_phentsize != 0x20)
		return -1; 

	instFetch->initPC(FH.e_entry);


	PH = new struct p_header [FH.e_phnum+1]; // añado un buffer para la pila???

	fseek(elf, FH.e_phoff, 0);				// carga de los program headers
	for (j = 0; j < FH.e_phnum; ++j) {
		fread(&PH[j], 1, FH.e_phentsize, elf);
			MEM->addMemSeg(PH[j].p_vaddr, PH[j].p_memsz); 
	}

	MEM->addMemSeg(0x20000000, 0x10000);	// 64 KB extra
	FH.e_phnum++;


	SH = new struct s_header[FH.e_shnum];

	fseek(elf, FH.e_shoff, 0);				// carga de las sections
	for (j = 0; j < FH.e_shnum; ++j) {
		fread(&SH[j], 1, FH.e_shentsize, elf);
	//	cout << SH[j].sh_addr << " " << SH[j].sh_flags << endl; 
	}

	for (j = 0; j < FH.e_shnum; ++j) {
		if (SH[j].sh_size && SH[j].sh_flags) {
			fseek(elf, SH[j].sh_offset, 0);
			for(k=0; k<SH[j].sh_size; ++k)
				MEM->writeByte(SH[j].sh_addr + k, fgetc(elf));
		}
	}

	fclose(elf);
	return 0; 

}