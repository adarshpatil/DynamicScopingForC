#!/usr/bin/python
import os, sys, unittest
from tools import SamplesTestCase

OUTPUT = '''\
#include<stdio.h>
#include<stdlib.h>
#include<stdio.h>
//Declaring all Variables of DynamicType
typedef struct {int type; union{int intval;}du;} btype;
typedef struct {int type; union{float floatval;int intval;}du;} atype;

btype b; atype a; 

;
void adder();
void f2()
{
	//Backup global struct before local init
	atype a1;
	a1.type = a.type; a1.du.floatval = a.du.floatval; 
	a.type = 0;
	a.du.floatval = 10.2;

	adder();
	//printf("IN F2:%f\\n",a.du.floatval);

	//Restoring from backup variables
	a.du.floatval = a1.du.floatval; a.type = a1.type;
}
void f1()
{
	//Backup global struct before local init
	atype a1;
	a1.type = a.type; a1.du.floatval = a.du.floatval; a1.du.intval = a.du.intval; 
	a.type = 1;
	a.du.intval = 5000;

	adder();
	//printf("IN F1:%d\\n",a.du.intval);

	//Restoring from backup variables
	a.du.floatval = a1.du.floatval; 	a.du.intval = a1.du.intval; a.type = a1.type;
}
int main()
{ 
	//Initalizing global variables
	b.type = 0;	b.du.intval = 100;
	a.type = -1;


	f1();
	f2();
	return 0;
}
void adder()
{
if (a.type==-1) {printf("ERROR: VARIABLE a IS UNDEFINED AT LOC 253"); exit(1);}
if (a.type==0 && b.type==0)	a.du.floatval = a.du.floatval+b.du.intval;
if (a.type==1 && b.type==0)	a.du.intval = a.du.intval+b.du.intval;

}

/*HIGHLIGHTS
adder() is called once with float var and then int var*/
'''

PROG = 'rewriter'

class TestRewriterSample(SamplesTestCase):
    def test_live(self):
        self.assertSampleOutput([PROG], 'test4.c', OUTPUT)

if __name__ == '__main__':
  unittest.main()
