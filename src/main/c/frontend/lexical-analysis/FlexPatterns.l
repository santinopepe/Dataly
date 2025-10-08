%top{

#include "../../support/type/CompilationStatus.h"
#include "../../support/type/FlexContext.h"
#include "../../support/type/TokenLabel.h"
#include "../syntactic-analysis/AbstractSyntaxTree.h"
#include "../syntactic-analysis/BisonParser.h"

}

%{

/**
 * @see https://westes.github.io/flex/manual/Definitions-Section.html#Definitions-Section
 */

extern CompilationStatus ArithmeticOperatorLexemeAction(TokenLabel label);
extern CompilationStatus EnterImportExpressionLexemeAction(FlexContext context);
extern CompilationStatus EnterMultilineCommentLexemeAction(FlexContext context);
extern CompilationStatus EOFLexemeAction();
extern CompilationStatus IgnoredLexemeAction();
extern CompilationStatus IntegerLexemeAction();
extern CompilationStatus LeaveImportExpressionLexemeAction();
extern CompilationStatus LeaveMultilineCommentLexemeAction();
extern CompilationStatus ParenthesisLexemeAction(TokenLabel label);
extern CompilationStatus SubexpressionLexemeAction();
extern CompilationStatus UnknownLexemeAction();

extern CompilationStatus KeywordLexemeAction(TokenLabel label);
extern CompilationStatus IdentifierLexemeAction();
extern CompilationStatus StringLiteralLexemeAction();
extern CompilationStatus OperatorLexemeAction(TokenLabel label);

%}

/**
 * Flex contexts (a.k.a. start conditions).
 *
 * @see https://westes.github.io/flex/manual/Start-Conditions.html
 */
%x IMPORT_EXPRESSION
%x MULTILINE_COMMENT

/**
 * Reusable patterns.
 *
 * @see https://westes.github.io/flex/manual/Matching.html
 * @see https://westes.github.io/flex/manual/Patterns.html
 */
whitespace			[[:space:]]+
identifier			[a-zA-Z_][a-zA-Z0-9_]*
string_literal		\"[^\"]*\"

%%

%{

/**
 * @see https://westes.github.io/flex/manual/Rules-Section.html#Rules-Section
 */

%}



"param"				{ return KeywordLexemeAction(PARAM); }
"source"			{ return KeywordLexemeAction(SOURCE); }
"dataset"			{ return KeywordLexemeAction(DATASET); }
"transform"			{ return KeywordLexemeAction(TRANSFORM); }
"sink"				{ return KeywordLexemeAction(SINK); }
"write"				{ return KeywordLexemeAction(WRITE); }
"from"				{ return KeywordLexemeAction(FROM); }
"use"				{ return KeywordLexemeAction(USE); }
"filter"			{ return KeywordLexemeAction(FILTER); }
"withColumn"		{ return KeywordLexemeAction(WITH_COLUMN); }
"select"			{ return KeywordLexemeAction(SELECT); }
"into"				{ return KeywordLexemeAction(INTO); }
"and"				{ return KeywordLexemeAction(AND); }
"or"				{ return KeywordLexemeAction(OR); }
"is"				{ return KeywordLexemeAction(IS); }
"null"				{ return KeywordLexemeAction(NULL_TOKEN); }
"true"				{ return KeywordLexemeAction(TRUE_TOKEN); }
"false"				{ return KeywordLexemeAction(FALSE_TOKEN); }

"="					{ return OperatorLexemeAction(ASSIGN); }
","					{ return OperatorLexemeAction(COMMA); }
">"					{ return OperatorLexemeAction(GREATER); }
"<"					{ return OperatorLexemeAction(LESS); }

{string_literal}	{ return StringLiteralLexemeAction(); }
{identifier}		{ return IdentifierLexemeAction(); }

"{"					{ return EnterImportExpressionLexemeAction(IMPORT_EXPRESSION); }
<IMPORT_EXPRESSION>[^{}]+			{ return SubexpressionLexemeAction(); }
<IMPORT_EXPRESSION>"}"				{ return LeaveImportExpressionLexemeAction(); }
<IMPORT_EXPRESSION>.				{ return UnknownLexemeAction(); }

"/*"								{ return EnterMultilineCommentLexemeAction(MULTILINE_COMMENT); }
<MULTILINE_COMMENT>"*/"				{ return LeaveMultilineCommentLexemeAction(); }
<MULTILINE_COMMENT>[^*]+			{ return IgnoredLexemeAction(); }
<MULTILINE_COMMENT>.				{ return IgnoredLexemeAction(); }

"-"									{ return ArithmeticOperatorLexemeAction(SUB); }
"*"									{ return ArithmeticOperatorLexemeAction(MUL); }
"/"									{ return ArithmeticOperatorLexemeAction(DIV); }
"+"									{ return ArithmeticOperatorLexemeAction(ADD); }

"("									{ return ParenthesisLexemeAction(OPEN_PARENTHESIS); }
")"									{ return ParenthesisLexemeAction(CLOSE_PARENTHESIS); }

[[:digit:]]+						{ return IntegerLexemeAction(); }

{whitespace}						{ return IgnoredLexemeAction(); }
.									{ return UnknownLexemeAction(); }

<<EOF>>								{ return EOFLexemeAction(); }

%%

/**
 * @see https://westes.github.io/flex/manual/User-Code-Section.html#User-Code-Section
 */

#include "FlexActions.h"
#include "FlexExport.h"
