import os, sys, unittest
from tools import SamplesTestCase

OUTPUT = '''\
#include<stdlib.h>
#include<stdio.h>
//Declaring all Variables of DynamicType
typedef struct {int type; union{float floatval;int intval;}du;} ctype;
typedef struct {int type; union{float floatval;int intval;}du;} btype;
typedef struct {int type; union{float floatval;int intval;}du;} mainvartype;

ctype c; btype b; mainvartype mainvar; 

;
void f();
void f1()
{
	//Backup global struct before local init
	mainvartype mainvar1;
	mainvar1.type = mainvar.type; mainvar1.du.floatval = mainvar.du.floatval; 
	mainvar.type = 0;
	mainvar.du.floatval = 1.0;

	f();

	//Restoring from backup variables
	mainvar.du.floatval = mainvar1.du.floatval; mainvar.type = mainvar1.type;
}
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

	//Backup global struct before local init
	mainvartype mainvar1;
	mainvar1.type = mainvar.type; mainvar1.du.floatval = mainvar.du.floatval; mainvar1.du.intval = mainvar.du.intval; 
	mainvar.type = 1;
	mainvar.du.intval = 1;

	f();
	f1();

	//Restoring from backup variables
	mainvar.du.floatval = mainvar1.du.floatval; 	mainvar.du.intval = mainvar1.du.intval; mainvar.type = mainvar1.type;

	//Restoring from backup variables
	b.du.floatval = b0.du.floatval; 	b.du.intval = b0.du.intval; b.type = b0.type;
}
void f()
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
	b.du.intval = 25;

if(b.type==0)		b.du.floatval = b.du.floatval * 3;
if(b.type==0)		b.du.floatval = b.du.floatval * 3;
	
		//Restoring from backup variables
		b.du.floatval = b1.du.floatval; 	b.du.intval = b1.du.intval; b.type = b1.type;
	}if (mainvar.type==-1) {printf("ERROR: VARIABLE mainvar IS UNDEFINED AT LOC 182"); exit(1);}
if (cvar.type==-1) {printf("ERROR: VARIABLE cvar IS UNDEFINED AT LOC 212"); exit(1);}
if (mainvar.type==0 && cvar.type==0)	if(mainvar.du.floatval==1)
	{
		mainvar.du.floatval++;
		cvar.du.floatval = cvar.du.floatval * 2;
	};
if (mainvar.type==0 && cvar.type==1)	if(mainvar.du.floatval==1)
	{
		mainvar.du.floatval++;
		cvar.du.intval = cvar.du.intval * 2;
	};
if (mainvar.type==1 && cvar.type==0)	if(mainvar.du.intval==1)
	{
		mainvar.du.intval++;
		cvar.du.floatval = cvar.du.floatval * 2;
	};
if (mainvar.type==1 && cvar.type==1)	if(mainvar.du.intval==1)
	{
		mainvar.du.intval++;
		cvar.du.intval = cvar.du.intval * 2;
	};


	//Restoring from backup variables
	c.du.floatval = c0.du.floatval; 	c.du.intval = c0.du.intval; c.type = c0.type;
}
'''

PROG = 'rewriter'

class TestRewriterSample(SamplesTestCase):
    def test_live(self):
        self.assertSampleOutput([PROG], 'test3.c', OUTPUT)

if __name__ == '__main__':
  unittest.main()
