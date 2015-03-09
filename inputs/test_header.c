#include<stdio.h>
#include<math.h>
void f()
{
	float a = 121;
	printf("in f %f\n",sqrt(a));
}
int main()
{
	int a = 5;
	printf("in here %f\n",pow(2,a));
	f();
	return 0;
}
