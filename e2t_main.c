#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "e2t.h"

static	inline	void	e2t_usage(char *progname)
{
	fprintf(stderr,"Usage : %s [-vX] filename\n",progname);
	return;
}

static	inline	int	e2t_options(e2t *e,int ac,char **av)
{
	int	oc;
	while((oc=getopt(ac,av,"vXCM"))!=-1)
	{
		switch(oc)
		{
			case 'v':
				fprintf(stderr,"e2t Version 1.0\n");
				return 2;
				break;
			case 'X':
				e->exposehidden=1;
				break;
			case 'M':
				e->genmap=1;
				break;
			case 'C':
				e->comasds=1;
				break;
			case '?':
				return 1;
		}
	}
	if(optind>=ac)
	{
		fprintf(stderr,"Missing input file\n");
		return 1;
	}
	e->ifn=malloc(strlen(av[optind])+1);
	strcpy(e->ifn,av[optind]);
	return 0;
}

int	main(int ac,char **av)
{
	e2t	e2t;
	int	rc;
	memset(&e2t,0,sizeof(e2t));
	rc=e2t_options(&e2t,ac,av);
	if(rc>0)
	{
		if(rc==2) return 0;
		e2t_usage(av[0]);
		return 1;
	}
	e2t_list_init(&e2t.got,256);
	e2t_list_init(&e2t.plt,256);
	e2t_list_init(&e2t.sect_relas,256);

	rc=e2t_elf_load(&e2t);
	if(rc)
	{
		return 1;
	}
#if 0
	rc=e2t_relocate(&e2t);
	if(rc)
	{
		return 1;
	}
	rc=e2t_asm_gen(&e2t);
	if(rc)
	{
		return 1;
	}
#endif
	return 0;
}
