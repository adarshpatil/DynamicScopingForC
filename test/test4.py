import os, sys, unittest
from tools import SamplesTestCase

OUTPUT = '''\
#include<stdlib.h>
#include<stdio.h>
//Declaring all Variables of DynamicType
typedef struct {int type; union{float floatval;int intval;}du;} atype;
typedef struct {int type; union{int intval;}du;} btype;

atype a; btype b; 

int f1();
void f2()
{
	//Backup global struct before local init
	atype a1;
	a1.type = a.type; a1.du.floatval = a.du.floatval; 
	a.type = 0;


	//Restoring from backup variables
	a.du.floatval = a1.du.floatval; a.type = a1.type;
}
int main()
{ 
	//Initalizing global variables
	a.type = -1;
	b.type = -1;


	//Backup global struct before local init
	atype a1;
	a1.type = a.type; a1.du.floatval = a.du.floatval; a1.du.intval = a.du.intval; 
	a.type = 1;
	a.du.intval = 2;

//Backup global struct before local init
	btype b1;
	b1.type = b.type; b1.du.intval = b.du.intval; 
	b.type = 0;
	b.du.intval = 10;

	f1();

	//Restoring from backup variables
	b.du.intval = b1.du.intval; b.type = b1.type;

	//Restoring from backup variables
	a.du.floatval = a1.du.floatval; 	a.du.intval = a1.du.intval; a.type = a1.type;
}
int f1()
{
if (a.type==-1) {printf("ERROR: VARIABLE a IS UNDEFINED AT LOC 85"); exit(1);}
if (a.type==0)	a.du.floatval=10;
if (a.type==1)	a.du.intval=10;

if (a.type==-1) {printf("ERROR: VARIABLE a IS UNDEFINED AT LOC 92"); exit(1);}
if (b.type==-1) {printf("ERROR: VARIABLE b IS UNDEFINED AT LOC 94"); exit(1);}
if (a.type==0 && b.type==0)	a.du.floatval=b.du.intval+a.du.floatval;
if (a.type==1 && b.type==0)	a.du.intval=b.du.intval+a.du.intval;

if (a.type==-1) {printf("ERROR: VARIABLE a IS UNDEFINED AT LOC 100"); exit(1);}
if (a.type==0)	a.du.floatval=f2();
if (a.type==1)	a.du.intval=f2();

if (a.type==-1) {printf("ERROR: VARIABLE a IS UNDEFINED AT LOC 116"); exit(1);}
if (a.type==0)	return a.du.floatval;
if (a.type==1)	return a.du.intval;

}
'''

PROG = 'rewriter'

class TestRewriterSample(SamplesTestCase):
    def test_live(self):
        self.assertSampleOutput([PROG], 'test4.c', OUTPUT)

if __name__ == '__main__':
  unittest.main()
