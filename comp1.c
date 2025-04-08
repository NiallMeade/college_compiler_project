/*--------------------------------------------------------------------------*/
/*                                                                          */
/*        compiler 1                                                        */
/*                                                                          */
/*       Group Members:          ID numbers                                 */
/*                                                                          */
/*           Niall Meade         22339752                                   */
/*           Nancy Haren         22352511                                   */
/*           Kennedy Ogbedeagu   22341439                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       This is a compiler performing syntax and semantic error detection  */
/*	 and code generation for the CPL language, excluding procedure 	    */
/*	definitions.							    */
/*  	 This handles READ and WRITE statements fully. 		            */
/*                                                                          */
/*--------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include "global.h"
#include "scanner.h"
#include "line.h"
#include "symbol.h"
#include "code.h"

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Global variables used by this compiler.                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE FILE *InputFile;           /*  CPL source comes from here.          */
PRIVATE FILE *ListFile;            /*  For nicely-formatted syntax errors.  */
PRIVATE FILE *CodeFile;           /*  Output machine code.          */

PRIVATE TOKEN  CurrentToken;       /*  Parser lookahead token.  Updated by  */
                                   /*  routine Accept (below).  Must be     */
                                   /*  initialised before Parser starts.    */

PRIVATE SET ProgramFS_aug;
PRIVATE SET ProgramSS_aug;
PRIVATE SET ProgramFBS;

PRIVATE SET ProcedureFS_aug;
PRIVATE SET ProcedureSS_aug;
PRIVATE SET ProcedureFBS;

PRIVATE SET BlockFS_aug;
PRIVATE SET BlockFBS;

PRIVATE SET RestOfStatementFS_aug;
PRIVATE SET RestOfStatementFBS;

int scope=1;/* global variables have scope 1*/ 
int varaddress=0;

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Function prototypes                                                     */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int  OpenFiles( int argc, char *argv[] );
PRIVATE void ParseProgram( void );
PRIVATE void ParseDeclarations( void );
PRIVATE void ParseProcDeclarations( void );
PRIVATE void ParseBlock( void );
PRIVATE void ParseParamList( void );
PRIVATE void ParseFormalParam( void );
PRIVATE void ParseStatement( void );
PRIVATE void ParseExpression( void );
PRIVATE void ParseTerm( void );
PRIVATE void ParseSimpleStatement( void );
PRIVATE void ParseWhileStatement( void );
PRIVATE void ParseIfStatement( void );
PRIVATE void ParseReadStatement( void );
PRIVATE void ParseWriteStatement( void );
PRIVATE void ParseRestOfStatement( SYMBOL *target );
PRIVATE void ParseProcCallList();
PRIVATE void ParseAssignment( void );
PRIVATE void ParseActualParameter( void );
PRIVATE int  OpenFiles( int argc, char *argv[] );
PRIVATE void Accept( int code );
PRIVATE void ReadToEndOfFile( void );
PRIVATE void ParseTerm(void);
PRIVATE void ParseReadStatement( void );
PRIVATE void ParseCompoundTerm( void );
PRIVATE int ParseRelOp( void );
PRIVATE void ParseMultOp( void );
PRIVATE void ParseAddOp( void );
PRIVATE void ParseWriteStatement( void );
PRIVATE void ParseSubTerm( void );
PRIVATE int ParseBooleanExpression( void );
PRIVATE void ParseIfStatement( void );
PRIVATE void Synchronise( SET *F, SET *FB );
PRIVATE void SetupSets( void );
PRIVATE void makeSymbolTableEntry( int symType );
PRIVATE SYMBOL *LookupSymbol( void );

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Main:  comp1 entry point.  Sets up Parser globals (opens input and      */
/*        output files, initialises current lookahead), then calls          */
/*        "ParseProgram" to start the Parse.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PUBLIC int main ( int argc, char *argv[] )
{
    if ( OpenFiles( argc, argv ) )  {
        InitCharProcessor( InputFile, ListFile );
        InitCodeGenerator( CodeFile );
        CurrentToken = GetToken();
        SetupSets();
        ParseProgram();
        WriteCodeFile();
        fclose( InputFile );
        fclose( ListFile );
        return  EXIT_SUCCESS;
    }
    else 
        return EXIT_FAILURE;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*                                                                          */
/*  ParseProgram implements:                                                */
/*                                                                          */
/*  <Program> :== "PROGRAM" <Identifier> ";" [<Declarations>]               */
/*                {<ProcDeclaration>} <Block> "."                           */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      Emit code for Halt at end of program                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced. Adds program to symbol table  */
/*                  Synchronise if syntax error detected                    */
/*--------------------------------------------------------------------------*/

/* :==   “PROGRAM〈Identifier〉“;”[〈Declarations〉]{ 〈ProcDeclaration〉 } 〈Block〉“.”*/
PRIVATE void ParseProgram( void )
{
    
    Accept( PROGRAM );  
    makeSymbolTableEntry(STYPE_PROGRAM);
    Accept( IDENTIFIER );
    Accept(SEMICOLON);
    Synchronise( &ProgramFS_aug, &ProgramFBS);
    if ( CurrentToken.code == VAR ) { 
    ParseDeclarations();
    }
    Synchronise( &ProgramSS_aug, &ProgramFBS);
    while (CurrentToken.code == PROCEDURE)
    {
        ParseProcDeclarations();
        Synchronise( &ProgramSS_aug, &ProgramFBS);
    }
    
    /* EBNF "zero or one of" operation: [...] implemented as if-statement.  */
    /* Operation triggered by a <Declarations> block in the input stream.   */
    /* <Declarations>, if present, begins with a "PROCEDURE" token.               */

    ParseBlock();
    Accept( ENDOFPROGRAM );     /* Token "." has name ENDOFPROGRAM          */
    Accept( ENDOFINPUT );
    _Emit(I_HALT);
}


/*----------------------------------------------------------------------------*/
/*                                                                            */
/*  Synchronise Implements:                                                   */
/*   Prints error if current token isnt in first set. Advances lookahead      */
/*   token until token in First or Follow beacon set is found in order to     */
/*    resynchronise parser                                                    */
/*                                                                            */
/*    Inputs:       SET *F: Set of first tokensfor expected grammar token     */
/*                  SET *FB: Set of follow and beacon symbols                 */
/*                                                                            */
/*    Outputs:      None                                                      */
/*                                                                            */
/*    Returns:      None                                                      */
/*                                                                            */
/*    Side Effects: Advances LookAhead token. Prints detected syntax error    */
/*                                                                            */
/*----------------------------------------------------------------------------*/

PRIVATE void Synchronise( SET *F, SET *FB )
{
    SET S;
    S = Union(2, F, FB);
    if(!InSet(F, CurrentToken.code)){
        SyntaxError2(*F, CurrentToken);
        while(!InSet( &S, CurrentToken.code ))
            CurrentToken = GetToken();
    }
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*  SetupSets Implements:                                                     */
/*   Initializes the global FIRST, FOLLOW, and beacon sets for program,	      */
/*    procedure and restofstatement grammar rules.                            */
/*    Inputs:       None                                                      */
/*                                                                            */
/*    Outputs:      None                                                      */
/*                                                                            */
/*    Returns:      None                                                      */
/*                                                                            */
/*    Side Effects: Initialises sets for syncronisation                       */
/*                                                                            */
/*----------------------------------------------------------------------------*/

PRIVATE void SetupSets( void )
{
    InitSet( &ProgramFS_aug, 3, VAR, PROCEDURE, BEGIN);
    InitSet( &ProgramSS_aug, 2, PROCEDURE, BEGIN);
    InitSet( &ProgramFBS, 3, ENDOFINPUT, END, ENDOFPROGRAM);

    InitSet( &ProcedureFS_aug, 3, VAR, PROCEDURE, BEGIN);
    InitSet( &ProcedureSS_aug, 2, PROCEDURE, BEGIN);
    InitSet( &ProcedureFBS, 3, ENDOFINPUT, END, ENDOFPROGRAM);

    InitSet( &BlockFS_aug, 6, IDENTIFIER, WHILE, IF, READ, WRITE, END);
    InitSet( &BlockFBS, 4, SEMICOLON, ELSE, ENDOFINPUT, ENDOFPROGRAM);

    InitSet( &RestOfStatementFS_aug, 3, LEFTPARENTHESIS, ASSIGNMENT, SEMICOLON);
    InitSet( &RestOfStatementFBS, 3, SEMICOLON, ENDOFINPUT, ENDOFPROGRAM);
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  	Expression implements:                          		    */
/*                                                                          */
/*         	〈Expression〉:==〈CompoundTerm〉{ 〈AddOp〉〈CompoundTerm〉 }      */
/*                                                                          */
/*       								    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      Code for Add and subtract operator                      */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/
PRIVATE void ParseExpression( void )
{
	int op;
	ParseCompoundTerm();
  	while((op=CurrentToken.code)== ADD || op== SUBTRACT){ /*TODO Find better way to do this*/
        ParseAddOp();
        ParseCompoundTerm();
        op==ADD ? _Emit(I_ADD): _Emit(I_SUB);
  	}
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseWhileStatement implements:                          		    */
/*                                                                          */
/* 〈WhileStatement〉:==   “WHILE”〈BooleanExpression〉“DO”〈Block〉 		        */
/*                                                                          */
/*       								    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      Emits code for while loop and backpatches loop          */
/*                  start address                                           */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseWhileStatement( void )
{
    int Label1, L2BackPatchLoc;
	Accept(WHILE);
    Label1 = CurrentCodeAddress();
    L2BackPatchLoc = ParseBooleanExpression();
	Accept(DO);
	ParseBlock();
    Emit(I_BR, Label1);
    BackPatch(L2BackPatchLoc, CurrentCodeAddress());
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseWhileStatement implements:                          		    */
/*                                                                          */
/* 〈WhileStatement〉:==   “WHILE”〈BooleanExpression〉“DO”〈Block〉 		        */
/*                                                                          */
/*       								    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced. Scope incremented  	    */
/*                  and decremented                                         */
/*--------------------------------------------------------------------------*/
PRIVATE void ParseProcDeclarations( void )
{
    Accept( PROCEDURE );
    makeSymbolTableEntry(STYPE_PROCEDURE);
    Accept( IDENTIFIER );
    scope++;

    if( CurrentToken.code == LEFTPARENTHESIS ) ParseParamList();

    Accept( SEMICOLON );

    Synchronise(&ProcedureFS_aug, &ProcedureFBS);
    if ( CurrentToken.code == VAR )  ParseDeclarations();

    Synchronise(&ProcedureSS_aug, &ProcedureFBS);
    while (CurrentToken.code == PROCEDURE)
    {
        ParseProcDeclarations();
        Synchronise(&ProcedureSS_aug, &ProcedureFBS);
    }

    ParseBlock();

    Accept( SEMICOLON );

    RemoveSymbols( scope );
    scope--;
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*  ParseBlock: Implements :		                                      */
/*                                                                            */
/*    〈Block〉 ::= “BEGIN” { 〈Statement〉 “;” } “END”                           */
/*                                                                            */
/*    Inputs:       None                                                      */
/*    Outputs:      None                                                      */
/*    Returns:      None                                                      */
/*    Side Effects: Advances the lookahead token.			      */
/*                                                                            */
/*----------------------------------------------------------------------------*/
PRIVATE void ParseBlock( void )
{
    Accept( BEGIN );

    /* EBNF repetition operator {...} implemented as a while-loop.          */
    /* Repetition triggered by a <Statement> in the input stream.           */
    /* A <Statement> starts with a <Variable>, which is an IDENTIFIER.      */
    Synchronise(&BlockFS_aug, &BlockFBS);
    while ( (CurrentToken.code == IDENTIFIER) || ((CurrentToken.code == WHILE)) || (CurrentToken.code == IF) || (CurrentToken.code == READ) || (CurrentToken.code == WRITE))  {
        ParseStatement();
        Accept( SEMICOLON );
        Synchronise(&BlockFS_aug, &BlockFBS);
    }

    Accept( END );
}
/*----------------------------------------------------------------------------*/
/*                                                                            */
/*  ParseParamList: Parses a parameter list in the following format:          */
/*                                                                            */
/*    〈ParameterList〉 ::= “(” 〈FormalParameter〉 { “,” 〈FormalParameter〉 } “)”  */
/*                                                                            */
/*    Inputs:       None                                                      */
/*    Outputs:      None                                                      */
/*    Returns:      None                                                      */
/*    Side Effects: Advances the lookahead toke.    			      */
/*                                                                            */
/*----------------------------------------------------------------------------*/
PRIVATE void ParseParamList( void )
{
    Accept( LEFTPARENTHESIS );
    ParseFormalParam();
    while ( CurrentToken.code == COMMA )  {
        Accept( COMMA );
        ParseFormalParam();
    }
    Accept( RIGHTPARENTHESIS );
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*  ParseDeclarations: implements					      */
/*                                                                            */
/*    〈Declarations〉 ::= “VAR” 〈Variable〉 { “,” 〈Variable〉 } “;”              */
/*                                                                            */
/*    Inputs:       None                                                      */
/*    Outputs:      emits code to allocate memory for variables               */
/*    Returns:      None                                                      */
/*    Side Effects: Adds variables to the symbol table, 		      */
/*                  and advances the lookahead token                          */
/*----------------------------------------------------------------------------*/
PRIVATE void ParseDeclarations( void )
{
    int num_new_vars = 0;
    Accept( VAR );
    makeSymbolTableEntry(STYPE_VARIABLE);
    Accept( IDENTIFIER );
    num_new_vars++;
    while( CurrentToken.code == COMMA){
        Accept( COMMA );
        makeSymbolTableEntry(STYPE_VARIABLE);
        Accept( IDENTIFIER );
        num_new_vars++;
    }
    Emit(I_INC, num_new_vars);
    Accept( SEMICOLON );
}
/*----------------------------------------------------------------------------*/
/*                                                                            */
/*  ParseActualParameter implements:					      */
/*                                                                            */
/*    〈ActualParameter〉 ::= 〈Variable〉 | 〈Expression〉                         */
/*                                                                            */
/*    Inputs:       None                                                      */
/*    Outputs:      None                                                      */
/*    Returns:      None                                                      */
/*    Side Effects: Advances the lookahead token			      */
/*                                                                            */
/*----------------------------------------------------------------------------*/

PRIVATE void ParseActualParameter( void )
{
    if( CurrentToken.code == IDENTIFIER){
        Accept( IDENTIFIER );
    } else{
        ParseExpression();
    }
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*  ParseFormalParam Implements:                                              */
/*                                                                            */
/*    〈FormalParam〉 ::= [ “REF” ] 〈Variable〉                                  */
/*                                                                            */
/*    Inputs:       None                                                      */
/*    Outputs:      None                                                      */
/*    Returns:      None                                                      */
/*    Side Effects: Advances the lookahead token			      */
/*                                                                            */
/*----------------------------------------------------------------------------*/

PRIVATE void ParseFormalParam( void )
{
    if(CurrentToken.code == REF) Accept( REF );
    Accept( IDENTIFIER );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseIfStatement implements:                          		    */
/*                                                                          */
/* 〈IfStatement〉:==   “IF”〈BooleanExpression〉“THEN”〈Block〉[ “ELSE”〈Block〉]  */
/*                                                                          */
/*       								    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      Emits code for conditionalbranching. Back patches       */
/*                  address for if and else loop jump around                */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced. 				    */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseIfStatement( void )
{
    int L2BackPatchLoc, L3BackPatchLoc;
	Accept(IF);
	L2BackPatchLoc = ParseBooleanExpression();
	Accept(THEN);
	ParseBlock();
    if(CurrentToken.code == ELSE){
        L3BackPatchLoc = CurrentCodeAddress();
        Emit(I_BR, 999);
		Accept(ELSE);
        BackPatch( L2BackPatchLoc, CurrentCodeAddress());
		ParseBlock();
        BackPatch( L3BackPatchLoc, CurrentCodeAddress());
  	} else{
        BackPatch( L2BackPatchLoc, CurrentCodeAddress());
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseReadStatement implements:                          		    */
/*                                                                          */
/* 〈ReadStatement〉:==   “READ” “(”〈Variable〉 {“,”〈Variable〉 }“)”   	    */
/*                                                                          */
/*       								    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      Emit code for read statement. Print error if            */
/*                  identifier is not defined                               */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced. kill code generation          */
/*       	    if error detected. emits code for read statement	    */
/*       								    */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseReadStatement( void )
{
    SYMBOL *var;
	Accept(READ);
	Accept(LEFTPARENTHESIS);
    var = LookupSymbol();
    if( var != NULL && (var->type == STYPE_VARIABLE || var->type == STYPE_LOCALVAR)){
	    Accept(IDENTIFIER);
        _Emit(I_READ);
        Emit(I_STOREA, var->address);
    } else{
        Error("Identifier is not defined or not of type variable", CurrentToken.pos);
        KillCodeGeneration();
    }
	while(CurrentToken.code== COMMA){
		Accept( COMMA );
        var = LookupSymbol();
        if( var != NULL && (var->type == STYPE_VARIABLE || var->type == STYPE_LOCALVAR)){
            Accept(IDENTIFIER);
            _Emit(I_READ);
            Emit(I_STOREA, var->address);
        } else{
            Error("Identifier is not defined or not of type variable", CurrentToken.pos);
            KillCodeGeneration();
        }  	
    }
  	Accept(RIGHTPARENTHESIS);
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*  ParseStatement Implements:                                                */
/*                                                                            */
/*    〈Statement〉 ::= 〈SimpleStatement〉                                       */
/*                    | 〈WhileStatement〉                                      */
/*                    | 〈IfStatement〉                                         */
/*                    | 〈ReadStatement〉                                       */
/*                    | 〈WriteStatement〉                                      */
/*                                                                            */
/*    Inputs:       None                                                      */
/*                                                                            */
/*    Outputs:      None                                                      */
/*                                                                            */
/*    Returns:      None                                                      */
/*                                                                            */
/*    Side Effects: Lookahead token advanced				      */
/*                  	                                                      */
/*                                                                            */
/*----------------------------------------------------------------------------*/
PRIVATE void ParseStatement( void )
{
    switch(CurrentToken.code) {
        case IDENTIFIER:
            ParseSimpleStatement();
            break;
        case WHILE:
            ParseWhileStatement();
            break;
        case IF:
            ParseIfStatement();
            break;
        case READ:
            ParseReadStatement();
            break;
        default:
            ParseWriteStatement();
    }
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*  ParseSimpleStatement Implements:                                          */
/*                                                                            */
/*    〈SimpleStatement〉 ::= 〈Variable〉 〈RestOfStatement〉                      */
/*                                                                            */
/*    Inputs:       None                                                      */
/*                                                                            */
/*    Outputs:      None                                                      */
/*                                                                            */
/*    Returns:      None                                                      */
/*                                                                            */
/*    Side Effects: Advances the lookahead token. Look up symbol for variable */
/*                                                                            */
/*----------------------------------------------------------------------------*/

PRIVATE void ParseSimpleStatement( void )
{
    SYMBOL *target = LookupSymbol();
    Accept( IDENTIFIER );
    ParseRestOfStatement( target );
}


/*----------------------------------------------------------------------------*/
/*                                                                            */
/*  ParseRestOfStatement Implements:                                          */
/*                                                                            */
/*    〈RestOfStatement〉 ::= 〈ProcCallList〉 | 〈Assignment〉 | ε                 */
/*                                                                            */
/*    Inputs:       SYMBOL *target: Pointer to the symbol table entry for     */
/*                  the identifier                                            */
/*                                                                            */
/*    Outputs:      None                                                      */
/*                                                                            */
/*    Returns:      None                                                      */
/*                                                                            */
/*    Side Effects: Performs synchronisation for error detection and recovery */
/*                  Emits code for procedure calls or assignment              */
/*                  Reports errors and halts code generation for invalid      */
/*                  identifiers                                               */
/*                                                                            */
/*----------------------------------------------------------------------------*/

PRIVATE void ParseRestOfStatement( SYMBOL *target )
{
    Synchronise(&RestOfStatementFS_aug, &RestOfStatementFBS);

    switch ( CurrentToken.code )
    {
    case LEFTPARENTHESIS:
        ParseProcCallList();
    case SEMICOLON:
        if ( target != NULL && target->type == STYPE_PROCEDURE){
            Emit( I_CALL, target->address);
        } else {
            Error("Identifier not a declared procedure.", CurrentToken.pos);
            KillCodeGeneration();
        }
        break;
    case ASSIGNMENT:
    default:
        ParseAssignment();
        if ( target != NULL && target->type == STYPE_VARIABLE ){
            Emit(I_STOREA, target->address);
        } else{
            Error("Identifier not a declared variable.", CurrentToken.pos);
            KillCodeGeneration();
        }
        break;
    }


}


/*----------------------------------------------------------------------------*/
/*                                                                            */
/*  ParseProcCallList Implements:                                             */
/*                                                                            */
/*    〈ProcCallList〉 ::= “(” 〈ActualParameter〉 { “,” 〈ActualParameter〉 } “)”  */
/*                                                                            */
/*    Inputs:       SYMBOL *target: Pointer to the symbol table entry for     */
/*                  procedure being called.                                   */
/*                                                                            */
/*    Outputs:      None                                                      */
/*                                                                            */
/*    Returns:      None                                                      */
/*                                                                            */
/*    Side Effects: Advance Lookahead symbol.  */
/*                                                                            */
/*----------------------------------------------------------------------------*/

PRIVATE void ParseProcCallList()
{
    Accept( LEFTPARENTHESIS );
    ParseActualParameter();
    while(CurrentToken.code == COMMA){
        Accept( COMMA );
        ParseActualParameter();
    }
    Accept(RIGHTPARENTHESIS);
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*  ParseAssignment Implements:                                               */
/*                                                                            */
/*    〈Assignment〉 ::= “:=” 〈Expression〉                                      */
/*                                                                            */
/*    Inputs:       None                                                      */
/*                                                                            */
/*    Outputs:      None                                                      */
/*                                                                            */
/*    Returns:      None                                                      */
/*                                                                            */
/*    Side Effects: Advances the lookahead token.			      */
/*                                                                            */
/*----------------------------------------------------------------------------*/

PRIVATE void ParseAssignment( void )
{
    Accept( ASSIGNMENT );
    ParseExpression();
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseWriteStatement implements:                          		    */
/*                                                                          */
/* 〈WriteStatement〉:==   “WRITE” “(”〈Expression〉 {“,”〈Expression〉 }“)”	    */
/*                                                                          */
/*       								    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseWriteStatement( void )
{
	Accept(WRITE);
	Accept(LEFTPARENTHESIS);
	ParseExpression();
        _Emit(I_WRITE);
	while(CurrentToken.code== COMMA){
		Accept( COMMA );
		ParseExpression();
        _Emit(I_WRITE);
  	}
  	Accept(RIGHTPARENTHESIS);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseCompoundTerm implements:                          		    */
/*                                                                          */
/*         	 〈CompoundTerm〉:==〈Term〉 { 〈MultOp〉〈Term〉  }                */
/*                                                                          */
/*       								    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced. Emits code for mult/div       */
/*                   operator                                               */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseCompoundTerm( void )
{
	int op;
	ParseTerm();
  	while((op=CurrentToken.code)== MULTIPLY || op== DIVIDE){ /*TODO Find better way to do this*/
  	ParseMultOp();
  	ParseTerm();
  	op==MULTIPLY ? _Emit(I_MULT): _Emit(I_DIV);
  	}
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseTerm implements:                          		            */
/*                                                                          */
/*         	 〈Term〉 :== [“−”]〈SubTerm〉		                                        */
/*                                                                          */
/*       								    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced. Emit code for subtract        */
/*                  and negate operator                                     */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseTerm( void )
{
	int negate=0;
	if(CurrentToken.code== SUBTRACT){
		negate=1;
		Accept( SUBTRACT );
  	}
  	ParseSubTerm();
  	
  	if(negate){ _Emit(I_NEG);}
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseSubTerm implements:                          		            */
/*                                                                          */
/*    〈SubTerm〉 :==〈Identifier〉 |<INTCONST> | "(" <EXPRESSION〉 ")"          */
/*                                                                          */
/*       								    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced. Lookup symbol for indentifier */
/*                  Emit code for loading intconst and indentifier value    */
/*       								    */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseSubTerm( void )
{
    SYMBOL *var;
	if ( CurrentToken.code == LEFTPARENTHESIS )  {
		Accept(LEFTPARENTHESIS);
		ParseExpression();
		Accept(RIGHTPARENTHESIS);
  	} else if(CurrentToken.code == INTCONST){
  		Emit(I_LOADI,CurrentToken.value);
        	Accept(INTCONST);
    	} else{
		var = LookupSymbol();
		if(var!=NULL && var->type== STYPE_VARIABLE){
			Emit(I_LOADA,var->address);
		}else{
			Error("variable undeclared", CurrentToken.pos);
		}
		
		Accept( IDENTIFIER );
  	}
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseBooleanExpression implements:                                      */
/*                                                                          */
/*     〈BooleanExpression〉:==〈Expression〉 〈RelOp〉〈Expression〉    		       */
/*                                                                          */
/*       								    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      address for back patching jump around                   */
/*                                                                          */
/*    Side Effects: Lookahead token advanced. Emit code for relationship    */
/*      operator. Emits code for jump around which requires backpatching    */
/*--------------------------------------------------------------------------*/

PRIVATE int ParseBooleanExpression( void )
{
    int BackPatchAddr, RelOpInstruction;
	ParseExpression();
	RelOpInstruction = ParseRelOp();
    ParseExpression();
    _Emit(I_SUB);
    BackPatchAddr = CurrentCodeAddress();
    Emit(RelOpInstruction, 9999);
    return BackPatchAddr;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseAddOp implements:                                             */
/*                                                                          */
/*       〈AddOp〉:==   “+”|“−”				                    */
/*                                                                          */
/*       								    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseAddOp( void )
{
    if ( CurrentToken.code == ADD ){
    	Accept( ADD );
    } else if(CurrentToken.code == SUBTRACT){
    	Accept( SUBTRACT );
    }  
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseMultOp implements:                                             */
/*                                                                          */
/*       〈MultOp〉:==   “*”|“/”				                    */
/*                                                                          */
/*       								    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ParseMultOp( void )
{
    if ( CurrentToken.code == MULTIPLY ){
    	Accept( MULTIPLY );
    	}
    	else if(CurrentToken.code == DIVIDE)
    	{
    		Accept( DIVIDE );
    	}  
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseRelOp implements:                                             */
/*                                                                          */
/*       〈RelOp〉 :==   “=”|“<=”|“>=”|“<”|“>”		                    */
/*                                                                          */
/*       								    */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Returns relationship operator code for emit instruction */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int ParseRelOp( void )
{
    int RelOpInstruction;
    if ( CurrentToken.code == EQUALITY){
        RelOpInstruction = I_BZ;
    	Accept( EQUALITY );
    }
    else if(CurrentToken.code == LESSEQUAL )
    {
        RelOpInstruction = I_BG;
        Accept( LESSEQUAL );
    } else if (CurrentToken.code ==GREATEREQUAL)
    {
        RelOpInstruction = I_BL;
        Accept( GREATEREQUAL);
    } else if (CurrentToken.code ==GREATER)
    {
        RelOpInstruction = I_BLZ;
        Accept( GREATER );
    } else if (CurrentToken.code ==LESS)
    {
        RelOpInstruction = I_BGZ;
        Accept( LESS);
    }
    return RelOpInstruction;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  End of Parser.  Support routines follow.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Accept:  Takes an expected token name as argument, and if the current   */
/*           lookahead matches this, advances the lookahead and returns.    */
/*                                                                          */
/*           If the expected token fails to match the current lookahead,    */
/*           this routine reports a syntax error and exits ("crash & burn"  */
/*           parsing).  Note the use of routine "SyntaxError"               */
/*           (from "scanner.h") which puts the error message on the         */
/*           standard output and on the listing file, and the helper        */
/*           "ReadToEndOfFile" which just ensures that the listing file is  */
/*           completely generated.                                          */
/*                                                                          */
/*                                                                          */
/*    Inputs:       Integer code of expected token                          */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: If successful, advances the current lookahead token     */
/*                  "CurrentToken".                                         */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void Accept( int ExpectedToken )
{
    static int recovering = 0;

    if ( recovering ) {
        while( (CurrentToken.code != ExpectedToken) && (CurrentToken.code != ENDOFINPUT))
            CurrentToken = GetToken();
        recovering = 0;
    }

    if ( CurrentToken.code != ExpectedToken )  {
        SyntaxError( ExpectedToken, CurrentToken );
        recovering = 1;
    }
    else  CurrentToken = GetToken();
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  OpenFiles:  Reads strings from the command-line and opens the           */
/*              associated input and listing files.                         */
/*                                                                          */
/*    Note that this routine modifies the globals "InputFile" and           */
/*    "ListingFile".  It returns 1 ("true" in C-speak) if the input and     */
/*    listing files are successfully opened, 0 if not, allowing the caller  */
/*    to make a graceful exit if the opening process failed.                */
/*                                                                          */
/*                                                                          */
/*    Inputs:       1) Integer argument count (standard C "argc").          */
/*                  2) Array of pointers to C-strings containing arguments  */
/*                  (standard C "argv").                                    */
/*                                                                          */
/*    Outputs:      No direct outputs, but note side effects.               */
/*                                                                          */
/*    Returns:      Boolean success flag (i.e., an "int":  1 or 0)          */
/*                                                                          */
/*    Side Effects: If successful, modifies globals "InputFile" and         */
/*                  "ListingFile".                                          */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int  OpenFiles( int argc, char *argv[] )
{

    if ( argc != 4 )  {
        fprintf( stderr, "%s <inputfile> <listfile> <codefile>\n", argv[0] );
        return 0;
    }

    if ( NULL == ( InputFile = fopen( argv[1], "r" ) ) )  {
        fprintf( stderr, "cannot open \"%s\" for input\n", argv[1] );
        return 0;
    }

    if ( NULL == ( ListFile = fopen( argv[2], "w" ) ) )  {
        fprintf( stderr, "cannot open \"%s\" for output\n", argv[2] );
        fclose( InputFile );
        return 0;
    }

    if ( NULL == ( CodeFile = fopen( argv[3], "w" ) ) )  {
        fprintf( stderr, "cannot open \"%s\" for output\n", argv[3] );
        fclose( CodeFile );
        return 0;
    }

    return 1;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ReadToEndOfFile:  Reads all remaining tokens from the input file.       */
/*              associated input and listing files.                         */
/*                                                                          */
/*    This is used to ensure that the listing file refects the entire       */
/*    input, even after a syntax error (because of crash & burn parsing,    */
/*    if a routine like this is not used, the listing file will not be      */
/*    complete.  Note that this routine also reports in the listing file    */
/*    exactly where the parsing stopped.  Note that this routine is         */
/*    superfluous in a Parser that performs error-recovery.                 */
/*                                                                          */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Reads all remaining tokens from the input.  There won't */
/*                  be any more available input after this routine returns. */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void ReadToEndOfFile( void )
{
    if ( CurrentToken.code != ENDOFINPUT )  {
        Error( "Parsing ends here in this program\n", CurrentToken.pos );
        while ( CurrentToken.code != ENDOFINPUT )  CurrentToken = GetToken();
    }
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*  makeSymbolTableEntry:                 	                              */
/*                                                                            */
/*    Creates a new entry in the symbol table for an identifier. Handles the  */
/*    insertion of new symbols and updates scope and symbol type and address  */
/*    Handles errors for redeclarations. 			              */
/*                                                                            */
/*    Inputs:       int symType: The type of the symbol being declared        */
/*                                                                            */
/*    Outputs:      None                                                      */
/*                                                                            */
/*    Returns:      None                                                      */
/*                                                                            */
/*    Side Effects: 				       			      */
/*		    Increments the variable address counter if the symbol is  */
/*                  a variable. Reports an error and kills code generation    */
/*                  if the identifier is already declared in the current      */
/*                  scope or if the entersymbol function fails                */
/*                                                                            */
/*----------------------------------------------------------------------------*/

PRIVATE void makeSymbolTableEntry(int symType) {
    SYMBOL *oldsptr=NULL;
    SYMBOL *newsptr=NULL;
    char *cptr;
    int hashindex;
    if (CurrentToken.code==IDENTIFIER) {
        if (NULL == (oldsptr = Probe(CurrentToken.s, &hashindex)) || oldsptr->scope < scope) {
            if (oldsptr==NULL) cptr = CurrentToken.s;
            else cptr= oldsptr->s;
            if ((newsptr = EnterSymbol(cptr, hashindex)) == NULL) {
                /*fatal error compiler must exit*/
                Error("failed to add symbol to table", CurrentToken.pos);
            	KillCodeGeneration();
            } else { 
                if (oldsptr==NULL) PreserveString();
                newsptr->scope = scope;
                newsptr->type = symType;
                if (symType == STYPE_VARIABLE) {
                    newsptr->address = varaddress;
                    varaddress++;
                } else {
                    newsptr->address = -1;
                }
            
            }
        } else {
            Error("Identifier previously declared", CurrentToken.pos);
            KillCodeGeneration();
        }
    }
}

/*----------------------------------------------------------------------------*/
/*                                                                            */
/*  LookupSymbol Implements:                                                  */
/*                                                                            */
/*    Searches for an identifier in the symbol table. Reports an error and    */
/*    halts code generation if symbol is not found in table                   */
/*                                                                            */
/*    Inputs:       None                                                      */
/*                                                                            */
/*    Outputs:      None                                                      */
/*                                                                            */
/*    Returns:      *SYMBOL : pointer to symbol if found in symbol table      */
/*		    or returns NULL if current token isnt an identifier	      */
/*                                                                            */
/*    Side Effects: prints error and kills code generation if symbol is not   */
/*		    found in symbol table. 	         	              */
/*                                                                            */
/*----------------------------------------------------------------------------*/

PRIVATE SYMBOL *LookupSymbol( void ) {
    SYMBOL *sptr;

    if (CurrentToken.code == IDENTIFIER) {
        sptr = Probe( CurrentToken.s, NULL);
        if (sptr == NULL) {
            Error("Identifier not declared", CurrentToken.pos);
            KillCodeGeneration();
        }
    }
    else sptr = NULL;
    return sptr;
}
