#!/usr/bin/python
import os, sys, unittest
from tools import SamplesTestCase

OUTPUT = '''\
#include<stdlib.h>
#include<stdio.h>
//Declaring all Variables of DynamicType
typedef struct {int type; union{float floatval;int intval;who *whoval;}du;} bvartype;
typedef struct {int type; union{who *whoval;}du;} svartype;

bvartype bvar; svartype svar; 

;
typedef struct {
	int z;
}who;
int main()
{ 
	//Initalizing global variables
	bvar.type = 0;	bvar.du.floatval = 50;
	svar.type = -1;


	//Backup global struct before local init
	bvartype bvar0;
	bvar0.type = bvar.type; bvar0.du.floatval = bvar.du.floatval; bvar0.du.intval = bvar.du.intval; 
	bvar.type = 1;


	//Restoring from backup variables
	bvar.du.floatval = bvar0.du.floatval; 	bvar.du.intval = bvar0.du.intval; bvar.type = bvar0.type;
}
void f()
{
	//Backup global struct before local init
	bvartype bvar1;
	bvar1.type = bvar.type; bvar1.du.floatval = bvar.du.floatval; bvar1.du.intval = bvar.du.intval; bvar1.du.whoval = bvar.du.whoval; 
	bvar.type = 2;
	bvar.du.whoval = malloc(sizeof( who));

//Backup global struct before local init
	svartype svar1;
	svar1.type = svar.type; svar1.du.whoval = svar.du.whoval; 
	svar.type = 0;
	svar.du.whoval = malloc(sizeof( who));

	if(bvar.type==0){printf("ERROR: EXPECTING STRUCT TYPE"); exit(1);}
	if(bvar.type==1){printf("ERROR: EXPECTING STRUCT TYPE"); exit(1);}
		bvar.du.whoval->z = svar.du.whoval->z = 20;
	if(bvar.type==0){printf("ERROR: EXPECTING STRUCT TYPE"); exit(1);}
	if(bvar.type==1){printf("ERROR: EXPECTING STRUCT TYPE"); exit(1);}
		bvar.du.whoval->z = bvar.du.whoval->z + svar.du.whoval->z;

	//Restoring from backup variables
	svar.du.whoval = svar1.du.whoval; svar.type = svar1.type;

	//Restoring from backup variables
	bvar.du.floatval = bvar1.du.floatval; 	bvar.du.intval = bvar1.du.intval; 	bvar.du.whoval = bvar1.du.whoval; bvar.type = bvar1.type;
}
'''

PROG = 'rewriter'

class TestRewriterSample(SamplesTestCase):
    def test_live(self):
        self.assertSampleOutput([PROG], 'test2.c', OUTPUT)

if __name__ == '__main__':
  unittest.main()
