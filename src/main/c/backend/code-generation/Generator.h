#ifndef GENERATOR_HEADER
#define GENERATOR_HEADER

#include "../../support/type/CompilationStatus.h"
#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"

// Inicializa el módulo (logger, etc.).
ModuleDestructor initializeGeneratorModule();

// Ejecuta la generación de código para el AST ya construido y validado.
CompilationStatus executeGenerator(CompilerState *compilerState);

#endif
