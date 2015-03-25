int f1();
int main()
{
	int a,b=10;
	printf("BEFORE F1 A=%d B=%d\n",a,b);
	printf("RETURN VALUE FROM F1:%d\n",f1());
	printf("AFTER F1 B=%d B=%d\n",a,b);
}
int z;
int f1()
{
	z = 1000;
	a=b+z;
	return (a+b);
}
