import os, sys, unittest
from tools import SamplesTestCase

OUTPUT = '''\
#include<stdlib.h>
#include<stdio.h>
//Declaring all Variables of DynamicType
typedef struct {int type; union{int intval;}du;} atype;
typedef struct {int type; union{int intval;}du;} btype;
typedef struct {int type; union{int intval;}du;} ztype;
typedef struct {int type; union{int intval;}du;} ctype;

atype a; btype b; ztype z; ctype c; 

int f1();
int main()
{ 
	//Initalizing global variables
	a.type = -1;
	b.type = -1;
	z.type = 0;	c.type = -1;


	//Backup global struct before local init
	atype a1;
	a1.type = a.type; a1.du.intval = a.du.intval; 
	a.type = 0;

	//Backup global struct before local init
	btype b1;
	b1.type = b.type; b1.du.intval = b.du.intval; 
	b.type = 0;
	b.du.intval = 10;

	f1();

	//Restoring from backup variables
	b.du.intval = b1.du.intval; b.type = b1.type;

	//Restoring from backup variables
	a.du.intval = a1.du.intval; a.type = a1.type;
}
;
int f1()
{
	//Backup global struct before local init
	ctype c1;
	c1.type = c.type; c1.du.intval = c.du.intval; 
	c.type = 0;

	if(z.type==0)	z.du.intval = 10;
if (a.type==-1) {printf("ERROR: VARIABLE a IS UNDEFINED AT LOC 89"); exit(1);}
if (b.type==-1) {printf("ERROR: VARIABLE b IS UNDEFINED AT LOC 91"); exit(1);}
if (a.type==0 && b.type==0)	a.du.intval=b.du.intval+a.du.intval;

if (a.type==-1) {printf("ERROR: VARIABLE a IS UNDEFINED AT LOC 105"); exit(1);}
if (b.type==-1) {printf("ERROR: VARIABLE b IS UNDEFINED AT LOC 107"); exit(1);}
if (a.type==0 && b.type==0)	return (a.du.intval+b.du.intval);


	//Restoring from backup variables
	c.du.intval = c1.du.intval; c.type = c1.type;
}
'''

PROG = 'rewriter'

class TestRewriterSample(SamplesTestCase):
    def test_live(self):
        self.assertSampleOutput([PROG], 'test1.c', OUTPUT)

if __name__ == '__main__':
  unittest.main()
