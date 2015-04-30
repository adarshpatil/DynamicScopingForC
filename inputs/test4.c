#include<stdio.h>
int b=100;
void adder();
void f2()
{
	float a=10.2;
	adder();
	//printf("IN F2:%f\n",a.du.floatval);
}
void f1()
{
	int a=5000;
	adder();
	//printf("IN F1:%d\n",a.du.intval);
}
int main()
{
	f1();
	f2();
	return 0;
}
void adder()
{
	a = a+b;
}

/*HIGHLIGHTS
adder() is called once with float var and then int var*/
