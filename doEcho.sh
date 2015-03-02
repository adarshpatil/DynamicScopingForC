#!/bin/bash
[ -f .clangdone ] && echo "clang done"; 
[ -f .clangdone ] && exit 0;
MYPWD=`pwd`
INCFILE="$1/tools/clang/include/clang/Sema/Sema.h"
LIBFILE="$1/tools/clang/lib/Sema/SemaExpr.cpp"
echo "==============="
echo "INCLUDE FILE " $INCFILE
echo "LIB FILE" $LIBFILE
echo "PWD" $MYPWD
echo "CLANG DIR" $2
echo "================"


head -n 1739 $LIBFILE > $LIBFILE.new
echo "  //Added by Adarsh
  if (diagnostic == diag::err_undeclared_var_use && 
	  Name.getNameKind() == DeclarationName::Identifier) {
	UndeclaredButUsed.push_back(std::make_pair(Name, R.getNameLoc()));
  } " >> $LIBFILE.new
tail -n 11795 $LIBFILE >> $LIBFILE.new
mv $LIBFILE.new $LIBFILE

head -n 834 $INCFILE > $INCFILE.new
echo "  /// Added by Adarsh
  /// UndeclaredIdentifiers 
  std::vector<std::pair<DeclarationName, SourceLocation>> UndeclaredButUsed;
" >> $INCFILE.new
tail -n 7671 $INCFILE >> $INCFILE.new
mv $INCFILE.new $INCFILE

echo "Running make on clang"
cd $2
make -j `nproc`
cd $MYPWD
touch .clangdone
