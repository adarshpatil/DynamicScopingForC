float bvar=50;
struct who{
	int z;
};
int main()
{
	int bvar;
}
void f()
{
	struct who bvar;
	struct who svar;
	bvar.z = svar.z = 20;
	bvar.z = bvar.z + svar.z;
}
