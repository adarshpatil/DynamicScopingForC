#!/usr/bin/python
import os, sys, unittest
from tools import SamplesTestCase

OUTPUT = '''\
#include<stdlib.h>
#include<stdio.h>
//Declaring all Variables of DynamicType
typedef struct {int type; union{int intval;}du;} atype;
typedef struct {int type; union{int intval;}du;} btype;
typedef struct {int type; union{int intval;}du;} ztype;

atype a; btype b; ztype z; 

int f1();
int main()
{ 
	//Initalizing global variables
	a.type = -1;
	b.type = -1;
	z.type = 0;

	//Backup global struct before local init
	atype a1;
	a1.type = a.type; a1.du.intval = a.du.intval; 
	a.type = 0;

//Backup global struct before local init
	btype b1;
	b1.type = b.type; b1.du.intval = b.du.intval; 
	b.type = 0;
	b.du.intval = 10;

if (a.type==0 && b.type==0)	printf("BEFORE F1 A=%d B=%d\\n",a.du.intval,b.du.intval);

	printf("RETURN VALUE FROM F1:%d\\n",f1());

if (a.type==0 && b.type==0)	printf("AFTER F1 B=%d B=%d\\n",a.du.intval,b.du.intval);


	//Restoring from backup variables
	b.du.intval = b1.du.intval; b.type = b1.type;

	//Restoring from backup variables
	a.du.intval = a1.du.intval; a.type = a1.type;
}
;
int f1()
{
	if(z.type==0)	z.du.intval = 1000;
if (a.type==-1) {printf("ERROR: VARIABLE a IS UNDEFINED AT LOC 188"); exit(1);}
if (b.type==-1) {printf("ERROR: VARIABLE b IS UNDEFINED AT LOC 190"); exit(1);}
if (a.type==0 && b.type==0 && z.type==0)	a.du.intval=b.du.intval+z.du.intval;

if (a.type==-1) {printf("ERROR: VARIABLE a IS UNDEFINED AT LOC 204"); exit(1);}
if (b.type==-1) {printf("ERROR: VARIABLE b IS UNDEFINED AT LOC 206"); exit(1);}
if (a.type==0 && b.type==0)	return (a.du.intval+b.du.intval);

}

/*HIGHLIGHTS
printf works
Line 13 addition between local and non local var*/
'''

PROG = 'rewriter'

class TestRewriterSample(SamplesTestCase):
    def test_live(self):
        self.assertSampleOutput([PROG], 'test1.c', OUTPUT)

if __name__ == '__main__':
  unittest.main()
