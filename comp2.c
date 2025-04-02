/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       smallparser                                                        */
/*                                                                          */
/*       Group Members:          ID numbers                                 */
/*                                                                          */
/*           Niall Meade         22339752                                   */
/*           Nancy Haren         22352511                                   */
/*           Kennedy Ogbedeagu   22341439                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       parses the language defined by this grammar:                       */
/*                                                                          */
/*       <Program>       :==  "PROGRAM" <Identifier> ";"                    */
/*                            [ <Declarations> ] <Block> "."           (1)  */
/*       <Declarations>  :==  "VAR" <Variable> { "," <Variable> } ";"  (2)  */
/*       <Block>         :==  "BEGIN" { <Statement> ";" } "END"        (3)  */
/*       <Statement>     :==  <Identifier> ":=" <Expression>           (4)  */
/*       <Expression>    :==  <Term> { ("+"|"-") <Term> }              (5)  */
/*       <Term>          :==  <Variable> | <IntConst>                  (6)  */
/*       <Variable>      :==  <Identifier>                             (7)  */
/*                                                                          */
/*                                                                          */
/*       Note - <Identifier> and <IntConst> are provided by the scanner     */
/*       as tokens IDENTIFIER and INTCONST respectively.                    */
/*                                                                          */
/*       Although the listing file generator has to be initialised in       */
/*       this program, full listing files cannot be generated in the        */
/*       presence of errors because of the "crash and burn" error-          */
/*       handling policy adopted. Only the first error is reported, the     */
/*       remainder of the input is simply copied to the output (using       */
/*       the routine "ReadToEndOfFile") without further comment.            */
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

PRIVATE TOKEN  CurrentToken;       /*  parser lookahead token.  Updated by  */
                                   /*  routine Accept (below).  Must be     */
                                   /*  initialised before parser starts.    */

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
PRIVATE void parseProgram( void );
PRIVATE void parseDeclarations( void );
PRIVATE void parseProcDeclarations( void );
PRIVATE void parseBlock( void );
PRIVATE void parseParamList( void );
PRIVATE void parseFormalParam( void );
PRIVATE void parseStatement( void );
PRIVATE void parseExpression( void );
PRIVATE void parseTerm( void );
PRIVATE void parseSimpleStatement( void );
PRIVATE void parseWhileStatement( void );
PRIVATE void parseIfStatement( void );
PRIVATE void parseReadStatement( void );
PRIVATE void parseWriteStatement( void );
PRIVATE void parseRestOfStatement( SYMBOL *target );
PRIVATE void parseProcCallList( SYMBOL *target );
PRIVATE void parseAssignment( void );
PRIVATE void parseActualParameter( void );
PRIVATE int  OpenFiles( int argc, char *argv[] );
PRIVATE void Accept( int code );
PRIVATE void ReadToEndOfFile( void );
PRIVATE void parseTerm(void);
PRIVATE void parseReadStatement( void );
PRIVATE void parseCompoundTerm( void );
PRIVATE int parseRelOp( void );
PRIVATE void parseMultOp( void );
PRIVATE void parseAddOp( void );
PRIVATE void parseWriteStatement( void );
PRIVATE void parseSubTerm( void );
PRIVATE int parseBooleanExpression( void );
PRIVATE void parseIfStatement( void );
PRIVATE void Synchronise( SET *F, SET *FB );
PRIVATE void SetupSets( void );
PRIVATE void makeSymbolTableEntry( int symType );
PRIVATE SYMBOL *makeSymbolTableEntry(int symType, int *varAddress );
PRIVATE SYMBOL *LookupSymbol( void );

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Main: Smallparser entry point.  Sets up parser globals (opens input and */
/*        output files, initialises current lookahead), then calls          */
/*        "parseProgram" to start the parse.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PUBLIC int main ( int argc, char *argv[] )
{
    if ( OpenFiles( argc, argv ) )  {
        InitCharProcessor( InputFile, ListFile );
        InitCodeGenerator( CodeFile );
        CurrentToken = GetToken();
        SetupSets();
        parseProgram();
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
/*  parser routines: Recursive-descent implementaion of the grammar's       */
/*                   productions.                                           */
/*                                                                          */
/*                                                                          */
/*  parseProgram implements:                                                */
/*                                                                          */
/*       <Program>     :== "BEGIN" { <Statement> ";" } "END" "."            */
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

/* :==   “PROGRAM〈Identifier〉“;”[〈Declarations〉]{ 〈ProcDeclaration〉 } 〈Block〉“.”*/
PRIVATE void parseProgram( void )
{
    
    Accept( PROGRAM );  
    makeSymbolTableEntry(STYPE_PROGRAM);
    Accept( IDENTIFIER );
    Accept(SEMICOLON);
    Synchronise( &ProgramFS_aug, &ProgramFBS);
    if ( CurrentToken.code == VAR ) { 
    parseDeclarations();
    }
    Synchronise( &ProgramSS_aug, &ProgramFBS);
    while (CurrentToken.code == PROCEDURE)
    {
        parseProcDeclarations();
        Synchronise( &ProgramSS_aug, &ProgramFBS);
    }
    
    /* EBNF "zero or one of" operation: [...] implemented as if-statement.  */
    /* Operation triggered by a <Declarations> block in the input stream.   */
    /* <Declarations>, if present, begins with a "PROCEDURE" token.               */

    parseBlock();
    Accept( ENDOFPROGRAM );     /* Token "." has name ENDOFPROGRAM          */
    Accept( ENDOFINPUT );
}

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
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/
PRIVATE void parseExpression( void )
{
	int op;
	parseCompoundTerm();
  	while((op=CurrentToken.code)== ADD || op== SUBTRACT){ /*TODO Find better way to do this*/
        parseAddOp();
        parseCompoundTerm();
        op==ADD ? _Emit(I_ADD): _Emit(I_SUB);
  	}
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  parseWhileStatement implements:                          		    */
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
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void parseWhileStatement( void )
{
    int Label1, L2BackPatchLoc;
	Accept(WHILE);
    Label1 = CurrentCodeAddress();
    L2BackPatchLoc = parseBooleanExpression();
	Accept(DO);
	parseBlock();
    Emit(I_BR, Label1);
    BackPatch(L2BackPatchLoc, CurrentCodeAddress());
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  parseWhileStatement implements:                          		    */
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
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/
PRIVATE void parseProcDeclarations( void )
{
    int BackPatchAddr;
    SYMBOL *procedure;

    Accept( PROCEDURE );
    procedure = makeSymbolTableEntry(STYPE_PROCEDURE, NULL);
    Accept( IDENTIFIER );
    BackPatchAddr = CurrentCodeAddress();
    Emit(I_BR. 0);
    
    if (procedure.address == NULL) {
        /* ERROR KILL CODDEGEN?*/
    } else {
        procedure.address = CurrentCodeAddress();

    }

    scope++;

    if( CurrentToken.code == LEFTPARENTHESIS ) parseParamList();

    Accept( SEMICOLON );

    Synchronise(&ProcedureFS_aug, &ProcedureFBS);
    if ( CurrentToken.code == VAR )  parseDeclarations();

    Synchronise(&ProcedureSS_aug, &ProcedureFBS);
    while (CurrentToken.code == PROCEDURE)
    {
        parseProcDeclarations();
        Synchronise(&ProcedureSS_aug, &ProcedureFBS);
    }

    parseBlock();

    Accept( SEMICOLON );
    
    _Emit(I_RET); /* RETURN*/
    BackPatch(BackPatchAddr, CurrentCodeAddress());
    RemoveSymbols( scope );
    scope--;
}

PRIVATE void parseBlock( void )
{
    Accept( BEGIN );

    /* EBNF repetition operator {...} implemented as a while-loop.          */
    /* Repetition triggered by a <Statement> in the input stream.           */
    /* A <Statement> starts with a <Variable>, which is an IDENTIFIER.      */
    Synchronise(&BlockFS_aug, &BlockFBS);
    while ( (CurrentToken.code == IDENTIFIER) || ((CurrentToken.code == WHILE)) || (CurrentToken.code == IF) || (CurrentToken.code == READ) || (CurrentToken.code == WRITE))  {
        parseStatement();
        Accept( SEMICOLON );
        Synchronise(&BlockFS_aug, &BlockFBS);
    }

    Accept( END );
}

PRIVATE void parseParamList( void )
{
    Accept( LEFTPARENTHESIS );
    parseFormalParam();
    while ( CurrentToken.code == COMMA )  {
        Accept( COMMA );
        parseFormalParam();
    }
    Accept( RIGHTPARENTHESIS );
}

PRIVATE void parseDeclarations( void )
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

PRIVATE void parseActualParameter( void )
{
    if( CurrentToken.code == IDENTIFIER){
        SYMBOL *var = LookupSymbol();
        Accept( IDENTIFIER );
    } else{
        parseExpression();
    }
}

PRIVATE void parseFormalParam( void )
{
    if(CurrentToken.code == REF) Accept( REF );
    Accept( IDENTIFIER );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  parseIfStatement implements:                          		    */
/*                                                                          */
/* 〈IfStatement〉:==   “IF”〈BooleanExpression〉“THEN”〈Block〉[ “ELSE”〈Block〉]  */
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

PRIVATE void parseIfStatement( void )
{
    int L2BackPatchLoc, L3BackPatchLoc;
	Accept(IF);
	L2BackPatchLoc = parseBooleanExpression();
	Accept(THEN);
	parseBlock();
    if(CurrentToken.code == ELSE){
        L3BackPatchLoc = CurrentCodeAddress();
        Emit(I_BR, 999);
		Accept(ELSE);
        BackPatch( L2BackPatchLoc, CurrentCodeAddress());
		parseBlock();
        BackPatch( L3BackPatchLoc, CurrentCodeAddress());
  	} else{
        BackPatch( L2BackPatchLoc, CurrentCodeAddress());
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  parseReadStatement implements:                          		    */
/*                                                                          */
/* 〈ReadStatement〉:==   “READ” “(”〈Variable〉 {“,”〈Variable〉 }“)”   	    */
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

PRIVATE void parseReadStatement( void )
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

PRIVATE void parseStatement( void )
{
    switch(CurrentToken.code) {
        case IDENTIFIER:
            parseSimpleStatement();
            break;
        case WHILE:
            parseWhileStatement();
            break;
        case IF:
            parseIfStatement();
            break;
        case READ:
            parseReadStatement();
            break;
        default:
            parseWriteStatement();
    }
}

PRIVATE void parseSimpleStatement( void )
{
    SYMBOL *target = LookupSymbol();
    Accept( IDENTIFIER );
    parseRestOfStatement( target );
}

PRIVATE void parseRestOfStatement( SYMBOL *target )
{
    Synchronise(&RestOfStatementFS_aug, &RestOfStatementFBS);
    int i, diffScope;
    switch ( CurrentToken.code )
    {
    case LEFTPARENTHESIS:
        parseProcCallList( target );
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
        parseAssignment();
        if ( target != NULL && target->type == STYPE_VARIABLE ){
            Emit(I_STOREA, target->address);
        } else if (target != NULL && target->type == STYPE_LOCALVAR){ /* TODO */
            diffScope = target->scope - 0; /* just get the scope and compare to baseline scope? */
            if (diffScope == 0){
                Emit(I_STOREFP, target->address);
            } else {
                _Emit(I_LOADFP);
                for (i = 0;  i < diffScope - 1; i++) {
                    _Emit(I_STORESP, target->address);
                }
            }

        } else{
            Error("Identifier not a declared variable.", CurrentToken.pos);
            KillCodeGeneration();
        }
        break;
    }


}

PRIVATE void parseProcCallList( SYMBOL *target )
{
    Accept( LEFTPARENTHESIS );
    parseActualParameter();
    while(CurrentToken.code == COMMA){
        Accept( COMMA );
        parseActualParameter();
    }
    Accept(RIGHTPARENTHESIS);
}

PRIVATE void parseAssignment( void )
{
    Accept( ASSIGNMENT );
    parseExpression();
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  parseWriteStatement implements:                          		    */
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

PRIVATE void parseWriteStatement( void )
{
	Accept(WRITE);
	Accept(LEFTPARENTHESIS);
	parseExpression();
    _Emit(I_WRITE);
	while(CurrentToken.code== COMMA){
		Accept( COMMA );
		parseExpression();
        _Emit(I_WRITE);
  	}
  	Accept(RIGHTPARENTHESIS);
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  parseCompoundTerm implements:                          		    */
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
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void parseCompoundTerm( void )
{
	int op;
	parseTerm();
  	while((op=CurrentToken.code)== MULTIPLY || op== DIVIDE){ /*TODO Find better way to do this*/
  	parseMultOp();
  	parseTerm();
  	op==MULTIPLY ? _Emit(I_MULT): _Emit(I_DIV);
  	}
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  parseTerm implements:                          		            */
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
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE void parseTerm( void )
{
	int negate=0;
	if(CurrentToken.code== SUBTRACT){
		negate=1;
		Accept( SUBTRACT );
  	}
  	parseSubTerm();
  	
  	if(negate){ _Emit(I_NEG);}
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  parseSubTerm implements:                          		            */
/*                                                                          */
/*    〈SubTerm〉 :==〈Identifier〉 |<INTCONST> | "(" <EXPRESSION〉 ")"       */
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

PRIVATE void parseSubTerm( void )
{
    int i, diffScope;
    SYMBOL *var;
	if ( CurrentToken.code == LEFTPARENTHESIS )  {
		Accept(LEFTPARENTHESIS);
		parseExpression();
		Accept(RIGHTPARENTHESIS);
  	} else if(CurrentToken.code == INTCONST){
  		Emit(I_LOADI,CurrentToken.value);
        	Accept(INTCONST);
    	} else{
		var = LookupSymbol();
		if (var!=NULL){

            if (var->type == STYPE_VARIABLE){
			Emit(I_LOADA,var->address);
            } else if (var->type == STYPE_LOCALVAR ) {
                diffScope = scope - var->scope;
                if (diffScope == 0){
                    Emit(I_LOADFP, var->address);
                } else{
                    _Emit(I_LOADFP);
                    for (i = 0; i < diffScope - 1; i++) {
                        _Emit(I_LOADSP, var->address);
                    }
                }
            }

		}else{
			Error("variable undeclared", CurrentToken.pos);
		}
		
		Accept( IDENTIFIER );
  	}
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  parseBooleanExpression implements:                                      */
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
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int parseBooleanExpression( void )
{
    int BackPatchAddr, RelOpInstruction;
	parseExpression();
	RelOpInstruction = parseRelOp();
    parseExpression();
    _Emit(I_SUB);
    BackPatchAddr = CurrentCodeAddress();
    Emit(RelOpInstruction, 9999);
    return BackPatchAddr;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  parseAddOp implements:                                             */
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

PRIVATE void parseAddOp( void )
{
    if ( CurrentToken.code == ADD ){
    	Accept( ADD );
    } else if(CurrentToken.code == SUBTRACT){
    	Accept( SUBTRACT );
    }  
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  parseMultOp implements:                                             */
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

PRIVATE void parseMultOp( void )
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
/*  parseRelOp implements:                                             */
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
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE int parseRelOp( void )
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
/*  End of parser.  Support routines follow.                                */
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
/*    superfluous in a parser that performs error-recovery.                 */
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

PRIVATE void makeSymbolTableEntry(int symType, int *varaddress) {
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
            } else { 
                if (oldsptr==NULL) PreserveString();
                newsptr->scope = scope;
                newsptr->type = symType;
                if (symType == STYPE_VARIABLE || symType == STYPE_LOCALVAR) {
                    newsptr->address = *varaddress;
                    (*varaddress)++;
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

PRIVATE SYMBOL *makeSymbolTableEntryRet(int symType, int *varaddress) {
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
            } else { 
                if (oldsptr==NULL) PreserveString();
                newsptr->scope = scope;
                newsptr->type = symType;
                if (symType == STYPE_VARIABLE || symType == STYPE_LOCALVAR) {
                    newsptr->address = *varaddress;
                    (*varaddress)++;
                } else {
                    newsptr->address = -1;
                }
            
            }
        } else {
            Error("Identifier previously declared", CurrentToken.pos);
            KillCodeGeneration();
        }
    }
    return newsptr;
}

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
