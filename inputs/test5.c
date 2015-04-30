int a=10;
void f	();
int main()
{
	float b=1.5;
	a=b+a;
	f();
	return a;
}
void f()
{
	float fy = 10.1;
	int fx = 35;
	fy = fx + b;
	fx = fx + b;
	fx = fx + fy;
}

/*HIGHLIGHTS
LINE 14,15,16 addition with local and non local, local and local var*/
