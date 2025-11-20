#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "../syntactic-analysis/AbstractSyntaxTree.h"
#include "../../support/type/CompilationStatus.h"

CompilationStatus executeSemanticAnalysis(Program *program);

#endif