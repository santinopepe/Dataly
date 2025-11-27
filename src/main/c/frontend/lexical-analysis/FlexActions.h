#ifndef FLEX_ACTIONS_HEADER
#define FLEX_ACTIONS_HEADER

#include "../../support/configuration/Environment.h"
#include "../../support/language/String.h"
#include "../../support/logging/Logger.h"
#include "../../support/type/CompilationStatus.h"
#include "../../support/type/FlexContext.h"
#include "../../support/type/LexicalAnalyzer.h"
#include "../../support/type/ModuleDestructor.h"
#include "../../support/type/Token.h"
#include "../../support/type/TokenLabel.h"
#include "../Frontend.h"
#include "../syntactic-analysis/AbstractSyntaxTree.h"

/** Initialize module's internal state. */
ModuleDestructor initializeFlexActionsModule();

CompilationStatus ArithmeticOperatorLexemeAction(TokenLabel label);
CompilationStatus EnterImportExpressionLexemeAction(FlexContext context);
CompilationStatus EnterMultilineCommentLexemeAction(FlexContext context);
CompilationStatus EOFLexemeAction();
CompilationStatus IgnoredLexemeAction();
CompilationStatus IntegerLexemeAction();
CompilationStatus LeaveImportExpressionLexemeAction();
CompilationStatus LeaveMultilineCommentLexemeAction();
CompilationStatus ParenthesisLexemeAction(TokenLabel label);
CompilationStatus UnknownLexemeAction();

CompilationStatus KeywordLexemeAction(TokenLabel label);
CompilationStatus IdentifierLexemeAction();
CompilationStatus StringLiteralLexemeAction();
CompilationStatus OperatorLexemeAction(TokenLabel label);
CompilationStatus JoinTypeLexemeAction(enum JoinType type);
CompilationStatus TypeTokenLexemeAction(UdfParamType type);

#endif
