#!/usr/bin/python
import os, sys, unittest
from tools import SamplesTestCase

OUTPUT = '''\
#include<stdlib.h>
#include<stdio.h>
//Declaring all Variables of DynamicType
typedef struct {int type; union{int intval;}du;} atype;
typedef struct {int type; union{float floatval;}du;} btype;
typedef struct {int type; union{float floatval;}du;} fbtype;
typedef struct {int type; union{int intval;}du;} fatype;

atype a; btype b; fbtype fb; fatype fa; 

;
void f	();
int main()
{ 
	//Initalizing global variables
	a.type = 0;	a.du.intval = 10;
	b.type = -1;
	fb.type = -1;
	fa.type = -1;


	//Backup global struct before local init
	btype b1;
	b1.type = b.type; b1.du.floatval = b.du.floatval; 
	b.type = 0;
	b.du.floatval = 1.5;

	if(a.type==0 && b.type==0)	a.du.intval=b.du.floatval+a.du.intval;
	if(a.type==0)	a.du.intval = 20;
	f();
	if(a.type==0)	return a.du.intval;

	//Restoring from backup variables
	b.du.floatval = b1.du.floatval; b.type = b1.type;
}
void f()
{
	//Backup global struct before local init
	fbtype fb1;
	fb1.type = fb.type; fb1.du.floatval = fb.du.floatval; 
	fb.type = 0;
	fb.du.floatval = 10.1;

	//Backup global struct before local init
	fatype fa1;
	fa1.type = fa.type; fa1.du.intval = fa.du.intval; 
	fa.type = 0;
	fa.du.intval = 35;

if (b.type==-1) {printf("ERROR: VARIABLE b IS UNDEFINED AT LOC 140"); exit(1);}
if (b.type==0 && fb.type==0 && fa.type==0)	fb.du.floatval.du.floatval = fa.du.intval + b.du.floatval;

if (b.type==-1) {printf("ERROR: VARIABLE b IS UNDEFINED AT LOC 154"); exit(1);}
if (b.type==0 && fa.type==0)	fa.du.intval = fa.du.intval + b.du.floatval;

	if(fa.type==0 && fb.type==0)	fa.du.intval = fa.du.intval + fb.du.floatval;

	//Restoring from backup variables
	fa.du.intval = fa1.du.intval; fa.type = fa1.type;

	//Restoring from backup variables
	fb.du.floatval = fb1.du.floatval; fb.type = fb1.type;
}
'''

PROG = 'rewriter'

class TestRewriterSample(SamplesTestCase):
    def test_live(self):
        self.assertSampleOutput([PROG], 'test5.c', OUTPUT)

if __name__ == '__main__':
  unittest.main()
