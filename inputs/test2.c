typedef struct {
	int z;
}who;
void foo();
int main()
{
	float bvar=2.45;
	foo();

	//printf("IN MAIN BEFORE: %f\n",bvar.du.floatval);
	bvar = bvar*2;
	//printf("IN MAIN AFTER: %f\n",bvar.du.floatval);
}
void foo()
{
	who bvar, svar;
	bvar.z = svar.z = 20;
	bvar.z = bvar.z + svar.z;
	printf("IN FOO BVAR.Z:%d SVAR.Z:%d\n",bvar.z,svar.z);
}

/*HIGHLIGHTS
typedef struct usage and operations
PROBLEM cut paste struct on top to avoid error*/
