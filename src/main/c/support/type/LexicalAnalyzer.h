#ifndef LEXICAL_ANALYZER_HEADER
#define LEXICAL_ANALYZER_HEADER

#include "../logging/Logger.h"


typedef struct {
	Logger * logger;
	void * location;
	void * parser;
	void * scanner;

    struct LexemeNode * lexemePool;
} LexicalAnalyzer;

#endif

