typedef struct {
	int z;
}who;
void foo();
void test();
int main()
{
	float bvar=2.45;
	foo();

	printf("IN MAIN BEFORE TEST BVAR:%f\n",bvar);
	test();
	printf("IN MAIN AFTER TEST BVAR:%f\n",bvar);
}
void foo()
{
	who bvar, svar;
	bvar.z = svar.z = 20;
	bvar.z = bvar.z + svar.z;
	printf("IN FOO BVAR.Z:%d SVAR.Z:%d\n",bvar.z,svar.z);
}
void test()
{
	bvar = bvar * 2;
}
