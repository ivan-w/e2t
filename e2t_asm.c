#include "e2t.h"
#include <elf.h>
#include <gelf.h>
#include <ctype.h>
#include <string.h>
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>

static	const	char	*upper="ABCDEFGHIJKLMNOPQRSTUVWXYZ$";
static	const	char	*uppernum="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789$";

/* Generate a portable IFOX comptatible label from a string */
/*
 * If the label is 8 char long, the first character is a lower alpahabetic,
     and the rest is lower alpha or numeric - convert to upper case 
 *
 * Otherwise make a MD5 hash, and create from that a 8 char upper case label
 *   with a starting alpha character
 *
*/
void	e2t_asm_normalize_symbol(char *name,char	*bfr,int always)
{
	if(!always)
	{
		do
		{
			/* Check for length */
			if(strlen(name)>8) break;
			/* check first char a lower case alpha */
			if(!islower(name[0]) && name[0]!='$') break;
			int	isbad=0;
			int	i;
			/* Check only lower case or digit for 1-7 */
			for(i=1;i<strlen(name);i++)
			{
				if(isdigit(name[i])) continue;
				if(isalpha(name[i]) && islower(name[i])) continue;
				if(name[i]=='$') continue;
				isbad=1;
				break;
			}
			if(isbad) break;
			/* Compliant, use it */
			strcpy(bfr,name);
			for(i=0;i<strlen(bfr);i++)
			{
				if(islower(bfr[i]))
				{
					bfr[i]=toupper(bfr[i]);
				}
			}
			return;
		} while(0);
	}
	char	res[MD5_DIGEST_LENGTH];
	MD5(name,strlen(name),res);
	int	i;
	bfr[0]=upper[res[0]%strlen(upper)];
	for(i=1;i<8;i++)
	{
		bfr[i]=uppernum[res[i]%strlen(uppernum)];
	}
	bfr[8]=0;
}

/*
 * generate EQU, ENTRY, EXTRN and COM records for symbols
*/

void	e2t_asm_head(e2t *e,char *name,char *nname)
{
	printf("%-8.8s CSECT * %s\n",nname,name);
	printf("        DC  AL4($$$$SMAP)\n");
}

void	e2t_asm_process_symbols(e2t *e,GElf_Shdr *gsh,Elf_Data *ed)
{
	GElf_Sym	gsym;
	int	symc;
	int	i;
	int	smap_count=0;
	symc=gsh->sh_size/gsh->sh_entsize;
	printf("         DS    0D\n");
	for(i=1;i<symc;i++)
	{
		char	*sn;
		char	nsn[9];
		int	stb;
		int	stt;
		int	svis;
		int	shndx;
		char	*tsn;	/* Target section name */
		char	ntsn[9];	/* Target section name Normalized */
		
		gelf_getsym(e->syms,i,&gsym);
		stb=GELF_ST_BIND(gsym.st_info);
		stt=GELF_ST_TYPE(gsym.st_info);
		svis=GELF_ST_VISIBILITY(gsym.st_other);
		shndx=gsym.st_shndx;
		sn=e2t_elf_symbol_name(e,i);
		e2t_asm_normalize_symbol(sn,nsn,0);
		if(shndx!=SHN_UNDEF && shndx<SHN_LORESERVE)
		{
			tsn=e2t_elf_section_name(e,shndx);
			e2t_asm_normalize_symbol(tsn,ntsn,1);
			switch(stt)
			{
				case STT_FUNC:
				case STT_NOTYPE:
				case STT_OBJECT:
					if(stb==STB_GLOBAL)
					{
                        if(!e->noentry)
                        {
                                if(svis!=STV_HIDDEN || e->exposehidden)
                                {
                                    printf("         ENTRY %s * %s\n",nsn,sn);
                                }
                                else
                                {
                                    printf("* HIDDEN ENTRY %s * %s\n",nsn,sn);
                                }
                        }
						if(e->genmap)
						{
							printf("$$NM%4.4X DC    A(%s)\n",smap_count,nsn);
							printf("         DC    A($$SM%4.4X)\n",smap_count);
							printf("         DC    A($$NM%4.4X)\n",smap_count+1);
							smap_count++;
						}
						
					}
					printf("%-8.8s EQU   %s+%d\n",
						nsn,
						ntsn,
						gsym.st_value);
					break;
			}
		}
		else
		{
			switch(shndx)
			{
				case SHN_COMMON:
				{
					if(e->comasds)
					{
                            if(!e->noentry)
                            {
							    printf("         ENTRY %s * %s\n",nsn,sn);
                            }
							printf("         DS    0D\n");
							printf("%-8.8s DS    %dC\n",nsn,gsym.st_size);
					}
					else
					{
						printf("%-8.8s COM * %s\n",nsn,sn);
						printf("         DS    %dC\n",gsym.st_size);
						char	nfname[9];
						e2t_asm_normalize_symbol(e->ifn,nfname,0);
						printf("%-8.8s CSECT\n",nfname);
					}
				}
				break;
				case SHN_UNDEF:
				{
					if(strcmp(sn,"_GLOBAL_OFFSET_TABLE_")!=0)
					{
						printf("         EXTRN %s * %s\n",nsn,sn);
					}
				}
				break;
				default:
				break;
			}
		}
	}
	printf("$$NM%4.4X DC   A(-1),A(-1),A(-1)\n",smap_count);
	smap_count=0;
	if(e->genmap)
	{
		for(i=1;i<symc;i++)
		{
			char	*sn;
			int	stb;
			int	stt;
			int	shndx;
			
			gelf_getsym(e->syms,i,&gsym);
			stb=GELF_ST_BIND(gsym.st_info);
			stt=GELF_ST_TYPE(gsym.st_info);
			shndx=gsym.st_shndx;
			sn=e2t_elf_symbol_name(e,i);
			if(shndx!=SHN_UNDEF && shndx<SHN_LORESERVE)
			{
				switch(stt)
				{
					case STT_FUNC:
					case STT_NOTYPE:
					case STT_OBJECT:
					if(stb==STB_GLOBAL)
					{
						printf("$$SM%4.4X DC    AL1(%d)\n",smap_count,strlen(sn));
						printf("         DC    C'%s'\n",sn);
						smap_count++;
					}
					break;
				}
			}
		}
	}
}
void	e2t_asm_process_progbits_section(e2t *e,int scn,Elf_Data *ed,char *sname,char *nsname)
{
	int	i,ix;
	uint8_t	*b;
	if(ed->d_size==0) return;
	printf("%-8.8s DS    0D * PROGBITS Section %s\n",nsname,sname);
	b=(uint8_t *)ed->d_buf;
	for(ix=0,i=0;i<ed->d_size;i++)
	{
		int	foundrela=0;
		int	j;
		for(j=0;j<e->sect_relas.c;j++)
		{
			e2t_rela_in_section	*sr;
			sr=e->sect_relas.d[j];
			if(sr->scn!=scn) continue;
			if(sr->offset!=i) continue;
			if(ix)
			{
				printf("'\n");
			}
			printf("%s",sr->asml);
			i+=((sr->len)-1);
			foundrela=1;
			ix=0;
			break;
		}
		if(foundrela) continue;
		if((ix%32)==0)
		{
			if(ix)
			{
				printf("'\n");
			}
			printf(" DC X'");
		}
		printf("%2.2X",b[i]);
		ix++;
	}
	if(ix) printf("'\n");
}
void	e2t_asm_process_nobits_section(e2t *e,size_t sz,char *sname,char *nsname)
{
	int	i;
	if(!sz) return;
	printf("%-8.8s DS    0D * NOBITS Section %s\n",nsname,sname);
	printf("         DS    %dC\n",sz);
}

void	e2t_asm_process_rela_got(e2t *e,int count,Elf_Data *ed)
{
	int	i;
	GElf_Rela	grela;
	int	rt;
	int	rs;
	char	*symn;
	char	nsymn[9];
	char	*got_symn;
	char	got_nsymn[9];
	for(i=0;i<count;i++)
	{
		GElf_Sym	sym;
		gelf_getrela(ed,i,&grela);
		rt=GELF_R_TYPE(grela.r_info);
		rs=GELF_R_SYM(grela.r_info);
		switch(rt)
		{
			case R_390_GOTENT:
			case R_390_PLT32DBL:
				symn=e2t_elf_symbol_name(e,rs);
				got_symn=e2t_elf_symbol_name_suffixed(e,rs,"@GOT");
				e2t_asm_normalize_symbol(symn,nsymn,0);
				e2t_asm_normalize_symbol(got_symn,got_nsymn,1);
				if(e2t_list_add_string(&e->got,got_nsymn)==0)
				{
					printf("%-8.8s DC    A(%s)\n",
						got_nsymn,
						nsymn);
				}
			break;
		}
	}
}
void	e2t_asm_process_rela_plt(e2t *e,int count,Elf_Data *ed)
{
	int	i;
	GElf_Rela	grela;
	int	rt;
	int	rs;
	char	*plt_symn;
	char	plt_nsymn[9];
	char	*got_symn;
	char	got_nsymn[9];
	for(i=0;i<count;i++)
	{
		GElf_Sym	sym;
		gelf_getrela(ed,i,&grela);
		rt=GELF_R_TYPE(grela.r_info);
		rs=GELF_R_SYM(grela.r_info);
		switch(rt)
		{
			case R_390_PLT32DBL:
				plt_symn=e2t_elf_symbol_name_suffixed(e,rs,"@PLT");
				got_symn=e2t_elf_symbol_name_suffixed(e,rs,"@GOT");
				e2t_asm_normalize_symbol(plt_symn,plt_nsymn,1);
				e2t_asm_normalize_symbol(got_symn,got_nsymn,1);
				if(e2t_list_add_string(&e->plt,plt_nsymn)==0)
				{
					printf("* PLT Entry for %s\n",plt_symn);
					printf("%-8.8s DC    X'C010',AL4(((%s+2)-*)/2),X'5811000007F1'\n",
						plt_nsymn,
						got_nsymn);
				}
			break;
		}
	}
}
void	e2t_asm_process_rela_final(e2t *e,int count,Elf_Data *ed,int shndx)
{
	int	i;
	GElf_Rela	grela;
	int	rt;
	int	rs;
	char	*symn;
	char	nsymn[9];
	int	isrel;
	int	isdbl;
	int	is64;

/*
	char	*secn;
	char	nsecn[9];
	secn=e2t_elf_section_name(e,shndx);
	e2t_asm_normalize_symbol(secn,nsecn,1);
*/

	for(i=0;i<count;i++)
	{
		size_t	rlen=4;
		GElf_Sym	sym;
		gelf_getrela(ed,i,&grela);
		rt=GELF_R_TYPE(grela.r_info);
		rs=GELF_R_SYM(grela.r_info);
		gelf_getsym(e->syms,rs,&sym);
		isrel=1;
		isdbl=1;
		is64=0;
		switch(rt)
		{
			case R_390_PLT32DBL:
				// printf("PLT32DBL\n");
				symn=e2t_elf_symbol_name_suffixed(e,rs,"@PLT");
				e2t_asm_normalize_symbol(symn,nsymn,0);
				break;
			case R_390_GOTENT:
				// printf("GOTENT\n");
				symn=e2t_elf_symbol_name_suffixed(e,rs,"@GOT");
				e2t_asm_normalize_symbol(symn,nsymn,0);
				break;
			case R_390_PC32:
				// printf("PC32\n");
				symn=e2t_elf_symbol_name(e,rs);
				e2t_asm_normalize_symbol(symn,nsymn,0);
				isdbl=0;
				break;
			case R_390_PC64:
				// printf("PC32\n");
				symn=e2t_elf_symbol_name(e,rs);
				e2t_asm_normalize_symbol(symn,nsymn,0);
				isdbl=0;
				is64=1;
				break;
			case R_390_32:
				// printf("PC32\n");
				symn=e2t_elf_symbol_name(e,rs);
				e2t_asm_normalize_symbol(symn,nsymn,0);
				isdbl=0;
				isrel=0;
				break;
			case R_390_PC32DBL:
				// printf("PC32DBL\n");
				symn=e2t_elf_symbol_name(e,rs);
				e2t_asm_normalize_symbol(symn,nsymn,0);
				break;
			case R_390_GOTPCDBL:
				// printf("GOTPCDBL\n");
				symn="$$$$$got";
				e2t_asm_normalize_symbol(symn,nsymn,0);
				break;
			default:
				fprintf(stderr,"What is Relocation type %d ?\n",rt);
				break;
		}
		/* Position to the right place */
/*
		printf("         ORG   %s+%d\n",nsecn,grela.r_offset);
*/

		char	*asmbfr;
		e2t_rela_in_section	*sr;

		asmbfr=malloc(80);
		sr=malloc(sizeof(e2t_rela_in_section));
		if(isrel)
		{
			if(isdbl)
			{
				sprintf(asmbfr,"         DC    AL4((((%s)+%d)-*)/2)\n",nsymn,grela.r_addend);
			}
			else
			{
				if(!is64)
				{
					sprintf(asmbfr,"         DC    AL4(((%s)+%d)-*)\n",nsymn,grela.r_addend);
				}
				else
				{
					sprintf(asmbfr,"         DC    AL8(((%s)+%d)-*)\n",nsymn,grela.r_addend);
				}
			}
		}
		else
		{
			sprintf(asmbfr,"         DC    AL4((%s)+%d)\n",nsymn,grela.r_addend);
		}
		
		sr->scn=shndx;
		sr->offset=grela.r_offset;
		sr->len=rlen;
		sr->asml=asmbfr;
		e2t_list_add(&e->sect_relas,sr);
	}
}
