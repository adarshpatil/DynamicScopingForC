#include<stdio.h>
#include<math.h>
void f()
{
	float k = 121;
	printf("in f %f\n",sqrt(k));
}
int main()
{
	int k = 5;
	printf("in main %f\n",pow(2,k));
	f();
	return 0;
}

/*HIGHLIGHTS
header functions pow and sqrt used*/
