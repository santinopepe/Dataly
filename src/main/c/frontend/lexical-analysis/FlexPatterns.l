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
extern CompilationStatus UnknownLexemeAction();

extern CompilationStatus KeywordLexemeAction(TokenLabel label);
extern CompilationStatus IdentifierLexemeAction();
extern CompilationStatus StringLiteralLexemeAction();
extern CompilationStatus OperatorLexemeAction(TokenLabel label);
extern CompilationStatus JoinTypeLexemeAction(enum JoinType type);
extern CompilationStatus TypeTokenLexemeAction(UdfParamType type);

%}

/**
 * Flex contexts (a.k.a. start conditions).
 *
 * @see https://westes.github.io/flex/manual/Start-Conditions.html
 */
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
"orderBy"			{ return KeywordLexemeAction(ORDER_BY); }
"limit"			{ return KeywordLexemeAction(LIMIT); }
"join"				{ return KeywordLexemeAction(JOIN); }
"on"				{ return KeywordLexemeAction(ON); }
"groupBy"			{ return KeywordLexemeAction(GROUP_BY); }
"window"			{ return KeywordLexemeAction(WINDOW); }
"partitionBy"		{ return KeywordLexemeAction(PARTITION_BY); }
"asc"				{ return KeywordLexemeAction(ASC); }
"desc"				{ return KeywordLexemeAction(DESC); }
"inner"			{ return JoinTypeLexemeAction(JOIN_INNER); }
"left"				{ return JoinTypeLexemeAction(JOIN_LEFT); }
"right"			{ return JoinTypeLexemeAction(JOIN_RIGHT); }
"full"				{ return JoinTypeLexemeAction(JOIN_FULL); }
"semi"				{ return JoinTypeLexemeAction(JOIN_SEMI); }
"anti"				{ return JoinTypeLexemeAction(JOIN_ANTI); }
"udf"				{ return KeywordLexemeAction(UDF); }
"returns"			{ return KeywordLexemeAction(RETURNS); }
"int"				{ return TypeTokenLexemeAction(UDF_TYPE_INT); }
"string"			{ return TypeTokenLexemeAction(UDF_TYPE_STRING); }
"bool"				{ return TypeTokenLexemeAction(UDF_TYPE_BOOL); }
"decimal"			{ return TypeTokenLexemeAction(UDF_TYPE_DECIMAL); }
"and"				{ return KeywordLexemeAction(AND); }
"or"				{ return KeywordLexemeAction(OR); }
"not"				{ return KeywordLexemeAction(NOT); }
"is"				{ return KeywordLexemeAction(IS); }
"null"				{ return KeywordLexemeAction(NULL_TOKEN); }
"true"				{ return KeywordLexemeAction(TRUE_TOKEN); }
"false"				{ return KeywordLexemeAction(FALSE_TOKEN); }

">="				{ return OperatorLexemeAction(GREATER_EQUAL); }
"<="				{ return OperatorLexemeAction(LESS_EQUAL); }
"!="				{ return OperatorLexemeAction(NOT_EQUALS); }
"="					{ return OperatorLexemeAction(ASSIGN); }
","					{ return OperatorLexemeAction(COMMA); }
">"					{ return OperatorLexemeAction(GREATER); }
"<"					{ return OperatorLexemeAction(LESS); }
"%"					{ return OperatorLexemeAction(MOD); }

{string_literal}	{ return StringLiteralLexemeAction(); }
{identifier}		{ return IdentifierLexemeAction(); }

"{"					{ return OperatorLexemeAction(OPEN_BRACE); }
"}"					{ return OperatorLexemeAction(CLOSE_BRACE); }

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
