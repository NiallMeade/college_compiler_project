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


/*--------------------------------------------------------------------------*/
/*                                                                          */
/*  Global variables used by this parser.                                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

PRIVATE FILE *InputFile;           /*  CPL source comes from here.          */
PRIVATE FILE *ListFile;            /*  For nicely-formatted syntax errors.  */

PRIVATE TOKEN  CurrentToken;       /*  parser lookahead token.  Updated by  */
                                   /*  routine Accept (below).  Must be     */
                                   /*  initialised before parser starts.    */


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
PRIVATE void parseRestOfStatement( void );
PRIVATE void parseProcCallList( void );
PRIVATE void parseAssignment( void );
PRIVATE void parseActualParameter( void );
PRIVATE int  OpenFiles( int argc, char *argv[] );
PRIVATE void Accept( int code );
PRIVATE void ReadToEndOfFile( void );
PRIVATE void parseTerm(void);
PRIVATE void parseReadStatement( void );
PRIVATE void parseCompoundTerm( void );
PRIVATE void parseRelOp( void );
PRIVATE void parseMultOp( void );
PRIVATE void parseAddOp( void );
PRIVATE void parseWriteStatement( void );
PRIVATE void parseSubTerm( void );
PRIVATE void parseBooleanExpression( void );
PRIVATE void parseIfStatement( void );


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
        CurrentToken = GetToken();
        parseProgram();
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
    Accept( IDENTIFIER );
    Accept(SEMICOLON);
    if ( CurrentToken.code == VAR ) { 
    parseDeclarations();
    }
    while (CurrentToken.code == PROCEDURE)
    {
        parseProcDeclarations();
    }
    
    
 

    /* EBNF "zero or one of" operation: [...] implemented as if-statement.  */
    /* Operation triggered by a <Declarations> block in the input stream.   */
    /* <Declarations>, if present, begins with a "VAR" token.               */

    

    

    parseBlock();
    Accept( ENDOFPROGRAM );     /* Token "." has name ENDOFPROGRAM          */
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
	parseCompoundTerm();
  	while(CurrentToken.code== ADD || CurrentToken.code== SUBTRACT){ /*TODO Find better way to do this*/
  	parseAddOp();
  	parseCompoundTerm();
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
	Accept(WHILE);
	parseBooleanExpression();
	Accept(DO);
	parseBlock();
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
    Accept( PROCEDURE );
    Accept( IDENTIFIER );
    
    if( CurrentToken.code == LEFTPARENTHESIS ) parseParamList();

    Accept( SEMICOLON );

    if ( CurrentToken.code == VAR )  parseDeclarations();

    while (CurrentToken.code == PROCEDURE)
    {
        parseProcDeclarations();
    }

    parseBlock();

    Accept( SEMICOLON );
}

PRIVATE void parseBlock( void )
{
    Accept( BEGIN );

    /* EBNF repetition operator {...} implemented as a while-loop.          */
    /* Repetition triggered by a <Statement> in the input stream.           */
    /* A <Statement> starts with a <Variable>, which is an IDENTIFIER.      */

    while ( (CurrentToken.code == IDENTIFIER) || ((CurrentToken.code == WHILE)) || (CurrentToken.code == IF) || (CurrentToken.code == READ) || (CurrentToken.code == WRITE))  {
        parseStatement();
        Accept( SEMICOLON );
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
    Accept( VAR );
    Accept( IDENTIFIER );
    while( CurrentToken.code == COMMA){
        Accept( COMMA );
        Accept( IDENTIFIER );
    }
    Accept( SEMICOLON );
}

PRIVATE void parseActualParameter( void )
{
    if( CurrentToken.code == IDENTIFIER){
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
	Accept(IF);
	parseBooleanExpression();
	Accept(THEN);
	parseBlock();
	if(CurrentToken.code== ELSE){
		Accept(ELSE);
		parseBlock();
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
	Accept(READ);
	Accept(LEFTPARENTHESIS);
	Accept(IDENTIFIER);
	while(CurrentToken.code== COMMA){
		Accept( COMMA );
		Accept(IDENTIFIER);
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
    Accept( IDENTIFIER );
    parseRestOfStatement();
}

PRIVATE void parseRestOfStatement( void )
{
    if(CurrentToken.code == LEFTPARENTHESIS){
        parseProcCallList();
    } else if(CurrentToken.code == ASSIGNMENT){
        parseAssignment();
    } else{
        /*todo*/
    }
}

PRIVATE void parseProcCallList( void )
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
	while(CurrentToken.code== COMMA){
		Accept( COMMA );
		parseExpression();
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
	parseTerm();
  	while(CurrentToken.code== MULTIPLY || CurrentToken.code== DIVIDE){ /*TODO Find better way to do this*/
  	parseMultOp();
  	parseTerm();
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
	if(CurrentToken.code== SUBTRACT){
		Accept( SUBTRACT );
  	}
  	parseSubTerm();
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
	if ( CurrentToken.code == IDENTIFIER )  {
	Accept( IDENTIFIER );
  	} else if(CurrentToken.code == INTCONST){
        Accept(INTCONST);
    	} else{
  	Accept(LEFTPARENTHESIS);
  	parseExpression();
  	Accept(RIGHTPARENTHESIS);
  	
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

PRIVATE void parseBooleanExpression( void )
{
	parseExpression();
	parseRelOp();
    	parseExpression();
  
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

PRIVATE void parseRelOp( void )
{
    if ( CurrentToken.code == EQUALITY){
    	Accept( EQUALITY );
    	}
    	else if(CurrentToken.code == LESSEQUAL )
    	{
    		Accept( LESSEQUAL );
    	} else if (CurrentToken.code ==GREATEREQUAL){
    	    	Accept( GREATEREQUAL);
    	} else if (CurrentToken.code ==GREATER){
    	    	Accept( GREATER );
    	} else if (CurrentToken.code ==LESS){
    	    	Accept( LESS);
    	}
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
    if ( CurrentToken.code != ExpectedToken )  {
        SyntaxError( ExpectedToken, CurrentToken );
        ReadToEndOfFile();
        fclose( InputFile );
        fclose( ListFile );
        exit( EXIT_FAILURE );
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


