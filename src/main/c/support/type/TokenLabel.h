#ifndef TOKEN_LABEL_HEADER
#define TOKEN_LABEL_HEADER

#include <stdint.h>

/**
 * The type of a Bison token label. Use a pointer-sized signed integer so we
 * can safely store pointers (casted) in token values on both 32- and
 * 64-bit platforms. This project uses the token value to transport a
 * lexeme pointer from the lexer to the parser for identifiers and
 * string literals.
 */
typedef intptr_t TokenLabel;

#endif
