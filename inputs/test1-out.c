#define SUBSTITUTE(x) (x.type==1 ? x.du.i : x.du.f)
typedef struct DynamicType
{
	union DynamicUnion { int i; float f; }du;
	int type;
}DT;
//Declaring all Variables of DynamicType
DT a; 

void f1();
int main()
{ 
	//Initalizing global variables
	a.type = 0;

	DT a1;
	a1.type = a.type;a1.du.i = a.du.i;a1.du.f = a.du.f;
	a.type = 1;;
	f1();
}
void f1()
{
	a.du.i = SUBSTITUTE(a) + 20;
	return;
}
