#include<math.h>
float c=2.5,b=5.2;
void func();
int main()
{
	int b=1000;
	int mainvar=1;
	printf("IN MAIN C:%f\n",c);
	func();
}
void func()
{
	int c=10;
	{
		int b=5;
		b = pow(b,3);
		printf("INSIDE BLOCK B:%d\n",b);
	}
	printf("INSIDE FUNC MAINVAR:%d B:%d\n",mainvar,b);
	if(mainvar==1)
	{
		mainvar++;
		c = c * 2;
	}
	printf("INSIDE FUNC C:%d\n",c);
}
