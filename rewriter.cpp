#include <cstdio>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;


// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
public:
	MyASTVisitor(Rewriter &R) : TheRewriter(R) {}

	bool VisitStmt(Stmt *s) {
		return true;
	}
  
	bool isNewVarName( std::string v ){
		for( std::vector<allVars>::iterator it = allVarNames.begin(); it != allVarNames.end(); ++it){
			if (strcmp(v.c_str(), it->vName.c_str()) == 0)
				return false;
		}
		return true;
	  
	}
  
	bool VisitVarDecl(const VarDecl *D) {
		bool isGlobal = !D->isLocalVarDecl();
		std::string type = QualType::getAsString(D->getType().split());
		std::string name = D->getName();
		std::string initVal;
		
		if ( isGlobal ) {
			//GLOBAL AND HAS INIT - STORE IN STRUCT ELSE NONE
			if( D->hasInit()) {
				SourceManager &SM = TheRewriter.getSourceMgr();
				unsigned int i = D->getInit()->getLocStart().getRawEncoding();
				while ( SM.getCharacterData ( SourceLocation::getFromRawEncoding(i) )[0] != ';' &&
						SM.getCharacterData ( SourceLocation::getFromRawEncoding(i) )[0] != ','   )  {
							llvm::errs() << SM.getCharacterData ( SourceLocation::getFromRawEncoding(i))[0];
							initVal = initVal + SM.getCharacterData ( SourceLocation::getFromRawEncoding(i) )[0] ;
							i++;
				}			
			}
			else 
				initVal = "none";
	
			if( isNewVarName(D->getName()) ) {
				allVarNames.push_back( {name,isGlobal,type,initVal} );
			}
			else {
				for( std::vector<allVars>::iterator it = allVarNames.begin(); it != allVarNames.end(); ++it){
					if (strcmp(name.c_str(), it->vName.c_str()) == 0){
						(*it).type = type;
						(*it).initVal = initVal;
						(*it).isGlobal = true;
					}
				}
			}
			Rewriter::RewriteOptions opts;
			opts.RemoveLineIfEmpty = true;
			TheRewriter.RemoveText(SourceRange(D->getLocStart(),D->getLocEnd()),opts);
		}
		else {
			std::stringstream SS;
			std::string backupvar = name + "1";  //TODO use a getNextNum() instead of 1
			if( isNewVarName(D->getName()) ) 
				allVarNames.push_back( {name,isGlobal,"none","none"} );
				
			SS << "DT " << backupvar << "\n";
			SS << backupvar << ".type = " << name << ".type\n";
			
			if ( strcmp(type.c_str(),"int") == 0 )
				SS << name << ".type = 1\n";
			if ( strcmp(type.c_str(),"float") == 0)
				SS << name << ".type = 2\n";
		}
		
		
		return true;
	}

	void dispVar(){
		llvm::errs() << "ALL VAR NAMES\n";
		for( std::vector<allVars>::iterator it = allVarNames.begin(); it != allVarNames.end(); ++it)
			llvm::errs() << it->vName << " " << it->type  << " " << it->initVal << " " << it->isGlobal;
		llvm::errs() << "\n";
	}
  
	void addGlobalDecl(){
		std::stringstream SSDecl;
		std::stringstream SSInit;
		SourceLocation glocalDecl = SourceLocation::getFromRawEncoding(globalDeclLoc);
		SourceLocation globalInit = SourceLocation::getFromRawEncoding(globalInitLoc);
		
		SSDecl << "//Declaring all Variables of DynamicType\n";
		SSInit << " \n\t//Initalizing global variables\n";
		
		for( std::vector<allVars>::iterator it = allVarNames.begin(); it != allVarNames.end(); ++it){
			SSDecl << "DT " << it->vName << "; ";
			if ( it->isGlobal ){
				if ( strcmp(it->type.c_str(),"int") == 0 ){
					SSInit << "\t" << it->vName << ".type = 1;\n";
					if ( strcmp(it->initVal.c_str(),"none") )
						SSInit << "\t" << it->vName << ".i = " << it->initVal << ";\n";
				}
				if ( strcmp(it->type.c_str(),"float") == 0 ){
					SSInit << "\t" << it->vName << ".type = 2;\n";
					if ( strcmp(it->initVal.c_str(),"none") )
						SSInit << "\t" << it->vName << ".f = " << it->initVal << ";\n";
				}
				
			}
			else{
				SSInit << "\t" << it->vName << ".type = 0;\n";
			}
		}
		SSDecl << "\n\n";
		TheRewriter.InsertText(glocalDecl, SSDecl.str(), true, true);
		TheRewriter.InsertText(globalInit, SSInit.str(), true, true);  
	}
  
	void insertDynamicTypeStruct(SourceLocation ST){
		std::stringstream SST;
		globalDeclLoc = ST.getRawEncoding();
		SST	<< "typedef struct DynamicType\n"
			<< "{\n"
			<< "\tunion DynamicUnion { int i; float f; }du;\n"
			<< "\tint type;\n"
			<< "}DT;\n";
			
		TheRewriter.InsertText(ST, SST.str(), true, true);
	}
  
	bool VisitFunctionDecl(FunctionDecl *f) {
		// Only function definitions (with bodies), not declarations.
		if (f->hasBody()) {
			Stmt *FuncBody = f->getBody();

			// Type name as string
			QualType QT = f->getReturnType();
			std::string TypeStr = QT.getAsString();

			// Function name
			DeclarationName DeclName = f->getNameInfo().getName();
			std::string FuncName = DeclName.getAsString();

			if ( f->isMain() ) {
				globalInitLoc = f->getBody()->getLocStart().getRawEncoding();
				globalInitLoc++;
			}
			
		}

		return true;
	}
  
  
  
private:
	Rewriter &TheRewriter;
	struct allVars {
	  std::string vName;
	  bool isGlobal;
	  std::string type;
	  std::string initVal;
	};
	std::vector<allVars> allVarNames;
	unsigned int globalDeclLoc;
	unsigned int globalInitLoc;	
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer {
public:
	MyASTConsumer(Rewriter &R) : Visitor(R) {}

	// Override the method that gets called for each parsed top-level
	// declaration.
	virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
		for (DeclGroupRef::iterator b = DR.begin(), e = DR.end(); b != e; ++b){
			//llvm::errs() << "\n\n******TOP LEVEL DECL******\n";
			//(*b)->dump();
			//llvm::errs() << "\n";
			if ( first_visit_flag == 0 ) {
				first_visit_flag  = 1;
				SourceLocation ST = (*b)->getLocStart();
				Visitor.insertDynamicTypeStruct(ST);
			}
			// Traverse the declaration using our AST visitor.
			Visitor.TraverseDecl(*b);
		}
		return true;
	}
  
	void showVarNames(){
		Visitor.dispVar();
	}
  
	void addGlobalVarDecl(){
		Visitor.addGlobalDecl();
	}
  
private:
	MyASTVisitor Visitor;
	int first_visit_flag = 0;
};

int main(int argc, char *argv[]) {
	if (argc != 2) {
		llvm::errs() << "Usage: rewritersample <filename>\n";
		return 1;
	}

	// CompilerInstance will hold the instance of the Clang compiler for us,
	// managing the various objects needed to run the compiler.
	CompilerInstance TheCompInst;
	TheCompInst.createDiagnostics();

	LangOptions &lo = TheCompInst.getLangOpts();
	lo.CPlusPlus = 1;

	// Initialize target info with the default triple for our platform.
	auto TO = std::make_shared<TargetOptions>();
	TO->Triple = llvm::sys::getDefaultTargetTriple();
	TargetInfo *TI = TargetInfo::CreateTargetInfo(TheCompInst.getDiagnostics(), TO);
	TheCompInst.setTarget(TI);

	TheCompInst.createFileManager();
	FileManager &FileMgr = TheCompInst.getFileManager();
	TheCompInst.createSourceManager(FileMgr);
	SourceManager &SourceMgr = TheCompInst.getSourceManager();
	TheCompInst.createPreprocessor(TU_Module);
	TheCompInst.createASTContext();

	// A Rewriter helps us manage the code rewriting task.
	Rewriter TheRewriter;
	TheRewriter.setSourceMgr(SourceMgr, TheCompInst.getLangOpts());

	// Set the main file handled by the source manager to the input file.
	const FileEntry *FileIn = FileMgr.getFile(argv[1]);
	SourceMgr.setMainFileID(
      SourceMgr.createFileID(FileIn, SourceLocation(), SrcMgr::C_User));
	TheCompInst.getDiagnosticClient().BeginSourceFile(
      TheCompInst.getLangOpts(), &TheCompInst.getPreprocessor());

	// Create an AST consumer instance which is going to get called by
	// ParseAST.
	MyASTConsumer TheConsumer(TheRewriter);

	// Parse the file to AST, registering our consumer as the AST consumer.
	ParseAST(TheCompInst.getPreprocessor(), &TheConsumer,
           TheCompInst.getASTContext());

           
	//TheConsumer.showVarNames();
	
	TheConsumer.addGlobalVarDecl();
	// At this point the rewriter's buffer should be full with the rewritten
	// file contents.
	const RewriteBuffer *RewriteBuf =
      TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());
	llvm::outs() << std::string(RewriteBuf->begin(), RewriteBuf->end());

	return 0;
}
