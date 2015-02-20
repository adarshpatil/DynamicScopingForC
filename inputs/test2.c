float bvar=50;
typedef struct {
	int z;
}who;
int main()
{
	int bvar;
}
void f()
{
	who bvar, svar;
	bvar.z = svar.z = 20;
	bvar.z = bvar.z + svar.z;
}
