/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       Parser2                                                        */
/*                                                                          */
/*       Group Members:          ID numbers                                 */
/*                                                                          */
/*           Niall Meade         22339752                                   */
/*           Nancy Haren         22352511                                   */
/*           Kennedy Ogbedeagu   22341439                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/*                                                                          */
/*       Parses the language defined by this grammar:                       */
/*                                                                          */
/*       <Program>         :==  "PROGRAM" <Identifier> ";"                         */
/*                              [ <Declarations> ] { <ProcDeclaration> }           */
/*                              <Block> "."                                        */
/*       <Declarations>    :==  "VAR" <Variable> { "," <Variable> } ";"            */
/*       <ProcDeclaration> :==  "PROCEDURE" <Identifier> [ <ParameterList> ] ";"   */
/*                              [ <Declarations> ] { <ProcDeclaration> }           */
/*                              <Block> ";"                                        */
/*       <ParameterList>   :==  "(" <FormalParameter> { "," <FormalParameter> } ")"*/
/*       <FormalParameter> :==  [ "REF" ] <Variable>                               */
/*       <Block>           :==  "BEGIN" { <Statement> ";" } "END"                  */
/*       <Statement>       :==  <SimpleStatement>                                  */
/*                          |   <WhileStatement>                                   */
/*                          |   <IfStatement>                                      */
/*                          |   <ReadStatement>                                    */
/*                          |   <WriteStatement>                                   */
/*       <SimpleStatement> :==  <VarOrProcName> <RestOfStatement>                  */
/*       <RestOfStatement> :==  <ProcCallList>                                     */
/*                          |   <Assignment>                                       */
/*                          |   ε                                                  */
/*       <ProcCallList>    :==  "(" <ActualParameter> { "," <ActualParameter> } ")"*/
/*       <Assignment>      :==  ":=" <Expression>                                  */
/*       <ActualParameter> :==  <Variable> | <Expression>                          */
/*       <WhileStatement>  :==  "WHILE" <BooleanExpression> "DO" <Block>           */
/*       <IfStatement>     :==  "IF" <BooleanExpression> "THEN" <Block>            */
/*                          [ "ELSE" <Block> ]                                     */
/*       <ReadStatement>   :==  "READ" "(" <Variable> { "," <Variable> } ")"       */
/*       <WriteStatement>  :==  "WRITE" "(" <Expression> { "," <Expression> } ")"  */
/*       <Expression>      :==  <CompoundTerm> { ("+" | "-") <CompoundTerm> }      */
/*       <CompoundTerm>    :==  <Term> { ("*" | "/") <Term> }                      */
/*       <Term>            :==  [ "-" ] <SubTerm>                                  */
/*       <SubTerm>         :==  <Variable> | <IntConst> | "(" <Expression> ")"     */
/*       <BooleanExpression> :==  <Expression> <RelOp> <Expression>                */
/*       <RelOp>           :==  "=" | "<=" | ">=" | "<" | ">"                      */
/*       <Variable>        :==  <Identifier>                                       */
/*       <VarOrProcName>   :==  <Identifier>                                       */
/*       <Identifier>      :==  <Alpha> { <AlphaNum> }                             */
/*       <IntConst>        :==  <Digit> { <Digit> }                                */
/*       <AlphaNum>        :==  <Alpha> | <Digit>                                  */
/*       <Alpha>           :==  "A".."Z" | "a".."z"                                */
/*       <Digit>           :==  "0".."9"                                           */
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


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Global variables used by this Parser.                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE FILE *InputFile;           /*  CPL source comes from here.          */
PRIVATE FILE *ListFile;            /*  For nicely-formatted syntax errors.  */

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
PRIVATE void ParseRestOfStatement( void );
PRIVATE void ParseProcCallList( void );
PRIVATE void ParseAssignment( void );
PRIVATE void ParseActualParameter( void );
PRIVATE int  OpenFiles( int argc, char *argv[] );
PRIVATE void Accept( int code );
PRIVATE void ReadToEndOfFile( void );
PRIVATE void ParseTerm(void);
PRIVATE void ParseReadStatement( void );
PRIVATE void ParseCompoundTerm( void );
PRIVATE void ParseRelOp( void );
PRIVATE void ParseMultOp( void );
PRIVATE void ParseAddOp( void );
PRIVATE void ParseWriteStatement( void );
PRIVATE void ParseSubTerm( void );
PRIVATE void ParseBooleanExpression( void );
PRIVATE void ParseIfStatement( void );
PRIVATE void Synchronise( SET *F, SET *FB );
PRIVATE void SetupSets( void );

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Main: Parser2 entry point.  Sets up Parser globals (opens input and     */
/*        output files, initialises current lookahead), then calls          */
/*        "ParseProgram" to start the Parse.                                */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PUBLIC int main ( int argc, char *argv[] )
{
    if ( OpenFiles( argc, argv ) )  {
        InitCharProcessor( InputFile, ListFile );
        CurrentToken = GetToken();
        SetupSets();
        ParseProgram();
        fclose( InputFile );
        fclose( ListFile );
        printf("valid\n");
        return  EXIT_SUCCESS;
    }
    else 
        return EXIT_FAILURE;
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Parser routines: Recursive-descent implementaion of the grammar's       */
/*                   productions.                                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  	ParseProgram implements:                          		            */
/*                                                                          */
/*         	〈Program〉:== “PROGRAM〈Identifier〉“;”                         */
/*           [〈Declarations〉]{ 〈ProcDeclaration〉 } 〈Block〉“.”          */
/*                                                                          */
/*       								                                    */
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
PRIVATE void ParseProgram( void )
{
    
    Accept( PROGRAM );  
    Accept( IDENTIFIER );
    Accept( SEMICOLON );
    Synchronise( &ProgramFS_aug, &ProgramFBS );
    if ( CurrentToken.code == VAR ) { 
        ParseDeclarations();
    }
        Synchronise( &ProgramSS_aug, &ProgramFBS);
    while ( CurrentToken.code == PROCEDURE ) {
        ParseProcDeclarations();
        Synchronise( &ProgramSS_aug, &ProgramFBS );
    }

    ParseBlock();
    Accept( ENDOFPROGRAM );     /* Token "." has name ENDOFPROGRAM          */
    Accept( ENDOFINPUT );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Synchronise Implements:                                                 */
/*   Prints error if current token isnt in first set. Advances lookahead    */
/*   token until token in First or Follow beacon set is found in order to   */
/*   resynchronise parser                                                   */
/*                                                                          */
/*    Inputs:       SET *F: Set of first tokensfor expected grammar token   */
/*                  SET *FB: Set of follow and beacon symbols               */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      None                                                    */
/*                                                                          */
/*    Side Effects: Advances LookAhead token. Prints detected syntax error  */
/*                                                                          */
/*--------------------------------------------------------------------------*/
PRIVATE void Synchronise( SET *F, SET *FB )
{
    SET S;
    S = Union( 2, F, FB );
    if (!InSet(F, CurrentToken.code)) {
        SyntaxError2( *F, CurrentToken );
        while ( !InSet( &S, CurrentToken.code ) )
            CurrentToken = GetToken();
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  	SetupSets:                          		                        */
/*      Setup the follower and beacon sets for use in synchronise later.    */
/*                                                                          */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      Configures global sets                                  */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Configures global sets                                  */
/*                                                                          */
/*--------------------------------------------------------------------------*/
PRIVATE void SetupSets( void )
{
    InitSet( &ProgramFS_aug, 3, VAR, PROCEDURE, BEGIN );
    InitSet( &ProgramSS_aug, 2, PROCEDURE, BEGIN );
    InitSet( &ProgramFBS, 3, ENDOFINPUT, END, ENDOFPROGRAM );

    InitSet( &ProcedureFS_aug, 3, VAR, PROCEDURE, BEGIN );
    InitSet( &ProcedureSS_aug, 2, PROCEDURE, BEGIN );
    InitSet( &ProcedureFBS, 3, ENDOFINPUT, END, ENDOFPROGRAM );

    InitSet( &BlockFS_aug, 6, IDENTIFIER, WHILE, IF, READ, WRITE, END );
    InitSet( &BlockFBS, 4, SEMICOLON, ELSE, ENDOFINPUT, ENDOFPROGRAM );

    InitSet( &RestOfStatementFS_aug, 3, LEFTPARENTHESIS, ASSIGNMENT, SEMICOLON );
    InitSet( &RestOfStatementFBS, 3, SEMICOLON, ENDOFINPUT, ENDOFPROGRAM );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*    ParseExpression implements:                          		            */
/*                                                                          */
/*         	〈Expression〉:==〈CompoundTerm〉{ 〈AddOp〉〈CompoundTerm〉 }   */
/*                                                                          */
/*       								                                    */
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
PRIVATE void ParseExpression( void )
{
	ParseCompoundTerm();
  	while ( CurrentToken.code == ADD || CurrentToken.code == SUBTRACT ) { /*TODO Find better way to do this*/
        ParseAddOp();
        ParseCompoundTerm();
  	}
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseWhileStatement implements:                          		        */
/*                                                                          */
/* 〈WhileStatement〉:==   “WHILE”〈BooleanExpression〉“DO”〈Block〉 		 */
/*                                                                          */
/*       								                                    */
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

PRIVATE void ParseWhileStatement( void )
{
	Accept( WHILE );
	ParseBooleanExpression();
	Accept( DO );
	ParseBlock();
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseProcDeclarations implements:                          		        */
/*                                                                          */
/*  <ProcDeclaration> :==  "PROCEDURE" <Identifier> [ <ParameterList> ] ";" */
/*                         [ <Declarations> ] { <ProcDeclaration> }         */
/*                         <Block> ";"                                      */
/*       								                                    */
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
PRIVATE void ParseProcDeclarations( void )
{
    Accept( PROCEDURE );
    Accept( IDENTIFIER );
    
    if ( CurrentToken.code == LEFTPARENTHESIS ) ParseParamList();

    Accept( SEMICOLON );

    Synchronise( &ProcedureFS_aug, &ProcedureFBS );
    if ( CurrentToken.code == VAR ) ParseDeclarations();

    Synchronise( &ProcedureSS_aug, &ProcedureFBS );
    while ( CurrentToken.code == PROCEDURE ) {
        ParseProcDeclarations();
        Synchronise( &ProcedureSS_aug, &ProcedureFBS );
    }

    ParseBlock();

    Accept( SEMICOLON );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseBlock implements:                          		                */
/*                                                                          */
/* 〈Block〉:==   "BEGIN" { <Statement> ";" } "END"                  		 */
/*                                                                          */
/*       								                                    */
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
PRIVATE void ParseBlock( void )
{
    Accept( BEGIN );

    /* EBNF repetition operator {...} implemented as a while-loop.          */
    /* Repetition triggered by a <Statement> in the input stream.           */
    /* A <Statement> starts with a <Variable>, which is an IDENTIFIER.      */
    Synchronise( &BlockFS_aug, &BlockFBS );
    while ( (CurrentToken.code == IDENTIFIER) || ((CurrentToken.code == WHILE)) || (CurrentToken.code == IF) || (CurrentToken.code == READ) || (CurrentToken.code == WRITE) ) {
        ParseStatement();
        Accept( SEMICOLON );
        Synchronise( &BlockFS_aug, &BlockFBS );
    }

    Accept( END );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseParamList implements:                          		            */
/*                                                                          */
/* 〈ParamList〉:==   "(" <FormalParameter> { "," <FormalParameter> } ")"   */
/*                                                                          */
/*       								                                    */
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
PRIVATE void ParseParamList( void )
{
    Accept( LEFTPARENTHESIS );
    ParseFormalParam();
    while ( CurrentToken.code == COMMA ) {
        Accept( COMMA );
        ParseFormalParam();
    }
    Accept( RIGHTPARENTHESIS );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseDeclarations implements:                          		            */
/*                                                                          */
/* 〈Declarations〉:==   “VAR”〈Variable〉 { "," <Variable> } ";"   		 */
/*                                                                          */
/*       								                                    */
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
PRIVATE void ParseDeclarations( void )
{
    Accept( VAR );
    Accept( IDENTIFIER );
    while( CurrentToken.code == COMMA ){
        Accept( COMMA );
        Accept( IDENTIFIER );
    }
    Accept( SEMICOLON );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseActualParameter implements:                          		        */
/*                                                                          */
/* 〈ActualParameter〉:==   <Variable> | <Expression>                		*/
/*                                                                          */
/*       								                                    */
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
PRIVATE void ParseActualParameter( void )
{
    if ( CurrentToken.code == IDENTIFIER ) {
        Accept( IDENTIFIER );
    } else {
        ParseExpression();
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseFormalParam implements:                          		            */
/*                                                                          */
/* 〈FormalParam〉:==   ["REF"] <Variable>              		            */
/*                                                                          */
/*       								                                    */
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
PRIVATE void ParseFormalParam( void )
{
    if ( CurrentToken.code == REF ) Accept( REF );
    Accept( IDENTIFIER );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseIfStatement implements:                          		            */
/*                                                                          */
/* 〈IfStatement〉:==   “IF”〈BooleanExpression〉“THEN”〈Block〉             */
/*                     [ “ELSE”〈Block〉]                                   */
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
PRIVATE void ParseIfStatement( void )
{
	Accept( IF );
	ParseBooleanExpression();
	Accept( THEN );
	ParseBlock();
	if ( CurrentToken.code == ELSE ) {
		Accept( ELSE );
		ParseBlock();
  	}
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseReadStatement implements:                          		        */
/*                                                                          */
/* 〈ReadStatement〉:==   “READ” “(”〈Variable〉 {“,”〈Variable〉 }“)”   	 */
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
PRIVATE void ParseReadStatement( void )
{
	Accept( READ );
	Accept( LEFTPARENTHESIS );
	Accept( IDENTIFIER );
	while ( CurrentToken.code == COMMA ) {
		Accept( COMMA );
		Accept( IDENTIFIER );
  	}
  	Accept( RIGHTPARENTHESIS );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseStatement implements:                                  		    */
/*                                                                          */
/* 〈Statement〉:==   <SimpleStatement>	| <WhileStatement> |                */
/*               <IfStatement> | <ReadStatement> | <WriteStatement>         */
/*       			                                					    */
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
PRIVATE void ParseStatement( void )
{
    switch ( CurrentToken.code ) {
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

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseSimpleStatement implements:                                 		*/
/*                                                                          */
/* 〈SimpleStatement〉:==   <VarOrProcName> <RestOfStatement>               */
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
PRIVATE void ParseSimpleStatement( void )
{
    Accept( IDENTIFIER );
    ParseRestOfStatement();
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseRestOfStatement implements:                                 		*/
/*                                                                          */
/* 〈RestOfStatement〉:==   <ProcCallList> | <Assignment> | ε               */
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
PRIVATE void ParseRestOfStatement( void )
{
    Synchronise( &RestOfStatementFS_aug, &RestOfStatementFBS );
    if ( CurrentToken.code == SEMICOLON ) {
        return;
    } else if ( CurrentToken.code == LEFTPARENTHESIS ) {
        ParseProcCallList();
    } else {
        ParseAssignment();
    }
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseProcCallList implements:                                 		    */
/*                                                                          */
/* 〈ProcCallList〉:==   "(" <ActualParameter> { "," <ActualParameter> } ")"*/
/*                                                                          */
/*       			                                					    */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/
PRIVATE void ParseProcCallList( void )
{
    Accept( LEFTPARENTHESIS );
    ParseActualParameter();
    while ( CurrentToken.code == COMMA ) {
        Accept( COMMA );
        ParseActualParameter();
    }
    Accept( RIGHTPARENTHESIS );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseAssignment implements:                                 		    */
/*                                                                          */
/* 〈Assignment〉:==   ":=" <Expression>                                    */
/*                                                                          */
/*       			                                					    */
/*    Inputs:       None                                                    */
/*                                                                          */
/*    Outputs:      None                                                    */
/*                                                                          */
/*    Returns:      Nothing                                                 */
/*                                                                          */
/*    Side Effects: Lookahead token advanced.                               */
/*                                                                          */
/*--------------------------------------------------------------------------*/
PRIVATE void ParseAssignment( void )
{
    Accept( ASSIGNMENT );
    ParseExpression();
}


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseWriteStatement implements:                          		        */
/*                                                                          */
/* 〈WriteStatement〉:==   “WRITE” “(”〈Expression〉 {“,”〈Expression〉 }“)” */
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
	Accept( WRITE );
	Accept( LEFTPARENTHESIS );
	ParseExpression();
	while ( CurrentToken.code == COMMA ){
		Accept( COMMA );
		ParseExpression();
  	}
  	Accept( RIGHTPARENTHESIS );
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseCompoundTerm implements:                          		            */
/*                                                                          */
/*         	〈CompoundTerm〉:==〈Term〉 { 〈MultOp〉〈Term〉  }              */
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
PRIVATE void ParseCompoundTerm( void )
{
	ParseTerm();
  	while ( (CurrentToken.code == MULTIPLY) || (CurrentToken.code == DIVIDE)) {
  	    ParseMultOp();
  	    ParseTerm();
  	}
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseTerm implements:                          		                    */
/*                                                                          */
/*         	 〈Term〉 :== [“−”]〈SubTerm〉                                   */
/*                                                                          */
/*       								                                    */
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
PRIVATE void ParseTerm( void )
{
	if ( CurrentToken.code == SUBTRACT ){
		Accept( SUBTRACT );
  	}
  	ParseSubTerm();
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseSubTerm implements:                          		                */
/*                                                                          */
/*    〈SubTerm〉 :==〈Identifier〉 |<INTCONST> | "(" <EXPRESSION〉 ")"      */
/*                                                                          */
/*       								                                    */
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
PRIVATE void ParseSubTerm( void )
{
	if ( CurrentToken.code == IDENTIFIER ) {
	Accept( IDENTIFIER );
  	} else if ( CurrentToken.code == INTCONST ) {
        Accept( INTCONST );
    } else {
        Accept( LEFTPARENTHESIS );
        ParseExpression();
        Accept( RIGHTPARENTHESIS );
  	}
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseBooleanExpression implements:                                      */
/*                                                                          */
/*     〈BooleanExpression〉:==〈Expression〉 〈RelOp〉〈Expression〉        */
/*                                                                          */
/*       								                                    */
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
PRIVATE void ParseBooleanExpression( void )
{
	ParseExpression();
	ParseRelOp();
    ParseExpression();
  
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseAddOp implements:                                                  */
/*                                                                          */
/*       〈AddOp〉:==   “+”|“−”				                                */
/*                                                                          */
/*       								                                    */
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
    if ( CurrentToken.code == ADD ) {
    	Accept( ADD );
    } else if (CurrentToken.code == SUBTRACT ) {
    	Accept( SUBTRACT );
    }  
}



/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseMultOp implements:                                                 */
/*                                                                          */
/*       〈MultOp〉:==   “*”|“/”				                            */
/*                                                                          */
/*       								                                    */
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
    if ( CurrentToken.code == MULTIPLY ) {
    	Accept( MULTIPLY );
    } else if ( CurrentToken.code == DIVIDE ) {
    	Accept( DIVIDE );
    }  
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  ParseRelOp implements:                                                  */
/*                                                                          */
/*       〈RelOp〉 :==   “=”|“<=”|“>=”|“<”|“>”		                        */
/*                                                                          */
/*       								                                    */
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
PRIVATE void ParseRelOp( void )
{
    if ( CurrentToken.code == EQUALITY ) {
    	Accept( EQUALITY );
    } else if ( CurrentToken.code == LESSEQUAL ) {
    	Accept( LESSEQUAL );
    } else if ( CurrentToken.code == GREATEREQUAL ) {
    	Accept( GREATEREQUAL);
    } else if ( CurrentToken.code == GREATER ) {
 		Accept( GREATER );
    } else if ( CurrentToken.code == LESS ) {
    	Accept( LESS);
    }
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
/*           this routine reports a syntax error and sets the recovering    */
/*           flag.  Note the use of routine "SyntaxError" (from "scanner.h")*/
/*           which puts the error message on the standard output and on the */
/*           listing file.                                                  */
/*                                                                          */
/*           In recovery mode we check if the current token is equal to the */
/*           expected token or end of input. If its not equal to either we  */
/*           progress the lookahead and loop back to the check again.       */
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

    if ( argc != 3 )  {
        fprintf( stderr, "%s <inputfile> <listfile>\n", argv[0] );
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