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
	OrderByOperation * order_by_operation;
	LimitOperation * limit_operation;
	JoinOperation * join_operation;
	GroupByOperation * group_by_operation;
	WindowOperation * window_operation;
	OrderByItem * order_items;
	ColumnAssignmentList * column_assignments;
	ColumnAssignment * column_assignment;
	ColumnList * column_list;
	ExpressionList * expression_list;
	enum JoinType join_type;
	UdfDeclaration * udf_declaration;
	UdfParamList * udf_params;
	UdfParam * udf_param;
	UdfParamType udf_type;
}

/**
 * Destructors. This functions are executed after the parsing ends, so if the
 * AST must be used in the following phases of the compiler you shouldn't used
 * this approach for the AST root node ("program" non-terminal, in this
 * grammar), or it will drop the entire tree even if the parsing succeeds.
 *
 * @see https://www.gnu.org/software/bison/manual/html_node/Destructor-Decl.html
 */
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
%token <token> ORDER_BY
%token <token> LIMIT
%token <token> JOIN
%token <token> ON
%token <token> GROUP_BY
%token <token> WINDOW
%token <token> PARTITION_BY
%token <token> ASC
%token <token> DESC
%token <join_type> JOIN_TYPE
%token <token> UDF
%token <token> RETURNS
%token <udf_type> TYPE_TOKEN

/* Operators and Punctuation */
%token <token> ASSIGN
%token <token> COLON
%token <token> COMMA
%token <token> GREATER
%token <token> LESS
%token <token> GREATER_EQUAL
%token <token> LESS_EQUAL
%token <token> NOT_EQUALS
%token <token> NOT
%token <token> AND
%token <token> OR
%token <token> IS
%token <token> NULL_TOKEN
%token <token> TRUE_TOKEN
%token <token> FALSE_TOKEN
%token <token> MOD


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
%type <order_by_operation> order_by_operation
%type <limit_operation> limit_operation
%type <join_operation> join_operation
%type <group_by_operation> group_by_operation
%type <window_operation> window_operation
%type <order_items> order_items order_item order_clause
%type <column_assignments> column_assignments
%type <column_assignment> column_assignment
%type <column_list> column_list partition_clause
%type <expression_list> expression_list
%type <join_type> join_type
%type <udf_declaration> udf_declaration
%type <udf_params> udf_params
%type <udf_param> udf_param
%type <column_assignments> window_body

/**
 * Precedence and associativity.
 */
%right NOT
%left OR
%left AND
%nonassoc IS
%left GREATER LESS GREATER_EQUAL LESS_EQUAL NOT_EQUALS ASSIGN
%left ADD SUB
%left MUL DIV MOD

%%

// IMPORTANT: To use λ in the following grammar, use the %empty symbol.
program: statements											{ $$ = StatementsSemanticAction($1); }
	| expression											{ $$ = ExpressionProgramSemanticAction($1); }
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
	| udf_declaration										{ $$ = UdfDeclarationSemanticAction($1); }
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

udf_declaration: UDF IDENTIFIER OPEN_PARENTHESIS udf_params CLOSE_PARENTHESIS RETURNS TYPE_TOKEN
					{ $$ = CreateUdfDeclarationSemanticAction($2, $4, $7); }
	;

udf_params: %empty										{ $$ = NULL; }
	| udf_param											{ $$ = CreateUdfParamListSemanticAction($1); }
	| udf_params COMMA udf_param						{ $$ = AddUdfParamSemanticAction($1, $3); }
	;

udf_param: IDENTIFIER TYPE_TOKEN						{ $$ = CreateUdfParamSemanticAction($1, $2); }
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
	| order_by_operation									{ $$ = OrderByOperationSemanticAction($1); }
	| limit_operation									{ $$ = LimitOperationSemanticAction($1); }
	| join_operation										{ $$ = JoinOperationSemanticAction($1); }
	| group_by_operation									{ $$ = GroupByOperationSemanticAction($1); }
	| window_operation									{ $$ = WindowOperationSemanticAction($1); }
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

order_by_operation: ORDER_BY OPEN_BRACE order_items CLOSE_BRACE
					{ $$ = CreateOrderBySemanticAction($3); }
	;

order_items: order_item								{ $$ = $1; }
	| order_items COMMA order_item					{ $$ = AddOrderByItemSemanticAction($1, $3); }
	;

order_item: expression								{ $$ = CreateOrderByItemSemanticAction($1, true); }
	| expression ASC								{ $$ = CreateOrderByItemSemanticAction($1, true); }
	| expression DESC								{ $$ = CreateOrderByItemSemanticAction($1, false); }
	;

limit_operation: LIMIT INTEGER						{ $$ = CreateLimitSemanticAction($2); }
	;

join_operation: JOIN join_type IDENTIFIER ON expression
					{ $$ = CreateJoinSemanticAction($2, $3, $5); }
	;

join_type: %empty									{ $$ = JOIN_INNER; }
	| JOIN_TYPE										{ $$ = $1; }
	;

group_by_operation: GROUP_BY OPEN_BRACE column_list CLOSE_BRACE OPEN_BRACE column_assignments CLOSE_BRACE
					{ $$ = CreateGroupBySemanticAction($3, $6); }
	;

window_operation: WINDOW OPEN_BRACE partition_clause order_clause window_body CLOSE_BRACE
					{ $$ = CreateWindowSemanticAction($3, $4, $5); }
	;

partition_clause: %empty							{ $$ = NULL; }
	| PARTITION_BY OPEN_BRACE column_list CLOSE_BRACE	{ $$ = $3; }
	;

window_body: column_assignments						{ $$ = $1; }
	| WITH_COLUMN OPEN_BRACE column_assignments CLOSE_BRACE	{ $$ = $3; }
	;

order_clause: %empty								{ $$ = NULL; }
	| ORDER_BY OPEN_BRACE order_items CLOSE_BRACE	{ $$ = $3; }
	;

column_assignments: column_assignment					{ $$ = CreateColumnAssignmentsSemanticAction($1); }
	| column_assignments COMMA column_assignment		{ $$ = AddColumnAssignmentSemanticAction($1, $3); }
	;

column_assignment: IDENTIFIER ASSIGN expression		{ $$ = CreateColumnAssignmentSemanticAction($1, $3); }
	;

column_list: expression								{ $$ = CreateColumnListSemanticAction($1, 0); }
	| column_list COMMA expression						{ $$ = AddColumnSemanticAction($1, $3, 0); }
	;

expression: expression[left] ADD expression[right]			{ $$ = ArithmeticExpressionSemanticAction($left, $right, ADDITION); }
	| expression[left] DIV expression[right]				{ $$ = ArithmeticExpressionSemanticAction($left, $right, DIVISION); }
	| expression[left] MUL expression[right]				{ $$ = ArithmeticExpressionSemanticAction($left, $right, MULTIPLICATION); }
	| expression[left] MOD expression[right]				{ $$ = ArithmeticExpressionSemanticAction($left, $right, MODULO); }
	| expression[left] SUB expression[right]				{ $$ = ArithmeticExpressionSemanticAction($left, $right, SUBTRACTION); }
	| expression[left] GREATER expression[right]			{ $$ = ComparisonExpressionSemanticAction($left, $right, GREATER_THAN); }
	| expression[left] LESS expression[right]				{ $$ = ComparisonExpressionSemanticAction($left, $right, LESS_THAN); }
	| expression[left] GREATER_EQUAL expression[right]		{ $$ = ComparisonExpressionSemanticAction($left, $right, GREATER_EQUAL_OP); }
	| expression[left] LESS_EQUAL expression[right]			{ $$ = ComparisonExpressionSemanticAction($left, $right, LESS_EQUAL_OP); }
	| expression[left] NOT_EQUALS expression[right]			{ $$ = ComparisonExpressionSemanticAction($left, $right, NOT_EQUALS_OP); }
	| expression[left] ASSIGN expression[right]				{ $$ = ComparisonExpressionSemanticAction($left, $right, EQUALS_OP); }
	| expression[left] IS NULL_TOKEN						{ $$ = ComparisonExpressionSemanticAction($left, FactorExpressionSemanticAction(ConstantFactorSemanticAction(NullConstantSemanticAction())), IS_NULL_OP); }
	| expression[left] IS NOT NULL_TOKEN					{ $$ = ComparisonExpressionSemanticAction($left, FactorExpressionSemanticAction(ConstantFactorSemanticAction(NullConstantSemanticAction())), IS_NOT_NULL_OP); }
	| expression[left] AND expression[right]				{ $$ = LogicalExpressionSemanticAction($left, $right, AND_OP); }
	| expression[left] OR expression[right]					{ $$ = LogicalExpressionSemanticAction($left, $right, OR_OP); }
	| NOT expression										{ $$ = NotExpressionSemanticAction($2); }
	| factor												{ $$ = FactorExpressionSemanticAction($1); }
	;

factor: OPEN_PARENTHESIS expression CLOSE_PARENTHESIS		{ $$ = ExpressionFactorSemanticAction($2); }
	| constant												{ $$ = ConstantFactorSemanticAction($1); }
	| IDENTIFIER											{ $$ = IdentifierFactorSemanticAction($1); }
	| IDENTIFIER OPEN_PARENTHESIS expression_list CLOSE_PARENTHESIS { $$ = ExpressionFactorSemanticAction(FunctionCallExpressionSemanticAction($1, $3)); }
	| IDENTIFIER OPEN_PARENTHESIS CLOSE_PARENTHESIS		{ $$ = ExpressionFactorSemanticAction(FunctionCallExpressionSemanticAction($1, NULL)); }
	;

expression_list: %empty									{ $$ = NULL; }
	| expression											{ $$ = CreateExpressionListSemanticAction($1); }
	| expression_list COMMA expression					{ $$ = AddExpressionSemanticAction($1, $3); }
	;

constant: INTEGER											{ $$ = IntegerConstantSemanticAction($1); }
	| STRING_LITERAL										{ $$ = StringConstantSemanticAction($1); }
	| TRUE_TOKEN											{ $$ = BooleanConstantSemanticAction(true); }
	| FALSE_TOKEN											{ $$ = BooleanConstantSemanticAction(false); }
	| NULL_TOKEN											{ $$ = NullConstantSemanticAction(); }
	;

%%
