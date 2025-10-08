%{

#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonActions.h"

/**
 * The error reporting function for Bison parser.
 *
 * @todo Add location to the grammar and "pushToken" API function.
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Error-Reporting-Function.html
 * @see https://www.gnu.org/software/bison/manual/html_node/Tracking-Locations.html
 */
void yyerror(const YYLTYPE * location, const char * message) {}

%}

// You touch this, and you die.
%define api.pure full
%define api.push-pull push
%define api.value.union.name SemanticValue
%define parse.error detailed
%locations

%union {
	/** Terminals. */

	signed int integer;
	TokenLabel token;

	/** Non-terminals. */

	Constant * constant;
	Expression * expression;
	Factor * factor;
	Program * program;
	
	Statement * statement;
	StatementList * statements;
	ParamDeclaration * param_declaration;
	SourceDeclaration * source_declaration;
	DatasetDeclaration * dataset_declaration;
	TransformDeclaration * transform_declaration;
	SinkDeclaration * sink_declaration;
	WriteStatement * write_statement;
	PropertyList * source_properties;
	PropertyList * sink_properties;
	Property * source_property;
	Property * sink_property;
	TransformOperationList * transform_operations;
	TransformOperation * transform_operation;
	FilterOperation * filter_operation;
	WithColumnOperation * with_column_operation;
	SelectOperation * select_operation;
	ColumnAssignmentList * column_assignments;
	ColumnAssignment * column_assignment;
	ColumnList * column_list;
}

/**
 * Destructors. This functions are executed after the parsing ends, so if the
 * AST must be used in the following phases of the compiler you shouldn't used
 * this approach for the AST root node ("program" non-terminal, in this
 * grammar), or it will drop the entire tree even if the parsing succeeds.
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Destructor-Decl.html
 */
%destructor { destroyProgram($$); } <program>
%destructor { destroyConstant($$); } <constant>
%destructor { destroyExpression($$); } <expression>
%destructor { destroyFactor($$); } <factor>
%destructor { destroyStatementList($$); } <statements>
%destructor { destroyStatement($$); } <statement>

/** Terminals. */
%token <integer> INTEGER
%token <token> ADD
%token <token> CLOSE_BRACE
%token <token> CLOSE_COMMENT
%token <token> CLOSE_PARENTHESIS
%token <token> DIV
%token <token> MUL
%token <token> OPEN_BRACE
%token <token> OPEN_COMMENT
%token <token> OPEN_PARENTHESIS
%token <token> SUB

/* Keywords */
%token <token> PARAM
%token <token> SOURCE
%token <token> DATASET
%token <token> TRANSFORM
%token <token> SINK
%token <token> WRITE
%token <token> FROM
%token <token> USE
%token <token> FILTER
%token <token> WITH_COLUMN
%token <token> SELECT
%token <token> INTO

/* Operators and Punctuation */
%token <token> ASSIGN
%token <token> COLON
%token <token> COMMA
%token <token> GREATER
%token <token> LESS
%token <token> AND
%token <token> OR
%token <token> IS
%token <token> NULL_TOKEN
%token <token> TRUE_TOKEN
%token <token> FALSE_TOKEN


%token <token> IDENTIFIER
%token <token> STRING_LITERAL

%token <token> IGNORED
%token <token> UNKNOWN

/** Non-terminals. */
%type <constant> constant
%type <expression> expression
%type <factor> factor
%type <program> program

%type <statements> statements
%type <statement> statement
%type <param_declaration> param_declaration
%type <source_declaration> source_declaration
%type <dataset_declaration> dataset_declaration
%type <transform_declaration> transform_declaration
%type <sink_declaration> sink_declaration
%type <write_statement> write_statement
%type <source_properties> source_properties
%type <sink_properties> sink_properties
%type <source_property> source_property
%type <sink_property> sink_property
%type <transform_operations> transform_operations
%type <transform_operation> transform_operation
%type <filter_operation> filter_operation
%type <with_column_operation> with_column_operation
%type <select_operation> select_operation
%type <column_assignments> column_assignments
%type <column_assignment> column_assignment
%type <column_list> column_list

/**
 * Precedence and associativity.
 *
 * @see https://en.cppreference.com/w/cpp/language/operator_precedence.html
 * @see https://www.gnu.org/software/bison/manual/html_node/Precedence.html
 */
%left ADD SUB
%left MUL DIV

%%

// IMPORTANT: To use λ in the following grammar, use the %empty symbol.
//program: expression											{ $$ = ExpressionProgramSemanticAction($1); }
program: statements											{ $$ = StatementsSemanticAction($1); }
	;

statements: statement										{ $$ = CreateStatementsSemanticAction($1); }
	| statements statement									{ $$ = AddStatementSemanticAction($1, $2); }
	;

statement: param_declaration								{ $$ = ParamDeclarationSemanticAction($1); }
	| source_declaration									{ $$ = SourceDeclarationSemanticAction($1); }
	| dataset_declaration									{ $$ = DatasetDeclarationSemanticAction($1); }
	| transform_declaration									{ $$ = TransformDeclarationSemanticAction($1); }
	| sink_declaration										{ $$ = SinkDeclarationSemanticAction($1); }
	| write_statement										{ $$ = WriteStatementSemanticAction($1); }
	;

param_declaration: PARAM IDENTIFIER ASSIGN STRING_LITERAL
					{ $$ = CreateParamSemanticAction($2, $4); }
	;

source_declaration: SOURCE IDENTIFIER OPEN_BRACE source_properties CLOSE_BRACE
					{ $$ = CreateSourceSemanticAction($2, $4); }
	;

dataset_declaration: DATASET IDENTIFIER FROM IDENTIFIER
					{ $$ = CreateDatasetSemanticAction($2, $4); }
	;

transform_declaration: TRANSFORM IDENTIFIER ASSIGN FROM IDENTIFIER USE transform_operations
					{ $$ = CreateTransformSemanticAction($2, $5, $7); }
	;

sink_declaration: SINK IDENTIFIER OPEN_BRACE sink_properties CLOSE_BRACE
					{ $$ = CreateSinkSemanticAction($2, $4); }
	;

write_statement: WRITE IDENTIFIER INTO IDENTIFIER
					{ $$ = CreateWriteSemanticAction($2, $4); }
	;

source_properties: %empty								{ $$ = NULL; }
	| source_properties source_property					{ $$ = AddSourcePropertySemanticAction($1, $2); }
	;

source_property: IDENTIFIER ASSIGN STRING_LITERAL
					{ $$ = CreatePropertySemanticAction($1, $3); }
	;

sink_properties: %empty									{ $$ = NULL; }
	| sink_properties sink_property						{ $$ = AddSinkPropertySemanticAction($1, $2); }
	;

sink_property: IDENTIFIER ASSIGN STRING_LITERAL
					{ $$ = CreatePropertySemanticAction($1, $3); }
	;

transform_operations: %empty							{ $$ = NULL; }
	| transform_operations transform_operation			{ $$ = AddTransformOperationSemanticAction($1, $2); }
	;

transform_operation: filter_operation					{ $$ = FilterOperationSemanticAction($1); }
	| with_column_operation								{ $$ = WithColumnOperationSemanticAction($1); }
	| select_operation									{ $$ = SelectOperationSemanticAction($1); }
	;

filter_operation: FILTER OPEN_BRACE expression CLOSE_BRACE
					{ $$ = CreateFilterSemanticAction($3); }
	;

with_column_operation: WITH_COLUMN OPEN_BRACE column_assignments CLOSE_BRACE
					{ $$ = CreateWithColumnSemanticAction($3); }
	;

select_operation: SELECT OPEN_BRACE column_list CLOSE_BRACE
					{ $$ = CreateSelectSemanticAction($3); }
	;

column_assignments: column_assignment					{ $$ = CreateColumnAssignmentsSemanticAction($1); }
	| column_assignments COMMA column_assignment		{ $$ = AddColumnAssignmentSemanticAction($1, $3); }
	;

column_assignment: IDENTIFIER ASSIGN expression		{ $$ = CreateColumnAssignmentSemanticAction($1, $3); }
	;

column_list: IDENTIFIER									{ $$ = CreateColumnListSemanticAction($1); }
	| column_list COMMA IDENTIFIER						{ $$ = AddColumnSemanticAction($1, $3); }
	;

expression: expression[left] ADD expression[right]			{ $$ = ArithmeticExpressionSemanticAction($left, $right, ADDITION); }
	| expression[left] DIV expression[right]				{ $$ = ArithmeticExpressionSemanticAction($left, $right, DIVISION); }
	| expression[left] MUL expression[right]				{ $$ = ArithmeticExpressionSemanticAction($left, $right, MULTIPLICATION); }
	| expression[left] SUB expression[right]				{ $$ = ArithmeticExpressionSemanticAction($left, $right, SUBTRACTION); }
	| expression[left] GREATER expression[right]			{ $$ = ComparisonExpressionSemanticAction($left, $right, GREATER_THAN); }
	| expression[left] LESS expression[right]				{ $$ = ComparisonExpressionSemanticAction($left, $right, LESS_THAN); }
	| expression[left] ASSIGN expression[right]				{ $$ = ComparisonExpressionSemanticAction($left, $right, EQUALS_OP); }
	| expression[left] AND expression[right]				{ $$ = LogicalExpressionSemanticAction($left, $right, AND_OP); }
	| expression[left] OR expression[right]					{ $$ = LogicalExpressionSemanticAction($left, $right, OR_OP); }
	| factor												{ $$ = FactorExpressionSemanticAction($1); }
	;

factor: OPEN_PARENTHESIS expression CLOSE_PARENTHESIS		{ $$ = ExpressionFactorSemanticAction($2); }
	| constant												{ $$ = ConstantFactorSemanticAction($1); }
	| IDENTIFIER											{ $$ = IdentifierFactorSemanticAction($1); }
	;

constant: INTEGER											{ $$ = IntegerConstantSemanticAction($1); }
	| STRING_LITERAL										{ $$ = StringConstantSemanticAction($1); }
	| TRUE_TOKEN											{ $$ = BooleanConstantSemanticAction(true); }
	| FALSE_TOKEN											{ $$ = BooleanConstantSemanticAction(false); }
	| NULL_TOKEN											{ $$ = NullConstantSemanticAction(); }
	;

%%
