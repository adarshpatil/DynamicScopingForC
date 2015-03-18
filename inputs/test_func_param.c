#include<stdio.h>
void f(int c);
int c = 10,b;
int main()
{
	int c = 50;
	printf("before f in main:%d\n",c);
	f(c);
	printf("after f in main: %d\n",c);
	return 0;
}
void f(int c)
{
	int b=2;
	c = c + 100;
	printf("in f %d\n",c);
}
