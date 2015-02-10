#include <cstdio>
#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <stdlib.h>

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
#include "clang/Sema/SemaConsumer.h"
#include "clang/Sema/Sema.h"

using namespace clang;


// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor> {
	friend class ChainedConsumer;

private:
	Rewriter &TheRewriter;
	struct allVars{
	  std::string vName;
	  bool isGlobal;
	  std::string types[10];
	  int num_types = 0;
	  std::string initVal;
	  std::string initType;
	  int backupvarIndex=0;
	};
	std::vector<allVars> allVarNames;
	unsigned int globalDeclLoc;
	unsigned int globalInitLoc;
	
public:
	MyASTVisitor(Rewriter &R) : TheRewriter(R) {}

	// Override VisitDeclRefExpr - called when a declared variable is referenced
	bool VisitDeclRefExpr(const DeclRefExpr *node) {
		std::stringstream SS;
		std::string name;
		//we are interested only in VarDecl not func, enums etc.
		if ( isa<VarDecl>(node->getDecl()) ) {
			SourceLocation drLoc = node->getLocation();
			name = node->getNameInfo().getName().getAsString();
			llvm::errs()<< "DECL REF " << name << " " << drLoc.getRawEncoding() << "\n";
			//SS << "if(" << name << ".tag==1){" << name ".i";
		}
		return true;
	}
  
	// Returns true if the Variable name is present in our allVars Vector
	bool isNewVarName( std::string v ) {
		for( std::vector<allVars>::iterator it = allVarNames.begin(); it != allVarNames.end(); ++it){
			if (strcmp(v.c_str(), it->vName.c_str()) == 0)
				return false;
		}
		return true;
	}
	
	int getIndexOfVarName( std::string v) {
		for( int i = 0 ; i< allVarNames.size(); i++){
			if (strcmp(v.c_str(), allVarNames[i].vName.c_str()) == 0)
				return i;
		}
	}
	
	// loads complete Line given a location into 'returnString'
	// returns number of characters in forward lookup till end of string
	int getCompleteLine(std::string &returnString, unsigned int loc) {
		SourceManager &SM = TheRewriter.getSourceMgr();
		unsigned int backloc = loc-1;
		unsigned int forwardloc = loc;
		char pc = SM.getCharacterData ( SourceLocation::getFromRawEncoding(forwardloc)) [0] ;
		while ( pc != ';')  {
			returnString = returnString + pc;
			pc = SM.getCharacterData ( SourceLocation::getFromRawEncoding(++forwardloc)) [0];
		}
		pc = SM.getCharacterData ( SourceLocation::getFromRawEncoding(backloc)) [0] ;
		while ( pc != ';' && pc!='{' && pc!='\n' && pc!='}') {
			returnString = pc + returnString;
			pc = SM.getCharacterData ( SourceLocation::getFromRawEncoding(--backloc)) [0];
		}
		return (forwardloc - loc);
	}	
	
	// returns offset to end of scope of a variable declaration
	int getOffsetToEndScope(unsigned int loc) {
		SourceManager &SM = TheRewriter.getSourceMgr();
		std::string scopeString = SM.getCharacterData ( SourceLocation::getFromRawEncoding(loc)) ;
		//llvm::errs()<<SM.getCharacterData ( SourceLocation::getFromRawEncoding(loc)) << "\n\n";
		unsigned int offset=0;
		int nest = 1;
		while ( nest!=0 ) {
			if (scopeString[offset] == '{')
				nest++;
			if (scopeString[offset] == '}')
				nest--;
			offset++;
		}
		return offset-1;
	}
	
	int getNextbackupvarIndex(std::string v) {
		if (isNewVarName(v))
			return 1;
		int index = getIndexOfVarName(v);
		return allVarNames[index].backupvarIndex++;
	}
	
	// get components as string for given basis
	std::string getComponents(int num, std::string basis) {
		int i = basis.length()-1;
		std::string components="";
		std::string rcomponents="";
		
		while( num < basis[i] ) {
			components += std::to_string(num % (basis[i]-'0'));
			num = num / (basis[i] - '0');
			i--;
		}
		if ( i>=0 )
			while(i!=0) {
				components += "0";
				i--;
			}
			
		components+="\0";
		
		for (std::string::reverse_iterator rit=components.rbegin(); rit!=components.rend(); ++rit)
			rcomponents += *rit;
		
		return rcomponents;
	}
	
	// get Initialized value
	std::string getInitVal(unsigned int pos) {
		SourceManager &SM = TheRewriter.getSourceMgr();
		std::string initVal;
		// We try to handle both cases int a=10,b; and int a=10; here
		while ( SM.getCharacterData ( SourceLocation::getFromRawEncoding(pos) )[0] != ';' &&
				SM.getCharacterData ( SourceLocation::getFromRawEncoding(pos) )[0] != ','   )  {
					initVal = initVal + SM.getCharacterData ( SourceLocation::getFromRawEncoding(pos) )[0] ;
					pos++;
		}
		return initVal;	
	}
	
	// have we encountered this Type for Variable av
	// returns av.num_types+1 it is new type else returns index
	int typeIndexForVar(allVars av,std::string type) {
		for( int i=0; i < av.num_types;i++){
			if(std::strcmp(av.types[i].c_str(),type.c_str()) == 0)
				return i;
		}
		return (av.num_types+1);
	}
	
	// Override VisitVarDecl - called when a variable is Declared
	bool VisitVarDecl(const VarDecl *D) {
		bool isGlobal = !D->isLocalVarDecl();
		std::string type = QualType::getAsString(D->getType().split());
		
		// we do this below to accomodate definitions with struct myStruct var; replace to struct_myStruct
		std::replace( type.begin(), type.end(), ' ', '_');
		//if (types.find("struct") != std::string::npos)
		//			it->types[t].replace(6,1,"_");

		
		std::string name = D->getName();
		std::string initVal;
		
		
		if( D->hasInit() )
			initVal = getInitVal(D->getInit()->getLocStart().getRawEncoding());
		else
			initVal = "none";
		
		if ( isGlobal ) {
			//GLOBAL AND HAS INIT - STORE IN STRUCT ELSE NONE
			if( isNewVarName(D->getName()) ) {
				allVars av;
				av.vName = name;
				av.isGlobal = true;
				av.types[av.num_types] = type;
				av.num_types++;
				av.initVal = initVal;
				av.initType = type;
				
				allVarNames.push_back( av );
			}
			else {
				int index = getIndexOfVarName(name);
				if( typeIndexForVar(allVarNames[index],type) == (allVarNames[index].num_types+1) ) {
					allVarNames[index].types[allVarNames[index].num_types] = type;
					allVarNames[index].num_types++;
				}
				allVarNames[index].initVal = initVal;
				allVarNames[index].initType = type;
				allVarNames[index].isGlobal = true;
				
			}
			
			Rewriter::RewriteOptions opts;
			opts.RemoveLineIfEmpty = true;
			// TODO: Remove ; from global variables
			TheRewriter.RemoveText(D->getSourceRange(),opts);

		}
		else {
			
			// LOCAL VAR - STORE TYPE IN STRUCT AND DO THE BACKUP THING
			std::stringstream SS;
			std::string backupvar = name + std::to_string(getNextbackupvarIndex(name)); 
			if( isNewVarName(D->getName()) ) {
				allVars av;
				av.vName = name;
				av.isGlobal = false;
				av.types[av.num_types] = type;
				av.num_types++;
				av.initVal = "none";
				av.initType = "none";
				av.backupvarIndex = 1;
				allVarNames.push_back( av );
			}
			else {
				int index = getIndexOfVarName(name);
				if( typeIndexForVar(allVarNames[index],type) == (allVarNames[index].num_types+1) ) {
					allVarNames[index].types[allVarNames[index].num_types] = type;
					allVarNames[index].num_types++;
				}
				
			}
			
			SS << "//Backup global struct before local init\n";
			SS << "\t" <<name << "type " << backupvar << ";\n";
			SS << "\t" << backupvar << ".type = " << name << ".type; ";
			
			
			int index = getIndexOfVarName(name);
			int type_index = typeIndexForVar(allVarNames[index],type);
			
			for( int i = 0; i < allVarNames[index].num_types; i++) 
				SS 	<< backupvar << ".du." << allVarNames[index].types[i] << "val = " 
					<< name << ".du." <<allVarNames[index].types[i] <<"val; ";
				
			
			SS << "\t" << name << ".type = " << type_index << ";\n";
			if (D->hasInit())
				SS << "\t" << name << ".du." << allVarNames[index].types[type_index] << "val = " << initVal << ";\n";
				
			
			TheRewriter.ReplaceText(SourceRange(D->getLocStart(),
												SourceLocation::getFromRawEncoding(D->getLocEnd().getRawEncoding()+2)),
									SS.str());
									
			// Flush SS (String Stream)
			SS.str("");
			
			// Restore from backup variable at end of scope
			unsigned int offset = getOffsetToEndScope( D->getSourceRange().getBegin().getRawEncoding() );
			
			SS << "\n\t//Restoring from backup variables\n";
			for( int i = 0; i < allVarNames[index].num_types; i++)
				SS 	<< "\t" << name << ".du." << allVarNames[index].types[i] << "val = " 
					<< backupvar << ".du." <<allVarNames[index].types[i] <<"val; ";	
			SS << "\n";
			
			TheRewriter.InsertText(D->getSourceRange().getBegin().getLocWithOffset(offset), 
									SS.str(), false, true);
		}
		
		return true;
	}

	void dispVar() {
		llvm::errs() << "ALL VAR NAMES\n";
		for( std::vector<allVars>::iterator it = allVarNames.begin(); it != allVarNames.end(); ++it) {
			llvm::errs() << it->vName << " " << it->initVal << " " << it->isGlobal << " TYPES ARE:";
			for ( int i = 0; i < it->num_types; i++)
				llvm::errs() << it->types[i] << " ";
			llvm::errs() << "\n";
		}
		llvm::errs() << "\n";
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
	
	
	void addGlobalDecl() {
		std::stringstream SSDecl;
		std::stringstream SSStruct;
		std::stringstream SSInit;
		SourceLocation glocalDecl = SourceLocation::getFromRawEncoding(globalDeclLoc);
		SourceLocation globalInit = SourceLocation::getFromRawEncoding(globalInitLoc);
		
		SSStruct << "//Declaring all Variables of DynamicType\n";
		SSInit << " \n\t//Initalizing global variables\n";
		
		for( std::vector<allVars>::iterator
				it = allVarNames.begin(); it != allVarNames.end(); ++it) {
			SSStruct << "typedef struct {int type; union{";
			for(int t = 0; t<(it->num_types); t++ ) {
				// replace _ to " " if this is a struct type for declaration and then restore back
				if (it->types[t].find("struct") != std::string::npos)
					it->types[t].replace(6,1," "); 
				SSStruct << it->types[t]; 
				std::replace( it->types[t].begin(), it->types[t].end(), ' ', '_');
				SSStruct << " " << it->types[t] <<"val;";
			}
			SSStruct << "};} " << it->vName << "type;\n";	
			SSDecl << it->vName << "type " << it->vName << "; " ;
			
			if ( it->isGlobal ) {
				SSInit << "\t" << it->vName << ".type = " << typeIndexForVar((*it),it->initType) << ";";
				if ( strcmp(it->initVal.c_str(),"none") != 0)
					SSInit << "\t" << it->vName << ".du." << it->initType << "val = " << it->initVal << ";\n";
			}
			else
				SSInit << "\t" << it->vName << ".type = -1;\n";
			
		}
		SSStruct << "\n";
		SSDecl << "\n\n";
		SSInit << "\n";
		

		TheRewriter.InsertText(glocalDecl, SSStruct.str() + SSDecl.str(), true, true);
		TheRewriter.InsertText(globalInit, SSInit.str(), true, true); 
		
	}
  
	void insertDynamicTypeStruct(SourceLocation ST){
		std::stringstream SST;
		globalDeclLoc = ST.getRawEncoding();
	}
  
};

// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class ChainedConsumer : public SemaConsumer {
public:
	ChainedConsumer(Rewriter &R) : Visitor(R) { }

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
  
	void InitializeSema(Sema& S) {
		mSema = &S;
	}
	void showVarNames(){
		Visitor.dispVar();
	}
  
	void addGlobalVarDecl(){
		Visitor.addGlobalDecl();
	}

	void replaceAll(std::string& str, const std::string& from, const std::string& to) {
		size_t start_pos = 0;
		while((start_pos = str.find(from, start_pos)) != std::string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length();
		}
	}
	
	void renameUndefinedButUsed() {
		
		for (std::vector<std::pair<DeclarationName, SourceLocation>>::iterator
				I = mSema->UndeclaredButUsed.begin(), E = mSema->UndeclaredButUsed.end();
				I != E;) {
			llvm::errs() << "UNDEFINED BUT USED " << I->first.getAsString() << " " << I->second.getRawEncoding() << "\n";
			if ( Visitor.isNewVarName(I->first.getAsString()) ) {
				llvm::errs() << "ERROR: UNDEFINED IDENTIFIER " << I->first.getAsString() << " AT LOC " 
							<< I->second.getRawEncoding() <<"\n";
				I++;
			}
			else {
				std::string errorLine="";
				std::stringstream SS;
				int forwardLocToEnd = Visitor.getCompleteLine(errorLine, I->second.getRawEncoding());
				int varCtr = 1;
				
				llvm::errs()<< "LINE :" << errorLine << " FOWARD: " << forwardLocToEnd << "\n";
				
				// Handle multiple undeclared variables in statements
				// Check if there are multiple errors on this line
				for (std::vector<std::pair<DeclarationName, SourceLocation>>::iterator
						K = (I+1); K != E; ++K)
					if ( K->second.getRawEncoding() <=  (I->second.getRawEncoding()+forwardLocToEnd) )
						varCtr++;

				// Generate if-else ladder
				std::vector<std::string> parsedVar;  // stores all unique variables encountered in line
				for( int i = 0;i<varCtr; i++) {
					if ( std::find( parsedVar.begin(), 
									parsedVar.end(), 
									(I+i)->first.getAsString() ) == parsedVar.end()) {
						parsedVar.push_back( (I+i)->first.getAsString() );
						SS 	<< "if (" << (I+i)->first.getAsString() << ".type==-1) {"
							<< "printf(\"ERROR: VARIABLE " << (I+i)->first.getAsString() << " IS UNDEFINED AT LOC " 
							<< (I+i)->second.getRawEncoding() <<"\"); exit(1);}\n";
						
					}
				}
				
				
				int combinations = 1;
				std::string basis = "";
				std::string components;
				std::string replacedString;
				std::string errorLineOrg;
				for( int i = 0;i< parsedVar.size();i++) {
					combinations *= Visitor.allVarNames[Visitor.getIndexOfVarName(parsedVar[i])].num_types;
					basis += std::to_string(Visitor.allVarNames[Visitor.getIndexOfVarName(parsedVar[i])].num_types);
				}
				
				llvm::errs() << "COMBINATIONS: " << combinations << "\n";
				
				llvm::errs() << "DEBUG: " << varCtr << " " << errorLine.length() << "\n";
				
				
				for ( int i = 0;i < combinations;i++) {
					components = Visitor.getComponents(i,basis);
					llvm::errs() << basis << " " << components << " " << parsedVar.size() << "\n";
					errorLineOrg.assign(errorLine);
					SS << "if (" ;
					for ( int j = 0;j<parsedVar.size();j++) {
						replacedString =parsedVar[j] + ".du." +
										Visitor.allVarNames[Visitor.getIndexOfVarName(parsedVar[j])].types
											[components[j]-'0'] + "val";
						
						
						replaceAll(errorLine, parsedVar[j], replacedString);
						//std::replace(errorLine.begin(), errorLine.end(), parsedVar[i], replacedString );
						
						SS 	<< parsedVar[j] << ".type==" << components[j]; 
						if((j+1) < parsedVar.size())
							SS << " && ";
						
					}
					SS << ")" << errorLine << ";\n";
					errorLine.assign(errorLineOrg);
				}
				
				
				//Replace text from the program
				unsigned int beginLoc = I->second.getRawEncoding();
				beginLoc = beginLoc - (errorLine.length() - forwardLocToEnd);
				Visitor.TheRewriter.ReplaceText(SourceLocation::getFromRawEncoding(beginLoc), errorLine.length()+1, SS.str());
				
				// Skip distinctVarCtr iterations ahead
				I = I + varCtr;
				
			} 
		}
	}
	
private:
	MyASTVisitor Visitor;
	int first_visit_flag = 0;
	Sema* mSema;
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
	TranslationUnitKind tuk = TU_Module;
	
	// Initialize target info with the default triple for our platform.
	auto TO = std::make_shared<TargetOptions>();
	TO->Triple = llvm::sys::getDefaultTargetTriple();
	TargetInfo *TI = TargetInfo::CreateTargetInfo(TheCompInst.getDiagnostics(), TO);
	TheCompInst.setTarget(TI);

	TheCompInst.createFileManager();
	FileManager &FileMgr = TheCompInst.getFileManager();
	TheCompInst.createSourceManager(FileMgr);
	SourceManager &SourceMgr = TheCompInst.getSourceManager();
	TheCompInst.createPreprocessor(tuk);
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
	ChainedConsumer TheConsumer(TheRewriter);
	
	ASTConsumer *astConsumer = (ASTConsumer *)&TheConsumer;
	
	//Initialize Sema S
	Sema mySema(TheCompInst.getPreprocessor(),TheCompInst.getASTContext(),*astConsumer,tuk,NULL);
	TheConsumer.InitializeSema(mySema);
	

	ParseAST(mySema,false,false);
	//TheConsumer.showVarNames();
	
	TheConsumer.addGlobalVarDecl();
	TheConsumer.renameUndefinedButUsed();
	
	// At this point the rewriter's buffer should be full with the rewritten
	// file contents.
	const RewriteBuffer *RewriteBuf =
      TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());
	llvm::outs() << std::string(RewriteBuf->begin(), RewriteBuf->end());

	return 0;
}
