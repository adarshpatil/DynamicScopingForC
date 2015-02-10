int f1();
void f2()
{
	float a;
}
int main()
{
	int a=2,b=10;
	f1();
}
int f1()
{
	a=10;
	a=b+a;
	a=f2();
	return a;
}
