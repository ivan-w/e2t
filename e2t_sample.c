#include<stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
static __thread volatile int	foo=1;
static __thread volatile int	bar;
// char	bar[256];
#define PRIME_TYPE	uint32_t
#define _STATIC static
#define _INLINE inline
_STATIC	_INLINE	void	setbitpos(PRIME_TYPE	*a,int pos)
{
	(*a)|=(1<<pos);
}

_STATIC	_INLINE	int	getbitpos(PRIME_TYPE	a,int pos)
{
	return !(a&=(1<<pos));
}
_STATIC	_INLINE	void	setnonprime(PRIME_TYPE p,PRIME_TYPE *a)
{
	size_t	ix;
	PRIME_TYPE	t;
	int	bpos;
	if(!p) return;
	if(p==1) return;	/* 1 is NOT prime */
	if(p==2) return;	/* 2 is prime */
	if(!(p&1)) return;	/* Even numbers (except 2) are not prime */
	ix=p/(sizeof(PRIME_TYPE)*8);	/* 32 bits per value in map */
	bpos=p%(sizeof(PRIME_TYPE)*8);
	setbitpos(&(a[ix]),bpos);
	return;
}
_STATIC	_INLINE	int	isprime(PRIME_TYPE p,PRIME_TYPE *a)
{
	size_t	ix;
	PRIME_TYPE	t;
	int	bpos;
	if(!p) return 0;
	if(p==1) return 0;	/* 1 is NOT prime */
	if(p==2) return 1;	/* 2 is prime */
	if(!(p&1)) return 0;	/* Even numbers (except 2) are not prime */
	ix=p/(sizeof(PRIME_TYPE)*8);	/* 32 bits per value in map */
	t=a[ix];
	bpos=p%(sizeof(PRIME_TYPE)*8);
	return getbitpos(a[ix],bpos);
}
	
_STATIC _INLINE PRIME_TYPE	*sieve(PRIME_TYPE t)
{
	PRIME_TYPE	*a;
	size_t	asz;
	asz=t/(sizeof(*a)/sizeof(uint8_t));
	asz++;
	printf("Size of array = %lu\n",asz);
	a=malloc(asz);
	// memset(a,0,asz);
	PRIME_TYPE	i;
	for(i=1;i<t;i++)
	{
		if(!isprime(i,a)) continue;
		PRIME_TYPE	j;
		for(j=i*3;j<t;j+=(i*2))
		{
			setnonprime(j,a);
		}
	}
	return a;
}
int	main(int ac,char **av)
{
	char	*eptr;
	PRIME_TYPE	x,y;
	PRIME_TYPE	*a;
    // memset(foo,0,sizeof(foo));
    printf("%d\n",foo);
    printf("%d\n",bar);
	// memset(bar,0,sizeof(bar));
	if(ac<2) return 1;
	x=strtol(av[1],&eptr,10);
	if(*eptr!=0) return 3;
	if(ac>2)
	{
		y=strtol(av[2],&eptr,10);
		x*=y;
	}
	if(!x) return 2;
	printf("Sieve program %s\n",av[0]);
	a=sieve(x);
	PRIME_TYPE	i;
	PRIME_TYPE	pcount=0;
	for(i=1;i<x;i++)
	{
		if(isprime(i,a))
		{
			// printf("%d is prime\n",i);
			pcount++;
		}
	}
	printf("Prime count = %u\n",pcount);
	return 0;
}
