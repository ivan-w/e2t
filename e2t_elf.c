#include <libelf.h>
#include <gelf.h>
#include "e2t.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static	inline	void	e2t_hex_dump(Elf_Data *ed,char *sname)
{
	int	i;
	uint8_t	*b;
	if(ed->d_size==0)
	{
		printf("	Section %s is empty\n",sname);
		return;
	}
	b=(uint8_t *)(ed->d_buf);
	printf("	Section %s\n",sname);
	for(i=0;i<ed->d_size;i++)
	{
		if(i%16==0)
		{
			if(i) printf("\n");
			printf("%4.4X :",i);
		}
		if(i%4==0) printf(" ");
		printf("%2.2X",b[i]);
	}
	printf("\n");
}

static	inline	void	e2t_elf_process_rela_section(e2t *e,Elf_Data *rt,GElf_Shdr *sh)
{
}


int	e2t_elf_open(e2t *e)
{
	int	efd;
	Elf	*ielf;
	efd=open(e->ifn,0);
	if(efd<0) 
	{
		fprintf(stderr,"Cannot open file %s : %s\n",
			e->ifn,
			strerror(errno));
		return 1;
	}
	elf_version(EV_CURRENT);
	ielf=elf_begin(efd,ELF_C_READ,NULL);
	if(ielf==NULL)
	{
		int	err;
		err=elf_errno();
		fprintf(stderr,"elf_begin returned a NULL Elf descriptor : %s\n",elf_errmsg(err));
		return 1;
	}
	e->iefd=efd;
	e->ielf=ielf;
	return 0;
}

int	e2t_elf_load(e2t *e)
{
	int	rc;
	int	j;
	int	i;
	Elf_Kind	ek;
	rc=e2t_elf_open(e);
	if(rc) return rc;
	ek=elf_kind(e->ielf);
	if(ek!=ELF_K_ELF)
	{
		fprintf(stderr,"%s is not an ELF file\n",e->ifn);
		return 1;
	}
	/* Get gelf ELF Header */
	GElf_Ehdr	*ge,*rge;
	ge=malloc(sizeof(GElf_Ehdr));
	rge=gelf_getehdr(e->ielf,ge);
	if(rge==NULL)
	{
		fprintf(stderr,"gelf_getehdr failed\n");
		free(ge);
		return 1;
	}
	// printf("Class = %d, Type = %d, machine = %d\n",gelf_getclass(e->ielf),rge->e_type,rge->e_machine);
	if(rge->e_machine!=EM_S390)
	{
		fprintf(stderr,"%s is not a S/390 ELF file\n",e->ifn);
		free(ge);
		return 1;
	}
	if(rge->e_type!=ET_REL)
	{
		fprintf(stderr,"%s is not a relocatable ELF file\n",e->ifn);
		free(ge);
		return 1;
	}
	e->iehdr=ge;
	// fprintf(stderr,"Section count = %d\n",ge->e_shnum);
	char	nfname[9];
	e2t_asm_normalize_symbol(e->ifn,nfname,0);
	// fprintf(stderr,"CSECT section based on file name : %s\n",nfname);
	e2t_asm_head(e,e->ifn,nfname);
	for(i=1;i<ge->e_shnum;i++)
	{
		Elf_Scn	*en;
		GElf_Shdr	gsh;
		en=elf_getscn(e->ielf,i);
		gelf_getshdr(en,&gsh);
		Elf_Data	*ed;
		ed=elf_getdata(en,NULL);
		if(gsh.sh_type==SHT_SYMTAB)
		{
			if(e->symthdr==NULL)
			{
				e->symthdr=malloc(sizeof(GElf_Shdr));
				memcpy(e->symthdr,&gsh,sizeof(gsh));
				e->syms=ed;
				break;
			}
			else
			{
				fprintf(stderr,"%s has more than 1 symbol table\n");
				return 1;
			}
		}
	}
	if(e->symthdr==NULL)
	{
		fprintf(stderr,"%s has no symbol table\n",e->ifn);
		printf("*$$EMPTY EMPTY OBJECT\n");
		printf("$$$$SMAP DC  C'$$$$SMAP'\n");
		printf("         DC  AL4(0)\n");
		printf("         END\n");
		return 1;
	}
	for(i=1;i<ge->e_shnum;i++)
	{
		Elf_Scn	*en;
		int	count;
		char	*sname;
		char	nsname[9];
		GElf_Shdr	gsh;
		en=elf_getscn(e->ielf,i);
		gelf_getshdr(en,&gsh);
		Elf_Data	*ed;
		ed=elf_getdata(en,NULL);
		switch(gsh.sh_type)
		{
			case SHT_RELA:
			{
				count=gsh.sh_size/gsh.sh_entsize;
				e2t_asm_process_rela_final(e,count,ed,gsh.sh_info);
				break;
			}
			break;
			default:
			break;
		}
	}
	/* Dump PROGBIT/NOBIT + ALLOC Section Data */
	for(i=1;i<ge->e_shnum;i++)
	{
		Elf_Scn	*en;
		char	*sname;
		char	nsname[9];
		GElf_Shdr	gsh;
		en=elf_getscn(e->ielf,i);
		gelf_getshdr(en,&gsh);
		Elf_Data	*ed;
		ed=elf_getdata(en,NULL);
		sname=elf_strptr(e->ielf,ge->e_shstrndx,gsh.sh_name);
		e2t_asm_normalize_symbol(sname,nsname,1);
		switch(gsh.sh_type)
		{
			case SHT_PROGBITS:
			{
				if(gsh.sh_flags & SHF_ALLOC)
				{
					e2t_asm_process_progbits_section(e,i,ed,sname,nsname);
				}
			}
			break;
			case SHT_NOBITS:
			{
				if(gsh.sh_flags & SHF_ALLOC)
				{
					e2t_asm_process_nobits_section(e,gsh.sh_size,sname,nsname);
				}
			}
			break;
			default:
			break;
		}
	}
	/* 2 pass process : */
	/* Process RELA pass 1 : Create the pseudo GOT */
	/* Process RELA pass 2 : Create the pseudo PLT */
	for(j=0;j<2;j++)
	{
		switch(j)
		{
			case 0:
				printf("$$$$$GOT DS    0D\n");
				break;
			case 1:
				printf("$$$$$PLT DS    0D\n");
				break;
			default:
				break;
		}
		for(i=1;i<ge->e_shnum;i++)
		{
			Elf_Scn	*en;
			int	count;
			char	*sname;
			char	nsname[9];
			GElf_Shdr	gsh;
			en=elf_getscn(e->ielf,i);
			gelf_getshdr(en,&gsh);
			Elf_Data	*ed;
			ed=elf_getdata(en,NULL);
			switch(gsh.sh_type)
			{
				case SHT_RELA:
				{
					count=gsh.sh_size/gsh.sh_entsize;
					switch(j)
					{
						case 0:
							e2t_asm_process_rela_got(e,count,ed);
							break;
						case 1:
							e2t_asm_process_rela_plt(e,count,ed);
							break;
						default:
							abort();
					}
				}
				break;
				default:
				break;
			}
		}
	}
	printf("$$$$SMAP DC  C'$$$$SMAP'\n");
	printf("         DC  AL4(%s)\n",nfname);
	printf("         DC  AL1(%d)\n",strlen(e->ifn));
	printf("         DC  C'%s'\n",e->ifn);
	for(i=1;i<ge->e_shnum;i++)
	{
		Elf_Scn	*en;
		GElf_Shdr	gsh;
		en=elf_getscn(e->ielf,i);
		gelf_getshdr(en,&gsh);
		Elf_Data	*ed;
		ed=elf_getdata(en,NULL);
		if(gsh.sh_type==SHT_SYMTAB)
		{
			e2t_asm_process_symbols(e,&gsh,ed);
		}
	}
	printf("         DS    0D\n");
	printf("         DC    AL4(0)\n");
	printf("         END\n");
	return 0;
}
char	*e2t_elf_symbol_name(e2t *e,int six)
{
	GElf_Sym	sym;
	gelf_getsym(e->syms,six,&sym);
	if(GELF_ST_TYPE(sym.st_info)==STT_SECTION)
	{
		return(e2t_elf_section_name(e,sym.st_shndx));
	}
	/*
	fprintf(stderr,"Symbol %d : Symbol string table is %d, offset is +%d : %s\n",
			six,
			e->symthdr->sh_info,
			sym.st_name,
			elf_strptr(e->ielf,e->symthdr->sh_link,sym.st_name));
	*/
    if(GELF_ST_BIND(sym.st_info)==STB_GLOBAL)
    {
	    return(elf_strptr(e->ielf,e->symthdr->sh_link,sym.st_name));
    }
    else
    {
        char    *sn;
        sn=malloc(16);
        sprintf(sn,"__%8.8X",six);
        return(sn);
    }
}

char	*e2t_elf_symbol_name_suffixed(e2t *e,int six,char *suf)
{
	char	*sn;
	char	*snp;
	sn=e2t_elf_symbol_name(e,six);
	// fprintf(stderr,"Symbol at index %d is %s\n",six,sn);
	snp=malloc(strlen(sn)+strlen(suf)+1);
	strcpy(snp,sn);
	strcat(snp,suf);
	return snp;
}
char	*e2t_elf_section_name(e2t *e,int six)
{
	char	*sname;
	Elf_Scn	*en;
	GElf_Shdr	gsh;
	en=elf_getscn(e->ielf,six);
	gelf_getshdr(en,&gsh);
	sname=elf_strptr(e->ielf,e->iehdr->e_shstrndx,gsh.sh_name);
	return sname;
}
