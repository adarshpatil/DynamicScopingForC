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
#include "clang/Lex/Token.h"

#define DEBUG 1      // set debug to 0 to turn off debug messages on stderr
#define dbg(...) if(DEBUG != 0) { \
	llvm::errs() << __VA_ARGS__; }
using namespace clang;

#define die(...) \
	llvm::errs() << __VA_ARGS__; \
	exit(1)

Sema *globalSema;

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
	std::vector<std::pair<std::string, SourceLocation>> visitedDeclRef;
	std::map<std::string, std::string> typedefDecl;
	std::vector<std::pair<unsigned int, unsigned int>> blockBoundry;
	
public:
	MyASTVisitor(Rewriter &R) : TheRewriter(R) {}
	
	// Map from SourceLocation to rewriter stringstream
	std::map<SourceLocation, std::string> rewriterDeclRef;
	std::map<SourceLocation, std::string> rewriterVarDecl;
	
	
	// Override VisitTypedefDecl - called when a typedef variable is declared
	bool VisitTypedefDecl(clang::TypedefDecl *d) {
		typedefDecl[d->getName()] = QualType::getAsString(d->getUnderlyingType().split());
		dbg("TYPEDEF DECL: " << d->getName() << " " << typedefDecl[d->getName()] << "\n");
		return true;
	}
	
	// checks if there is a "." or "->" after this location to check if this is expecting struct access
	// returns true if there is else false
	bool isStructAccess(unsigned int loc) {
		SourceManager &SM = TheRewriter.getSourceMgr();
		char p = SM.getCharacterData ( SourceLocation::getFromRawEncoding(loc)) [0];
		while(p == ' ')
			p = SM.getCharacterData ( SourceLocation::getFromRawEncoding(loc++)) [0];
			
		if(p == '.')
			return true;
		else if ( p == '-' &&
				SM.getCharacterData ( SourceLocation::getFromRawEncoding(loc)) [0] == '>' )
			return true;
		return false;
	}
	
	// Override VisitDeclRefExpr - called when a declared variable is referenced
	bool VisitDeclRefExpr(const DeclRefExpr *node) {
		std::stringstream SS;
		std::string name;
		//we are interested only in VarDecl not func, enums etc.
		if ( isa<VarDecl>(node->getDecl()) ) {
			SourceLocation drLoc = node->getLocation();
			name = node->getNameInfo().getName().getAsString();
			
			std::string declrefLine;
			int forwardLocToEnd,back;
			std::vector<std::string> parsedVar;
			std::stringstream SS;
			
			dbg( "DECL REF " << name << " " << drLoc.getRawEncoding() << "\n");
			forwardLocToEnd = getCompleteLine(declrefLine,drLoc.getRawEncoding());
			back = declrefLine.length()-forwardLocToEnd;
			
			dbg("DECL REF LINE: " << declrefLine << "\n");
			
			
			// replace variable names and generate if-else ladder	
			int structaccessflag = 0;
			int combinations = 1;
			std::string basis = "";
			std::string components;
			std::string replacedString;
			std::string declrefLineOrg;
			
			// check if we encountered DeclRefExpr earlier in this line
			// return if this was already processed
			// all visited declref are stored in visitedDeclRef
			for (std::vector<std::pair<std::string, SourceLocation>>::iterator
					I = visitedDeclRef.begin(), E = visitedDeclRef.end();
					I != E;I++) {
				if ( drLoc.getRawEncoding()-back < I->second.getRawEncoding() ) {
					// get only unique names i.e a=a+b gives only a,b
					if ( strcmp(name.c_str(),I->first.c_str()) == 0)
						return true;
					
					// if variable in statement is expecting struct type
					// generate error message for all other types except stuct for that variable
					if( isStructAccess(I->second.getRawEncoding()+I->first.length()) ) {
						int structtypeindex;
						for ( int k=0;k<allVarNames[getIndexOfVarName(I->first)].num_types;k++ ) {
							if (allVarNames[getIndexOfVarName(I->first)].types[k].find("struct") != std::string::npos || 
								typedefDecl[allVarNames[getIndexOfVarName(I->first)].types[k]].find("struct") != std::string::npos) {
								structtypeindex = k;
								continue;
							}
							SS 	<< "if(" << I->first << ".type==" << k << "){"
								<< "printf(\"ERROR: EXPECTING STRUCT TYPE\"); exit(1);}\n\t";
						}
						replacedString = I->first + ".du." + 
										allVarNames[getIndexOfVarName(I->first)].types[structtypeindex] + "val->";
						replaceAll(declrefLine, I->first+".", replacedString); 
					}
					else 
						parsedVar.push_back(I->first);
				}
			}
			
				
			// push into vector as this is encountered first time
			visitedDeclRef.push_back(std::make_pair(name, drLoc));
			
				
			if ( isStructAccess(drLoc.getRawEncoding()+name.length()) ) {
				structaccessflag = 1;
				int structtypeindex;
				for ( int k=0;k<allVarNames[getIndexOfVarName(name)].num_types;k++ ) {
					if (allVarNames[getIndexOfVarName(name)].types[k].find("struct") != std::string::npos || 
						typedefDecl[allVarNames[getIndexOfVarName(name)].types[k]].find("struct") != std::string::npos) {
							structtypeindex = k;
							continue;
					}
					SS 	<< "if(" << name << ".type==" << k << "){"
						<< "printf(\"ERROR: EXPECTING STRUCT TYPE\"); exit(1);}\n\t";
				}
				replacedString = name + ".du." + 
								allVarNames[getIndexOfVarName(name)].types[structtypeindex] + "val->";
				replaceAll(declrefLine, name+".", replacedString);
			}
			
			if(parsedVar.size() == 0 && structaccessflag ) {
				SS << declrefLine << ";\n";
				rewriterDeclRef[ SourceLocation::getFromRawEncoding( drLoc.getRawEncoding()-back+1 ) ] = SS.str();
				return true;
			}
			
			parsedVar.push_back(name);
			
			dbg("Unique LENGTH: " << parsedVar.size() << "\n");	
					
				
			for( int i = 0;i< parsedVar.size();i++) {
				combinations *= allVarNames[getIndexOfVarName(parsedVar[i])].num_types;
				basis += std::to_string(allVarNames[getIndexOfVarName(parsedVar[i])].num_types);
			}
				
				
			dbg( "COMBINATIONS: " << combinations <<  " BASIS " << basis << "\n");
				
				
				
			for ( int i = 0;i < combinations;i++) {
				components.assign("");
				components = getComponents(i,basis);
				dbg( basis << " " << components << " " << parsedVar.size() << "\n");
				declrefLineOrg.assign(declrefLine);
				
				SS << "if(" ;
				for ( int j = 0;j<parsedVar.size();j++) {
					if( allVarNames[getIndexOfVarName(parsedVar[j])].types[components[j]-'0'].find("struct") != std::string::npos ||
						typedefDecl[allVarNames[getIndexOfVarName(parsedVar[j])].types[components[j]-'0']].find("struct") != std::string::npos )
						continue;
						
					replacedString =parsedVar[j] + ".du." +
									allVarNames[getIndexOfVarName(parsedVar[j])].types
										[components[j]-'0'] + "val";
						
						
					replaceAll(declrefLine, parsedVar[j], replacedString);
						
					SS 	<< parsedVar[j] << ".type==" << components[j];
							
					if((j+1) < parsedVar.size())
						SS << " && ";
						
				}
				SS << ")" << declrefLine << ";\n";
				declrefLine.assign(declrefLineOrg);
			}
				
			
			rewriterDeclRef[ SourceLocation::getFromRawEncoding( drLoc.getRawEncoding()-back+1 ) ] = SS.str();
			
			
		}
		return true;
	}

	void replaceAll(std::string& str, const std::string& from, const std::string& to) {
		size_t start_pos = 0;
		while((start_pos = str.find(from, start_pos)) != std::string::npos) {
			str.replace(start_pos, from.length(), to);
			start_pos += to.length();
		}
	}
	
	Rewriter & getRewriter() {
		return TheRewriter;
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
		int blockflag = 0;
		while ( pc != ';')  {
			
			if ( pc == '{' ) {
				blockflag = 1;
				while ( pc != '}') {
					returnString = returnString + pc;
					pc = SM.getCharacterData ( SourceLocation::getFromRawEncoding(++forwardloc)) [0];
				}
				returnString = returnString + pc;
			}
			
			if (blockflag == 1) 
				break;
			returnString = returnString + pc;
			pc = SM.getCharacterData ( SourceLocation::getFromRawEncoding(++forwardloc)) [0];
		}
		pc = SM.getCharacterData ( SourceLocation::getFromRawEncoding(backloc)) [0] ;
		while ( pc != ';' && pc!='{' && pc!='\n' && pc!='}') {
			returnString = pc + returnString;
			pc = SM.getCharacterData ( SourceLocation::getFromRawEncoding(--backloc)) [0];
		}
		if (blockflag  == 1) {
			dbg("COMPLETE BLOCK" << returnString);
			blockBoundry.push_back( std::make_pair(backloc,forwardloc) );
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
		
		while( num < basis[i] && i>=0) {
			components += std::to_string(num % (basis[i]-'0'));
			num = num / (basis[i] - '0');
			i--;
			dbg("1value of i: "<< i << "\n");
		}
		
		if ( i>=0 ) {
			components += "0";
			dbg("2value of i: "<< i << "\n");
			while(i!=0) {
				components += "0";
				i--;
			}
		}
		
		components+="\0";
		dbg("getComponents :" << components << "\n");
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
		bool isParam = false;
		std::string type = QualType::getAsString(D->getType().split());
		
		// we do this below to accomodate definitions with struct myStruct var; replace to struct_myStruct
		std::replace( type.begin(), type.end(), ' ', '_');
		
		std::string name = D->getName();
		std::string initVal;
		
		dbg("PARAM VAR: " << name << " global: " << isGlobal << " param: " << isParam <<"\n");
		
		const DeclContext *DC = D->getLexicalDeclContext()->getRedeclContext();
		// only function declarations not definitions
		if(DC->isFunctionOrMethod() && (D->getKind() == Decl::ParmVar)) {
			FunctionDecl *FD = FunctionDecl::castFromDeclContext(DC);
			if(FD->hasBody()) {
				isGlobal = false;
				isParam = true;
			}
		}
		
		
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
				
			
			SS << "\n\t" << name << ".type = " << type_index << ";\n";
			if (D->hasInit()) {
				dbg(" QUALIFIED NAME: " << D->getQualifiedNameAsString() << "\n");
				// check if its struct type
				if( type.find("struct") != std::string::npos ) {
					type.replace(6,1," ");
					SS 	<< "\t" << name << ".du." << allVarNames[index].types[type_index] 
						<< "val = malloc(sizeof( " << type << "));\n";
					std::replace( type.begin(), type.end(), ' ', '_');
				}
				
				// check if its typedef struct type
				else if( typedefDecl[type].find("struct") != std::string::npos ) 
					SS 	<< "\t" << name << ".du." << allVarNames[index].types[type_index] 
						<< "val = malloc(sizeof( " << type << "));\n";
				
				else 
					SS << "\t" << name << ".du." << allVarNames[index].types[type_index] << "val = " << initVal << ";\n";
			}
			
			SS << "\n";
			if( isParam ) {
				const DeclContext *DC = D->getLexicalDeclContext()->getRedeclContext();
				FunctionDecl *f = FunctionDecl::castFromDeclContext(DC);
				SourceLocation startLoc = SourceLocation::getFromRawEncoding(f->getBody()->getLocStart().getRawEncoding()+1);
				TheRewriter.InsertText(startLoc,SS.str(),true,true);
				TheRewriter.RemoveText(D->getSourceRange());				
			}
				
			else if ( rewriterVarDecl.find(D->getLocStart()) != rewriterVarDecl.end() )
				rewriterVarDecl[ D->getLocStart() ] += SS.str();
			else
				rewriterVarDecl[ D->getLocStart() ] = SS.str();
				

			SS.str("");
			
			// Restore from backup variable at end of scope
			unsigned int  offset;
			SourceLocation insertLoc;
			if ( isParam ) {
				const DeclContext *DC = D->getLexicalDeclContext()->getRedeclContext();
				FunctionDecl *f = FunctionDecl::castFromDeclContext(DC);	
				offset = getOffsetToEndScope(f->getBody()->getLocStart().getRawEncoding()+1);
				insertLoc = f->getBody()->getLocStart().getLocWithOffset(offset);
			}
			else {
				offset = getOffsetToEndScope( D->getSourceRange().getBegin().getRawEncoding() );
				insertLoc = D->getSourceRange().getBegin().getLocWithOffset(offset);
			}
			
			SS << "\n\t//Restoring from backup variables\n";
			for( int i = 0; i < allVarNames[index].num_types; i++)
				SS 	<< "\t" << name << ".du." << allVarNames[index].types[i] << "val = " 
					<< backupvar << ".du." <<allVarNames[index].types[i] <<"val; ";	
			SS << name << ".type = " << backupvar << ".type;\n";
			
			
			TheRewriter.InsertText(insertLoc, SS.str(), false, true);
		
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
		
		SSStruct << "#include<stdlib.h>\n";
		SSStruct << "#include<stdio.h>\n";
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
				if (it->types[t].find("struct") != std::string::npos || 
					typedefDecl[it->types[t]].find("struct") != std::string::npos)
					SSStruct << " *" << it->types[t] <<"val;";
				else
					SSStruct << " " << it->types[t] <<"val;";
			}
			SSStruct << "}du;} " << it->vName << "type;\n";	
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
		dbg(" GLOBAL DECL LOC " << globalDeclLoc << "\n");
		if(globalDeclLoc != 2) {
			char typedefcheck[9];
			SourceManager &SM = TheRewriter.getSourceMgr();
			std::memcpy(typedefcheck, SM.getCharacterData(SourceLocation::getFromRawEncoding(globalDeclLoc-8)),8);
			typedefcheck[8]='\0';
			dbg(typedefcheck<<":\n");
			if( !strcmp(typedefcheck,"typedef ")  )
				globalDeclLoc -= 8;

		}
	}
	
	bool isHeaderFunc(std::string func) {
		std::vector<std::string> headerFuncs{"pow", "sqrt", "printf"};
		for( int i =0;i<headerFuncs.size();i++)
			if(!strcmp(func.c_str(),headerFuncs[i].c_str()))
				return true;
		return false;
	}
	
	bool isprintf(std::string func) {
		if(!strcmp( func.c_str(),"printf") )
			return true;
		return false;
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
	
	
	void doPostRewrite()
	{
		// rewrite VisitDeclRef strings into rewrite buffer
		for( std::map<SourceLocation, std::string>::iterator
				I = Visitor.rewriterDeclRef.begin(), E = Visitor.rewriterDeclRef.end();
				I != E; I++) {
					std::string line;
					Visitor.getCompleteLine(line,I->first.getRawEncoding());
					Visitor.TheRewriter.ReplaceText(I->first, line.length()+1 , I->second);
					dbg( "Rewriting VisitDeclRef SL: " << I->first.getRawEncoding() << " " << I->second << "\n");
		}
		
		//rewrite VisitVarDecl strings into rewrite buffer
		for( std::map<SourceLocation, std::string>::iterator
				I = Visitor.rewriterVarDecl.begin(), E = Visitor.rewriterVarDecl.end();
				I != E; I++) {
					std::string line;
					Visitor.getCompleteLine(line,I->first.getRawEncoding());
					Visitor.TheRewriter.ReplaceText(I->first, line.length()+1 , I->second);
					dbg( "Rewriting VisitVarDecl SL: " << I->first.getRawEncoding() << " " << I->second << "\n");
		}
		
	}
	
	// Rename the undefined variables to corresponding dynamic structs
	void renameUndefinedButUsed() {
		
		doPostRewrite();
		
		for (std::vector<std::pair<DeclarationName, SourceLocation>>::iterator
				I = mSema->UndeclaredButUsed.begin(), E = mSema->UndeclaredButUsed.end();
				I != E;) {
			dbg( "UNDEFINED BUT USED " << I->first.getAsString() << " " << I->second.getRawEncoding() << "\n");
	
			if ( Visitor.isNewVarName(I->first.getAsString()) && !Visitor.isHeaderFunc(I->first.getAsString()) ) {
				die("ERROR: UNDEFINED IDENTIFIER " << I->first.getAsString() << " AT LOC " 
						<< I->second.getRawEncoding() <<"\n");
			}	
			else {
				std::string errorLine="";
				std::stringstream SS;
				int forwardLocToEnd = Visitor.getCompleteLine(errorLine, I->second.getRawEncoding());
				int varCtr = 1;
				int errorLineLength;
				
				dbg("LINE :" << errorLine << " FOWARD: " << forwardLocToEnd << "\n");
				errorLineLength = errorLine.length();
				
				// Handle multiple undeclared variables in statements
				// Check if there are multiple errors on this line
				int structtypeindex;
				std::string replacedString;
				std::string errorLineStruct;
				errorLineStruct.assign(errorLine);
				
				for (std::vector<std::pair<DeclarationName, SourceLocation>>::iterator K = (I+1); K != E; ++K) {
					if ( K->second.getRawEncoding() <=  (I->second.getRawEncoding()+forwardLocToEnd) )
						varCtr++;
				}
				
				dbg("VARCTR: " << varCtr << "\n");
				// Generate if-else ladder
				std::vector<std::string> parsedVar;  // stores all unique variables encountered in line
				for( int i = 0;i<varCtr; i++) {
					if ( std::find( parsedVar.begin(), 
									parsedVar.end(), 
									(I+i)->first.getAsString() ) == parsedVar.end()) {
						
						if( Visitor.isHeaderFunc((I+i)->first.getAsString()) )
							continue;
							
						SS 	<< "if (" << (I+i)->first.getAsString() << ".type==-1) {"
							<< "printf(\"ERROR: VARIABLE " << (I+i)->first.getAsString() << " IS UNDEFINED AT LOC " 
							<< (I+i)->second.getRawEncoding() <<"\"); exit(1);}\n";
						
						if( !Visitor.isStructAccess((I+i)->second.getRawEncoding() + (I+i)->first.getAsString().length()) )
							parsedVar.push_back( (I+i)->first.getAsString() );
							
					}
				}
				
				
				// check if there are any VisitDeclRef variables in this line
				int back = errorLine.length()-forwardLocToEnd;
				int startloc = I->second.getRawEncoding() - back;
				int iterloc = startloc;
				int tokenlen;
				Token Result;
				
				while( iterloc <= (startloc + errorLine.length()) ) {
					if( errorLine[iterloc - startloc] == ' ' || errorLine[iterloc - startloc] == '\t') {
						iterloc++;
						continue;
					}
					Lexer::getRawToken(SourceLocation::getFromRawEncoding(iterloc),
										Result,	Visitor.getRewriter().getSourceMgr(),
										Visitor.getRewriter().getLangOpts(), true);
					tokenlen = Result.getLength();
					std::string token = errorLine.substr( iterloc - startloc, tokenlen);
					dbg("Token: " << token << " iterloc: " << iterloc);
					if( !Visitor.isNewVarName(token) 
						&& (std::find(parsedVar.begin(), parsedVar.end(), token) == parsedVar.end())
						&& !Visitor.isStructAccess(iterloc+tokenlen) ) {
							parsedVar.push_back( Visitor.allVarNames[Visitor.getIndexOfVarName( token )].vName );
							dbg("\n ADDED: "<< Visitor.allVarNames[Visitor.getIndexOfVarName( token )].vName);
					}
					
					dbg(" tokenlen: " << tokenlen << "\n");
					
					if( Visitor.isStructAccess(iterloc+tokenlen) ) {
							
						dbg("struct access " << token << " " << iterloc << "\n");
						for(int i=0;i<Visitor.allVarNames[Visitor.getIndexOfVarName(token)].num_types;i++) {
							if (Visitor.allVarNames[Visitor.getIndexOfVarName(token)].types[i].find("struct") != std::string::npos || 
								Visitor.typedefDecl[Visitor.allVarNames[Visitor.getIndexOfVarName(token)].types[i]].find("struct") != std::string::npos) {
									structtypeindex = i;
								continue;
							}
							SS 	<< "if(" << token << ".type==" << i << "){"
								<< "printf(\"ERROR: EXPECTING STRUCT TYPE\"); exit(1);}\n\t";
						}
						replacedString = token + ".du." + 
									Visitor.allVarNames[Visitor.getIndexOfVarName(token)].types[structtypeindex] + "val->";
						Visitor.replaceAll(errorLineStruct, token+".", replacedString);
	
					}
					iterloc = iterloc + tokenlen;
				}
				
				errorLine.assign(errorLineStruct);
				
				int combinations = 1;
				std::string basis = "";
				std::string components;
				std::string errorLineOrg;
				for( int i = 0;i< parsedVar.size();i++) {
					combinations *= Visitor.allVarNames[Visitor.getIndexOfVarName(parsedVar[i])].num_types;
					basis += std::to_string(Visitor.allVarNames[Visitor.getIndexOfVarName(parsedVar[i])].num_types);
				}
				
				dbg("COMBINATIONS: " << combinations << "\n");
				
				dbg("PARSEDVAR SIZE and LINE: " << parsedVar.size() << " " << errorLine.length() << "\n");
				
				
				for ( int i = 0;i < combinations;i++) {
					components = Visitor.getComponents(i,basis);
					dbg( basis << " : " << components << " : " << parsedVar.size() << "\n");
					if(parsedVar.size() == 0) {
						SS << errorLine << ";\n";
						continue;
					}
					errorLineOrg.assign(errorLine);
					SS << "if (" ;
					for ( int j = 0;j<parsedVar.size();j++) {
						replacedString =parsedVar[j] + ".du." +
										Visitor.allVarNames[Visitor.getIndexOfVarName(parsedVar[j])].types
											[components[j]-'0'] + "val";
						
						
						Visitor.replaceAll(errorLine, parsedVar[j], replacedString);
						
						SS 	<< parsedVar[j] << ".type==" << components[j]; 
						if((j+1) < parsedVar.size())
							SS << " && ";
						
					}
					SS << ")" << errorLine << ";\n";
					errorLine.assign(errorLineOrg);
				}
				
				
				//Replace text from the program
				unsigned int beginLoc = I->second.getRawEncoding();
				beginLoc = beginLoc - (errorLineLength - forwardLocToEnd);
				Visitor.TheRewriter.ReplaceText(SourceLocation::getFromRawEncoding(beginLoc), errorLineLength+1, SS.str());
				
				// Skip VarCtr iterations ahead
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
	
	globalSema = &mySema;

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
