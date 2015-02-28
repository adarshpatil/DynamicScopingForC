float c=2.5,b=5.2;
void f();
void f1()
{
	float mainvar = 1.0;
	f();
}
int main()
{
	int b;
	int mainvar=1;
	f();
	f1();
}
void f()
{
	int c=10;
	{
		int b=25;
		b = b * 3;
	}
	if(mainvar==1)
	{
		mainvar++;
		cvar = cvar * 2;
	}
}
