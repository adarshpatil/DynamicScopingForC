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

Important Notes
----
- This code runs only with LLVM and Clang 3.5 stable release
WHY? - Clang and libtooling is rapidly evolving and the API calls change often. Trying to make it run for each version is like shooting a moving target!
- Running make adds a few lines to 2 files in clang and runs a make of clang
    * include/clang/Sema/Sema.h
```
835	/// Added by Adarsh
836	/// UndeclaredIdentifiers 
837  std::vector<std::pair<DeclarationName, SourceLocation>> UndeclaredButUsed;
```
    * clang/lib/Sema/SemaExpr.cpp
```
1740  //Added by Adarsh
1741  if (diagnostic == diag::err_undeclared_var_use && 
1742	  Name.getNameKind() == DeclarationName::Identifier) {
1743	UndeclaredButUsed.push_back(std::make_pair(Name, R.getNameLoc()));
1744  }
```

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
- Works with typedef definitions of variables.

TO FIX
----
- We place our dynamic stuct type all the way on top and if suppose a type struct is defined later we will get an error!! :: (HIGH)
- including stdio.h gives error :: (HIGH)
- Remove ; from global variable declarations
- if there is a return/break from a scope our restore mechanism won't fix be able to restore global values :: (HIGH)
- no support for arrays	
- will not support single line if or for blocks without parathesis i.e. always use parantheis for if/switch/for/function blocks even if they are single line blocks
- initializations within if ( i am stupid-coding this )

Download Links
----
- http://llvm.org/releases/download.html
- http://llvm.org/releases/3.5.0/llvm-3.5.0.src.tar.xz
- http://llvm.org/releases/3.5.0/cfe-3.5.0.src.tar.xz

License
----

MIT

**Free Software, Hell Yeah!**


=============
