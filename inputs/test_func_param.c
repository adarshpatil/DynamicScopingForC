#include<stdio.h>
void func(int c);
int var = 10;
int main()
{
	printf("BEFORE FUNC IN MAIN:%d\n",var);
	func(var);
	printf("AFTER FUNC IN MAIN: %d\n",var);
	return 0;
}
void func(int var)
{
	var = var + 76;
	printf("IN FUNC %d\n",var);
}
