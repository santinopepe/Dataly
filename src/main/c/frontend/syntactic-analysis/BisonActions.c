#include "BisonActions.h"
#include <string.h>
#include <stdbool.h>

/* MODULE INTERNAL STATE */

static CompilerState * _compilerState = NULL;
static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownBisonActionsModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: BisonActions...");
		destroyLogger(_logger);
		_logger = NULL;
	}
	_compilerState = NULL;
}

ModuleDestructor initializeBisonActionsModule(CompilerState * compilerState) {
	_compilerState = compilerState;
	_logger = createLogger("BisonActions");
	return _shutdownBisonActionsModule;
}

/* IMPORTED FUNCTIONS */

/* PRIVATE FUNCTIONS */

static void _logSyntacticAnalyzerAction(const char * functionName);

/**
 * Logs a syntactic-analyzer action in DEBUGGING level.
 */
static void _logSyntacticAnalyzerAction(const char * functionName) {
	logDebugging(_logger, "%s", functionName);
}

/* PUBLIC FUNCTIONS */

Constant * IntegerConstantSemanticAction(const int value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Constant * constant = calloc(1, sizeof(Constant));
	constant->intValue = value;
	constant->value = value; 
	constant->type = INTEGER_CONST;
	return constant;
}

Expression * ArithmeticExpressionSemanticAction(Expression * leftExpression, Expression * rightExpression, ExpressionType type) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->leftExpression = leftExpression;
	expression->rightExpression = rightExpression;
	expression->type = type;
	return expression;
}

Expression * NotExpressionSemanticAction(Expression * expr) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->leftExpression = expr;
	expression->type = LOGICAL;
	expression->logOp = NOT_OP;
	return expression;
}

Expression * FactorExpressionSemanticAction(Factor * factor) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->factor = factor;
	expression->type = FACTOR;
	return expression;
}

Factor * ConstantFactorSemanticAction(Constant * constant) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Factor * factor = calloc(1, sizeof(Factor));
	factor->constant = constant;
	factor->type = CONSTANT;
	return factor;
}

Factor * ExpressionFactorSemanticAction(Expression * expression) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Factor * factor = calloc(1, sizeof(Factor));
	factor->expression = expression;
	factor->type = EXPRESSION;
	return factor;
}

Program * ExpressionProgramSemanticAction(Expression * expression) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Program * program = calloc(1, sizeof(Program));
	program->expression = expression;
	program->type = EXPRESSION_PROGRAM;

	if (_compilerState != NULL) {
		_compilerState->abstractSyntaxtTree = program;
	}
	return program;
}


static char* copyTokenText(TokenLabel token) {

	if (token == (TokenLabel) 0) {
		return NULL;
	}
	const char * text = (const char *) token;
	if (text == NULL) return NULL;
	char * dup = NULL;
	if (text != NULL) {
		dup = strdup(text);
	}
	return dup;
}



Program * StatementsSemanticAction(StatementList * statements) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Program * program = calloc(1, sizeof(Program));
	program->statements = statements;
	program->type = STATEMENT_LIST_PROGRAM;

	if (_compilerState != NULL) {
		_compilerState->abstractSyntaxtTree = program;
	}
	return program;
}

StatementList * CreateStatementsSemanticAction(Statement * statement) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	StatementList * statements = calloc(1, sizeof(StatementList));
	statements->statement = statement;
	statements->next = NULL;
	return statements;
}

StatementList * AddStatementSemanticAction(StatementList * statements, Statement * statement) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	StatementList * newStatement = calloc(1, sizeof(StatementList));
	newStatement->statement = statement;
	newStatement->next = NULL;
	
	if (statements == NULL) {
		return newStatement;
	}
	
	StatementList * current = statements;
	while (current->next != NULL) {
		current = current->next;
	}
	current->next = newStatement;
	return statements;
}

Statement * ParamDeclarationSemanticAction(ParamDeclaration * param) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Statement * statement = calloc(1, sizeof(Statement));
	statement->paramDecl = param;
	statement->type = PARAM_STMT;
	return statement;
}

Statement * SourceDeclarationSemanticAction(SourceDeclaration * source) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Statement * statement = calloc(1, sizeof(Statement));
	statement->sourceDecl = source;
	statement->type = SOURCE_STMT;
	return statement;
}

Statement * DatasetDeclarationSemanticAction(DatasetDeclaration * dataset) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Statement * statement = calloc(1, sizeof(Statement));
	statement->datasetDecl = dataset;
	statement->type = DATASET_STMT;
	return statement;
}

Statement * TransformDeclarationSemanticAction(TransformDeclaration * transform) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Statement * statement = calloc(1, sizeof(Statement));
	statement->transformDecl = transform;
	statement->type = TRANSFORM_STMT;
	return statement;
}

Statement * SinkDeclarationSemanticAction(SinkDeclaration * sink) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Statement * statement = calloc(1, sizeof(Statement));
	statement->sinkDecl = sink;
	statement->type = SINK_STMT;
	return statement;
}

Statement * WriteStatementSemanticAction(WriteStatement * write) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Statement * statement = calloc(1, sizeof(Statement));
	statement->writeStmt = write;
	statement->type = WRITE_STMT;
	return statement;
}

Statement * UdfDeclarationSemanticAction(UdfDeclaration * udf) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Statement * statement = calloc(1, sizeof(Statement));
	statement->udfDecl = udf;
	statement->type = UDF_STMT;
	return statement;
}

ParamDeclaration * CreateParamSemanticAction(TokenLabel name, TokenLabel value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	ParamDeclaration * param = calloc(1, sizeof(ParamDeclaration));
	param->name = copyTokenText(name);
	param->value = copyTokenText(value);
	return param;
}

SourceDeclaration * CreateSourceSemanticAction(TokenLabel name, PropertyList * properties) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	SourceDeclaration * source = calloc(1, sizeof(SourceDeclaration));
	source->name = copyTokenText(name);
	source->properties = properties;
	return source;
}

DatasetDeclaration * CreateDatasetSemanticAction(TokenLabel name, TokenLabel sourceName) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	DatasetDeclaration * dataset = calloc(1, sizeof(DatasetDeclaration));
	dataset->name = copyTokenText(name);
	dataset->sourceName = copyTokenText(sourceName);
	return dataset;
}

TransformDeclaration * CreateTransformSemanticAction(TokenLabel name, TokenLabel sourceName, TransformOperationList * operations) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	TransformDeclaration * transform = calloc(1, sizeof(TransformDeclaration));
	transform->name = copyTokenText(name);
	transform->sourceName = copyTokenText(sourceName);
	transform->operations = operations;
	return transform;
}

SinkDeclaration * CreateSinkSemanticAction(TokenLabel name, PropertyList * properties) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	SinkDeclaration * sink = calloc(1, sizeof(SinkDeclaration));
	sink->name = copyTokenText(name);
	sink->properties = properties;
	return sink;
}

WriteStatement * CreateWriteSemanticAction(TokenLabel datasetName, TokenLabel sinkName) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	WriteStatement * write = calloc(1, sizeof(WriteStatement));
	write->datasetName = copyTokenText(datasetName);
	write->sinkName = copyTokenText(sinkName);
	return write;
}

PropertyList * AddSourcePropertySemanticAction(PropertyList * properties, Property * property) {
	return AddSinkPropertySemanticAction(properties, property);
}

PropertyList * AddSinkPropertySemanticAction(PropertyList * properties, Property * property) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	PropertyList * newProperty = calloc(1, sizeof(PropertyList));
	newProperty->property = property;
	newProperty->next = NULL;
	
	if (properties == NULL) {
		return newProperty;
	}
	
	PropertyList * current = properties;
	while (current->next != NULL) {
		current = current->next;
	}
	current->next = newProperty;
	return properties;
}

Property * CreatePropertySemanticAction(TokenLabel key, TokenLabel value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Property * property = calloc(1, sizeof(Property));
	property->key = copyTokenText(key);
	property->value = copyTokenText(value);
	return property;
}

TransformOperationList * AddTransformOperationSemanticAction(TransformOperationList * operations, TransformOperation * operation) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	TransformOperationList * newOp = calloc(1, sizeof(TransformOperationList));
	newOp->operation = operation;
	newOp->next = NULL;
	
	if (operations == NULL) {
		return newOp;
	}
	
	TransformOperationList * current = operations;
	while (current->next != NULL) {
		current = current->next;
	}
	current->next = newOp;
	return operations;
}

TransformOperation * FilterOperationSemanticAction(FilterOperation * filter) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	TransformOperation * operation = calloc(1, sizeof(TransformOperation));
	operation->filter = filter;
	operation->type = FILTER_OP;
	return operation;
}

TransformOperation * WithColumnOperationSemanticAction(WithColumnOperation * withColumn) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	TransformOperation * operation = calloc(1, sizeof(TransformOperation));
	operation->withColumn = withColumn;
	operation->type = WITH_COLUMN_OP;
	return operation;
}

TransformOperation * SelectOperationSemanticAction(SelectOperation * select) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	TransformOperation * operation = calloc(1, sizeof(TransformOperation));
	operation->select = select;
	operation->type = SELECT_OP;
	return operation;
}

TransformOperation * OrderByOperationSemanticAction(OrderByOperation * orderBy) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	TransformOperation * operation = calloc(1, sizeof(TransformOperation));
	operation->orderBy = orderBy;
	operation->type = ORDER_BY_OP;
	return operation;
}

TransformOperation * LimitOperationSemanticAction(LimitOperation * limit) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	TransformOperation * operation = calloc(1, sizeof(TransformOperation));
	operation->limitOp = limit;
	operation->type = LIMIT_OP;
	return operation;
}

TransformOperation * JoinOperationSemanticAction(JoinOperation * join) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	TransformOperation * operation = calloc(1, sizeof(TransformOperation));
	operation->join = join;
	operation->type = JOIN_OP;
	return operation;
}

TransformOperation * GroupByOperationSemanticAction(GroupByOperation * groupBy) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	TransformOperation * operation = calloc(1, sizeof(TransformOperation));
	operation->groupBy = groupBy;
	operation->type = GROUP_BY_OP;
	return operation;
}

TransformOperation * WindowOperationSemanticAction(WindowOperation * windowOp) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	TransformOperation * operation = calloc(1, sizeof(TransformOperation));
	operation->windowOp = windowOp;
	operation->type = WINDOW_OP;
	return operation;
}

FilterOperation * CreateFilterSemanticAction(Expression * condition) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	FilterOperation * filter = calloc(1, sizeof(FilterOperation));
	filter->condition = condition;
	return filter;
}

WithColumnOperation * CreateWithColumnSemanticAction(ColumnAssignmentList * assignments) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	WithColumnOperation * withColumn = calloc(1, sizeof(WithColumnOperation));
	withColumn->assignments = assignments;
	return withColumn;
}

SelectOperation * CreateSelectSemanticAction(ColumnList * columns) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	SelectOperation * select = calloc(1, sizeof(SelectOperation));
	select->columns = columns;
	return select;
}

OrderByOperation * CreateOrderBySemanticAction(OrderByItem * items) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	OrderByOperation * orderBy = calloc(1, sizeof(OrderByOperation));
	orderBy->items = items;
	return orderBy;
}

LimitOperation * CreateLimitSemanticAction(int limit) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	LimitOperation * op = calloc(1, sizeof(LimitOperation));
	op->limit = limit;
	return op;
}

JoinOperation * CreateJoinSemanticAction(enum JoinType type, TokenLabel target, Expression * condition) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	JoinOperation * op = calloc(1, sizeof(JoinOperation));
	op->joinType = type;
	op->target = copyTokenText(target);
	op->condition = condition;
	return op;
}

GroupByOperation * CreateGroupBySemanticAction(ColumnList * keys, ColumnAssignmentList * aggregations) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	GroupByOperation * op = calloc(1, sizeof(GroupByOperation));
	op->keys = keys;
	op->aggregations = aggregations;
	return op;
}

WindowOperation * CreateWindowSemanticAction(ColumnList * partitionBy, OrderByItem * orderBy, ColumnAssignmentList * computations) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	WindowOperation * op = calloc(1, sizeof(WindowOperation));
	op->partitionBy = partitionBy;
	op->orderBy = orderBy;
	op->computations = computations;
	return op;
}

OrderByItem * CreateOrderByItemSemanticAction(Expression * expr, bool ascending) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	OrderByItem * item = calloc(1, sizeof(OrderByItem));
	item->expression = expr;
	item->ascending = ascending;
	return item;
}

OrderByItem * AddOrderByItemSemanticAction(OrderByItem * list, OrderByItem * item) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	if (list == NULL) {
		return item;
	}
	OrderByItem * current = list;
	while (current->next != NULL) current = current->next;
	current->next = item;
	return list;
}

ColumnAssignmentList * CreateColumnAssignmentsSemanticAction(ColumnAssignment * assignment) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	ColumnAssignmentList * assignments = calloc(1, sizeof(ColumnAssignmentList));
	assignments->assignment = assignment;
	assignments->next = NULL;
	return assignments;
}

ColumnAssignmentList * AddColumnAssignmentSemanticAction(ColumnAssignmentList * assignments, ColumnAssignment * assignment) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	ColumnAssignmentList * newAssignment = calloc(1, sizeof(ColumnAssignmentList));
	newAssignment->assignment = assignment;
	newAssignment->next = NULL;
	
	if (assignments == NULL) {
		return newAssignment;
	}
	
	ColumnAssignmentList * current = assignments;
	while (current->next != NULL) {
		current = current->next;
	}
	current->next = newAssignment;
	return assignments;
}

ColumnAssignment * CreateColumnAssignmentSemanticAction(TokenLabel name, Expression * expression) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	ColumnAssignment * assignment = calloc(1, sizeof(ColumnAssignment));
	assignment->columnName = copyTokenText(name);
	assignment->expression = expression;
	return assignment;
}

ColumnList * CreateColumnListSemanticAction(Expression * expression, TokenLabel alias) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	ColumnList * columns = calloc(1, sizeof(ColumnList));
	columns->expression = expression;
	columns->alias = copyTokenText(alias);
	columns->next = NULL;
	return columns;
}

ColumnList * AddColumnSemanticAction(ColumnList * columns, Expression * expression, TokenLabel alias) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	ColumnList * newColumn = calloc(1, sizeof(ColumnList));
	newColumn->expression = expression;
	newColumn->alias = copyTokenText(alias);
	newColumn->next = NULL;
	
	if (columns == NULL) {
		return newColumn;
	}
	
	ColumnList * current = columns;
	while (current->next != NULL) {
		current = current->next;
	}
	current->next = newColumn;
	return columns;
}

ExpressionList * CreateExpressionListSemanticAction(Expression * expression) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	ExpressionList * list = calloc(1, sizeof(ExpressionList));
	list->expression = expression;
	list->next = NULL;
	return list;
}

ExpressionList * AddExpressionSemanticAction(ExpressionList * list, Expression * expression) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	ExpressionList * item = calloc(1, sizeof(ExpressionList));
	item->expression = expression;
	item->next = NULL;
	if (list == NULL) {
		return item;
	}
	ExpressionList * current = list;
	while (current->next != NULL) current = current->next;
	current->next = item;
	return list;
}

UdfParam * CreateUdfParamSemanticAction(TokenLabel name, UdfParamType type) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	UdfParam * param = calloc(1, sizeof(UdfParam));
	param->name = copyTokenText(name);
	param->type = type;
	return param;
}

UdfParamList * CreateUdfParamListSemanticAction(UdfParam * param) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	UdfParamList * list = calloc(1, sizeof(UdfParamList));
	list->param = param;
	list->next = NULL;
	return list;
}

UdfParamList * AddUdfParamSemanticAction(UdfParamList * list, UdfParam * param) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	UdfParamList * node = calloc(1, sizeof(UdfParamList));
	node->param = param;
	node->next = NULL;
	if (list == NULL) return node;
	UdfParamList * cur = list;
	while (cur->next != NULL) cur = cur->next;
	cur->next = node;
	return list;
}

UdfDeclaration * CreateUdfDeclarationSemanticAction(TokenLabel name, UdfParamList * params, UdfParamType returnType) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	UdfDeclaration * udf = calloc(1, sizeof(UdfDeclaration));
	udf->name = copyTokenText(name);
	udf->params = params;
	udf->returnType = returnType;
	return udf;
}


Constant * StringConstantSemanticAction(TokenLabel value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Constant * constant = calloc(1, sizeof(Constant));
	constant->stringValue = copyTokenText(value);
	constant->type = STRING_CONST;
	return constant;
}

Constant * BooleanConstantSemanticAction(bool value) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Constant * constant = calloc(1, sizeof(Constant));
	constant->boolValue = value;
	constant->type = BOOLEAN_CONST;
	return constant;
}

Constant * NullConstantSemanticAction() {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Constant * constant = calloc(1, sizeof(Constant));
	constant->type = NULL_CONST;
	return constant;
}

Factor * IdentifierFactorSemanticAction(TokenLabel identifier) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Factor * factor = calloc(1, sizeof(Factor));
	factor->identifier = copyTokenText(identifier);
	factor->type = IDENTIFIER_FACTOR;
	return factor;
}

Expression * FunctionCallExpressionSemanticAction(TokenLabel name, ExpressionList * arguments) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->functionCall.functionName = copyTokenText(name);
	expression->functionCall.arguments = arguments;
	expression->type = FUNCTION_CALL;
	return expression;
}

Expression * ComparisonExpressionSemanticAction(Expression * left, Expression * right, ComparisonOperator op) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->leftExpression = left;
	expression->rightExpression = right;
	expression->compOp = op;
	expression->type = COMPARISON;
	return expression;
}

Expression * LogicalExpressionSemanticAction(Expression * left, Expression * right, LogicalOperator op) {
	_logSyntacticAnalyzerAction(__FUNCTION__);
	Expression * expression = calloc(1, sizeof(Expression));
	expression->leftExpression = left;
	expression->rightExpression = right;
	expression->logOp = op;
	expression->type = LOGICAL;
	return expression;
}
