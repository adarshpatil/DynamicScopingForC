#!/usr/bin/python
import os, sys, unittest
from tools import SamplesTestCase

OUTPUT = '''\
#include<math.h>
#include<stdlib.h>
#include<stdio.h>
//Declaring all Variables of DynamicType
typedef struct {int type; union{float floatval;int intval;}du;} ctype;
typedef struct {int type; union{float floatval;int intval;}du;} btype;
typedef struct {int type; union{int intval;}du;} mainvartype;

ctype c; btype b; mainvartype mainvar; 

;
void func();
int main()
{ 
	//Initalizing global variables
	c.type = 0;	c.du.floatval = 2.5;
	b.type = 0;	b.du.floatval = 5.2;
	mainvar.type = -1;


	//Backup global struct before local init
	btype b0;
	b0.type = b.type; b0.du.floatval = b.du.floatval; b0.du.intval = b.du.intval; 
	b.type = 1;
	b.du.intval = 1000;

	//Backup global struct before local init
	mainvartype mainvar1;
	mainvar1.type = mainvar.type; mainvar1.du.intval = mainvar.du.intval; 
	mainvar.type = 0;
	mainvar.du.intval = 1;

	//printf("IN MAIN C:%f\\n",c.du.floatval);
	func();

	//Restoring from backup variables
	mainvar.du.intval = mainvar1.du.intval; mainvar.type = mainvar1.type;

	//Restoring from backup variables
	b.du.floatval = b0.du.floatval; 	b.du.intval = b0.du.intval; b.type = b0.type;
}
void func()
{
	//Backup global struct before local init
	ctype c0;
	c0.type = c.type; c0.du.floatval = c.du.floatval; c0.du.intval = c.du.intval; 
	c.type = 1;
	c.du.intval = 10;

	{
		//Backup global struct before local init
	btype b1;
	b1.type = b.type; b1.du.floatval = b.du.floatval; b1.du.intval = b.du.intval; 
	b.type = 1;
	b.du.intval = 5;
if (b.type==0)		b.du.floatval = pow(b.du.floatval,3);
if (b.type==1)		b.du.intval = pow(b.du.intval,3);

		//printf("INSIDE BLOCK B:%d\\n",b.du.intval);
	
		//Restoring from backup variables
		b.du.floatval = b1.du.floatval; 	b.du.intval = b1.du.intval; b.type = b1.type;
	}
	//printf("INSIDE FUNC MAINVAR:%d B:%d\\n",mainvar.du.intval,b.du.intval);if (mainvar.type==-1) {printf("ERROR: VARIABLE mainvar IS UNDEFINED AT LOC 330"); exit(1);}
if (mainvar.type==0 && c.type==0)	if(mainvar.du.intval==1)
	{
		mainvar.du.intval++;
		c.du.floatval = c.du.floatval * 2;
	};
if (mainvar.type==0 && c.type==1)	if(mainvar.du.intval==1)
	{
		mainvar.du.intval++;
		c.du.intval = c.du.intval * 2;
	};

	//printf("INSIDE FUNC C:%d\\n",c.du.intval);

	//Restoring from backup variables
	c.du.floatval = c0.du.floatval; 	c.du.intval = c0.du.intval; c.type = c0.type;
}

/* HIGHLIGHTS
LINE 16 initialization in a block
LINE 20 conditional block*/
'''

PROG = 'rewriter'

class TestRewriterSample(SamplesTestCase):
    def test_live(self):
        self.assertSampleOutput([PROG], 'test3.c', OUTPUT)

if __name__ == '__main__':
  unittest.main()
