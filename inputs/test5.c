int a=10;
void f	();
int main()
{
	float b=1.5;
	a=b+a;
	a = 20;
	f();
	return a;
}
void f()
{
	float fb = 10.1;
	int fa = 35;
	fb = fa + b;
	fa = fa + b;
	fa = fa + fb;
}
