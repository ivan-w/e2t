#include "e2t.h"
#include <stdlib.h>
#include <string.h>

void	e2t_list_init(e2t_list *l,size_t sz)
{
	l->d=malloc(sizeof(void *)*sz);
	l->c=0;
	l->s=sz;
	l->i=sz;
}
void	e2t_list_add(e2t_list *l,void *d)
{
	if(l->c==l->s)
	{
		l->s+=l->i;
		l->d=realloc(l->d,sizeof(void *)*l->s);
	}
	l->d[l->c]=d;
	l->c++;
}
int	e2t_list_add_string(e2t_list	*l,char *str)
{
	int	i;
	for(i=0;i<l->c;i++)
	{
		if(strcmp((char *)(l->d[i]),str)==0) return 1;
	}
	char	*s;
	s=malloc(strlen(str)+1);
	strcpy(s,str);
	e2t_list_add(l,s);
	return 0;
}
