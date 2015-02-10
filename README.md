Dynamic Scoping for C
=============

Aim
----

Compiler Design - IISc Course Project
- Implementing [Dynamic Scoping] for C using the Clang compiler front-end
- Support only int, float, array, and structure type variables. 

[Dynamic Scoping]: http://en.wikipedia.org/wiki/Scope_%28computer_science%29#Dynamic_scoping

Algorithm - Shallow Binding
----

- Shallow binding is a strategy that is considerably faster and simpler to implement than Deep binding
- We make use of simple global typedef Dynamic Structure variables to keep to track references and initialization
- Local binding is performed by saving the original value in another typedef Dynamic Structure and restoring when the local definition goes out of scope

Implementation
----
There are 2 methods to achieve this goal. In both methods we traverse and parse the Abstract Syntax Tree(AST) produced by Clang and generate modified AST. 
1. The new AST produced can be handed over to the internal compilation pipeline for further processing.
2. Produce new C source code from the AST with support for dynamic scoping that can be compiled with any C compiler.

We choose the 2<sup>nd</sup> method for this implementation here. 
- The program spits out new C code with support for Dynamic Scoping which can then be compiled and run using any C compiler. 
- The new C code is present in the PWD with the added suffix _dyn.c

How to run this code
----
- Edit the following parameters Makefile
    *  LLVM_SRC_PATH := $$HOME/llvm/llvm-3.5.0.src  <br /> This points to LLVM source which contains lib, include, tools etc.
    *  LLVM_BUILD_PATH := $$HOME/llvm/build <br/> This points to the built directory of LLVM where make was run
    *  LLVM_BIN_PATH := $(LLVM_BUILD_PATH)/Release+Asserts/bin/  <br/> This points to the location of the binary of clang within the build directory

- Once you make the modifications you can run make and make test to test the build.

Features
----
Can handle the following declarations
- Multiple declarations in a line with initializations, example:
```
int a = 20,b;
OR
int a = (10*20+30); //complex expression
```
- Multiple (and repeated) undeclared variables in a single expression or statement, example:
```
a = b + c; (where a,b and c are undeclared in current scope)
OR
a = a * b; 
````
- Global variables (can be declared anywhere in global scope) and local variables with same name but different type in different scopes, examples:
```
int a;
//statements
void f()
{
    float a = 2.5;
    //statements
}
void f1()
{
		int a = 30;
		//statements
		if(<condition)
		{
			char a;
			//statements
		}
}
``` 
- Can detect errors if variables that have never been declared anywhere in the program are used
- Can convert statemnts which have a combination of DeclRef variables and undeclaredButUsed variables


TO FIX
----
- We place our dynamic stuct type all the way on top and if suppose a type struct is defined later we will get an error!!
- Fix int a,b; in local variable declaration because of replacetext visitVarDecl()
- Use tracker function to find the next backup variable name for local vars


License
----

MIT

**Free Software, Hell Yeah!**


=============
