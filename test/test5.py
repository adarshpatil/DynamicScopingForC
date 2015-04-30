#!/usr/bin/python
import os, sys, unittest
from tools import SamplesTestCase

OUTPUT = '''\
#include<stdlib.h>
#include<stdio.h>
//Declaring all Variables of DynamicType
typedef struct {int type; union{int intval;}du;} atype;
typedef struct {int type; union{float floatval;}du;} btype;
typedef struct {int type; union{float floatval;}du;} fytype;
typedef struct {int type; union{int intval;}du;} fxtype;

atype a; btype b; fytype fy; fxtype fx; 

;
void f	();
int main()
{ 
	//Initalizing global variables
	a.type = 0;	a.du.intval = 10;
	b.type = -1;
	fy.type = -1;
	fx.type = -1;


	//Backup global struct before local init
	btype b1;
	b1.type = b.type; b1.du.floatval = b.du.floatval; 
	b.type = 0;
	b.du.floatval = 1.5;

	if(a.type==0 && b.type==0)	a.du.intval=b.du.floatval+a.du.intval;
	f();
	if(a.type==0)	return a.du.intval;

	//Restoring from backup variables
	b.du.floatval = b1.du.floatval; b.type = b1.type;
}
void f()
{
	//Backup global struct before local init
	fytype fy1;
	fy1.type = fy.type; fy1.du.floatval = fy.du.floatval; 
	fy.type = 0;
	fy.du.floatval = 10.1;

	//Backup global struct before local init
	fxtype fx1;
	fx1.type = fx.type; fx1.du.intval = fx.du.intval; 
	fx.type = 0;
	fx.du.intval = 35;

if (b.type==-1) {printf("ERROR: VARIABLE b IS UNDEFINED AT LOC 131"); exit(1);}
if (b.type==0 && fy.type==0 && fx.type==0)	fy.du.floatval = fx.du.intval + b.du.floatval;

if (b.type==-1) {printf("ERROR: VARIABLE b IS UNDEFINED AT LOC 145"); exit(1);}
if (b.type==0 && fx.type==0)	fx.du.intval = fx.du.intval + b.du.floatval;

	if(fx.type==0 && fy.type==0)	fx.du.intval = fx.du.intval + fy.du.floatval;

	//Restoring from backup variables
	fx.du.intval = fx1.du.intval; fx.type = fx1.type;

	//Restoring from backup variables
	fy.du.floatval = fy1.du.floatval; fy.type = fy1.type;
}

/*HIGHLIGHTS
LINE 14,15,16 addition with local and non local, local and local var*/
'''

PROG = 'rewriter'

class TestRewriterSample(SamplesTestCase):
    def test_live(self):
        self.assertSampleOutput([PROG], 'test5.c', OUTPUT)

if __name__ == '__main__':
  unittest.main()
