#ifndef GENERATOR_HEADER
#define GENERATOR_HEADER

#include "../../support/type/CompilationStatus.h"
#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"

ModuleDestructor initializeGeneratorModule();

CompilationStatus executeGenerator(CompilerState *compilerState);

#endif
