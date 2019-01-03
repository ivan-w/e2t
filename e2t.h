#ifndef __E2T_H__
#define __E2T_H__

#include <libelf.h>
#include <gelf.h>
#include <string.h>
#include <stdlib.h>

typedef struct __e2t_list
{
	void	**d;
	size_t	c;
	size_t	s;
	size_t	i;
} e2t_list;
	

typedef struct __e2t
{
	char	*ifn;
	int	iefd;	/* File Descriptor of source */
	Elf	*ielf;	/* Elf Descriptor of source */
	GElf_Ehdr	*iehdr;	/* GElf Header */
	GElf_Shdr	*symthdr;	/* Section Header for SYMTAB */
	Elf_Data	*syms;		/* Section Data for SYMTAB */
	e2t_list	got;	/* Symbols defined in GOT */
	e2t_list	plt;	/* Symbols defined in PLT */
	e2t_list	sect_relas;	/* rela in section list */
	int		exposehidden : 1,   /* Generate ENTRY for symbols that have the HIDDEN flag in ELF (-X) */
			comasds : 1,    /* Generate DS instead of COM for ELF Common symbols (-D) */
			noentry : 1,    /* Do not generate ENTRY statements - to overcome ESD limit in IFOX (-N) */
			genmap : 1,     /* Generate a symbol map (-M) */
			lplt : 1;       /* Always do a PLT call even for local calls instead of a direct call (-P) */
                            /* PLT Calls are still generated for foreign (external) calls if the flag is not set */
} e2t;

typedef struct __e2t_rela_in_section
{
	int	scn;	/* Section to which RELA applies */
	size_t	offset;	/* Offset in section */
	size_t	len;	/* Length of RELA */
	char	*asml;	/* ASM line to add at this offset */
} e2t_rela_in_section;

extern	int	e2t_elf_load(e2t *);
extern	void	e2t_asm_normalize_symbol(char *,char *,int);
extern	char 	*e2t_elf_symbol_name_suffixed(e2t *e,int,char *);
extern	char 	*e2t_elf_symbol_name(e2t *e,int);
extern	void    e2t_asm_process_symbols(e2t *,GElf_Shdr *,Elf_Data *);
extern	char    *e2t_elf_section_name(e2t *,int);
extern	void	e2t_asm_head(e2t *,char *,char *);
extern	void    e2t_asm_process_progbits_section(e2t *,int,Elf_Data *,char *,char *);
extern	void    e2t_asm_process_nobits_section(e2t *,size_t len,char *,char *);
extern	void    e2t_asm_process_rela_got(e2t *,int,Elf_Data *);
extern	void    e2t_asm_process_rela_plt(e2t *,int,Elf_Data *);
extern	void    e2t_asm_process_rela_final(e2t *,int,Elf_Data *,int);

static inline void    e2t_list_init(e2t_list *l,size_t sz)
{
    l->d=malloc(sizeof(void *)*sz);
    l->c=0;
    l->s=sz;
    l->i=sz;
}
static inline void    e2t_list_add(e2t_list *l,void *d)
{
    if(l->c==l->s)
    {
        l->s+=l->i;
        l->d=realloc(l->d,sizeof(void *)*l->s);
    }
    l->d[l->c]=d;
    l->c++;
}
static inline int e2t_list_add_string(e2t_list    *l,char *str)
{
    int i;
    for(i=0;i<l->c;i++)
    {
        if(strcmp((char *)(l->d[i]),str)==0) return 1;
    }
    char    *s;
    s=malloc(strlen(str)+1);
    strcpy(s,str);
    e2t_list_add(l,s);
    return 0;
}


#endif
