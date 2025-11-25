#ifndef ABSTRACT_SYNTAX_TREE_HEADER
#define ABSTRACT_SYNTAX_TREE_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/ModuleDestructor.h"
#include <stdlib.h>
#include <stdbool.h>

/** Initialize module's internal state. */
ModuleDestructor initializeAbstractSyntaxTreeModule();

/**
 * This type definitions allows self-referencing types (e.g., an expression
 * that is made of another expressions, such as talking about you in 3rd
 * person, but without the madness).
 */

typedef enum ExpressionType ExpressionType;
typedef enum FactorType FactorType;
typedef enum ProgramType ProgramType;

typedef struct Constant Constant;
typedef struct Expression Expression;
typedef struct Factor Factor;
typedef struct Program Program;


typedef enum StatementType StatementType;
typedef enum ConstantType ConstantType;
typedef enum ComparisonOperator ComparisonOperator;
typedef enum LogicalOperator LogicalOperator;

typedef struct Statement Statement;
typedef struct StatementList StatementList;
typedef struct ParamDeclaration ParamDeclaration;
typedef struct SourceDeclaration SourceDeclaration;
typedef struct DatasetDeclaration DatasetDeclaration;
typedef struct TransformDeclaration TransformDeclaration;
typedef struct SinkDeclaration SinkDeclaration;
typedef struct WriteStatement WriteStatement;
typedef struct Property Property;
typedef struct PropertyList PropertyList;
typedef struct TransformOperation TransformOperation;
typedef struct TransformOperationList TransformOperationList;
typedef struct FilterOperation FilterOperation;
typedef struct WithColumnOperation WithColumnOperation;
typedef struct SelectOperation SelectOperation;
typedef struct ColumnAssignment ColumnAssignment;
typedef struct ColumnAssignmentList ColumnAssignmentList;
typedef struct ColumnList ColumnList;
typedef struct OrderByItem OrderByItem;
typedef struct OrderByList OrderByList;
typedef struct OrderByOperation OrderByOperation;
typedef struct LimitOperation LimitOperation;
typedef struct JoinOperation JoinOperation;
typedef struct GroupByOperation GroupByOperation;
typedef struct WindowOperation WindowOperation;
typedef struct ExpressionList ExpressionList;
typedef struct UdfDeclaration UdfDeclaration;
typedef struct UdfParam UdfParam;
typedef struct UdfParamList UdfParamList;

/**
 * Node types for the Abstract Syntax Tree (AST).
 */

enum ExpressionType {
	ADDITION,
	DIVISION,
	FACTOR,
	MULTIPLICATION,
	MODULO,
	SUBTRACTION,

	COMPARISON,
	LOGICAL,
	IDENTIFIER_EXPR,
	FUNCTION_CALL
};

enum FactorType {
	CONSTANT,
	EXPRESSION,
	IDENTIFIER_FACTOR
};

enum ProgramType {
	STATEMENT_LIST_PROGRAM,
	EXPRESSION_PROGRAM
};


enum StatementType {
	PARAM_STMT,
	SOURCE_STMT,
	DATASET_STMT,
	TRANSFORM_STMT,
	SINK_STMT,
	WRITE_STMT,
	UDF_STMT
};

enum ConstantType {
	INTEGER_CONST,
	STRING_CONST,
	BOOLEAN_CONST,
	NULL_CONST
};

enum ComparisonOperator {
	EQUALS_OP,
	GREATER_THAN,
	LESS_THAN,
	NOT_EQUALS_OP,
	GREATER_EQUAL_OP,
	LESS_EQUAL_OP,
	IS_NULL_OP,
	IS_NOT_NULL_OP
};

enum LogicalOperator {
	AND_OP,
	OR_OP,
	NOT_OP
};

enum JoinType {
	JOIN_INNER,
	JOIN_LEFT,
	JOIN_RIGHT,
	JOIN_FULL,
	JOIN_SEMI,
	JOIN_ANTI
};

typedef enum {
	UDF_TYPE_INT,
	UDF_TYPE_STRING,
	UDF_TYPE_BOOL,
	UDF_TYPE_DECIMAL,
	UDF_TYPE_UNKNOWN
} UdfParamType;

struct Constant {
	union {
		int intValue;
		char* stringValue;
		bool boolValue;
	};
	ConstantType type;
	int value; 
};

struct Factor {
	union {
		Constant * constant;
		Expression * expression;
		char* identifier;
	};
	FactorType type;
};

struct Expression {
	union {
		Factor * factor;
		struct {
			Expression * leftExpression;
			Expression * rightExpression;
			ComparisonOperator compOp;
			LogicalOperator logOp;
		};
		char* identifier;
		struct {
			char* functionName;
			ExpressionList* arguments;
		} functionCall;
	};
	ExpressionType type;
};

struct Program {
	union {
		StatementList * statements;
		Expression * expression;
	};
	ProgramType type;
};


struct Statement {
	union {
		ParamDeclaration* paramDecl;
		SourceDeclaration* sourceDecl;
		DatasetDeclaration* datasetDecl;
		TransformDeclaration* transformDecl;
		SinkDeclaration* sinkDecl;
		WriteStatement* writeStmt;
		UdfDeclaration* udfDecl;
	};
	StatementType type;
};

struct StatementList {
	Statement* statement;
	StatementList* next;
};

struct ParamDeclaration {
	char* name;
	char* value;
};

struct SourceDeclaration {
	char* name;
	PropertyList* properties;
};

struct DatasetDeclaration {
	char* name;
	char* sourceName;
};

struct TransformDeclaration {
	char* name;
	char* sourceName;
	TransformOperationList* operations;
};

struct SinkDeclaration {
	char* name;
	PropertyList* properties;
};

typedef struct UdfParam {
	char* name;
	UdfParamType type;
} UdfParam;

typedef struct UdfParamList {
	UdfParam* param;
	struct UdfParamList* next;
} UdfParamList;

typedef struct UdfDeclaration {
	char* name;
	UdfParamList* params;
	UdfParamType returnType;
} UdfDeclaration;

struct WriteStatement {
	char* datasetName;
	char* sinkName;
};

struct Property {
	char* key;
	char* value;
};

struct PropertyList {
	Property* property;
	PropertyList* next;
};

struct TransformOperation {
	union {
		FilterOperation* filter;
		WithColumnOperation* withColumn;
		SelectOperation* select;
		OrderByOperation* orderBy;
		LimitOperation* limitOp;
		JoinOperation* join;
		GroupByOperation* groupBy;
		WindowOperation* windowOp;
	};
	enum {
		FILTER_OP,
		WITH_COLUMN_OP,
		SELECT_OP,
	ORDER_BY_OP,
	LIMIT_OP,
	JOIN_OP,
	GROUP_BY_OP,
	WINDOW_OP
	} type;
};

struct TransformOperationList {
	TransformOperation* operation;
	TransformOperationList* next;
};

struct FilterOperation {
	Expression* condition;
};

struct WithColumnOperation {
	ColumnAssignmentList* assignments;
};

struct SelectOperation {
	ColumnList* columns;
};

struct ColumnAssignment {
	char* columnName;
	Expression* expression;
};

struct ColumnAssignmentList {
	ColumnAssignment* assignment;
	ColumnAssignmentList* next;
};

struct ColumnList {
	Expression* expression;
	char* alias;
	ColumnList* next;
};

struct OrderByItem {
	Expression* expression;
	bool ascending;
	OrderByItem* next;
};

struct OrderByList {
	OrderByItem* head;
};

struct OrderByOperation {
	OrderByItem* items;
};

struct LimitOperation {
	int limit;
};

struct JoinOperation {
	enum JoinType joinType;
	char* target;
	Expression* condition;
};

struct GroupByOperation {
	ColumnList* keys;
	ColumnAssignmentList* aggregations;
};

struct WindowOperation {
	ColumnList* partitionBy;
	OrderByItem* orderBy;
	ColumnAssignmentList* computations;
};

struct ExpressionList {
	Expression* expression;
	ExpressionList* next;
};

/**
 * Node recursive super-duper-trambolik-destructors.
 */

void destroyConstant(Constant * constant);
void destroyExpression(Expression * expression);
void destroyFactor(Factor * factor);
void destroyProgram(Program * program);


void destroyStatement(Statement * statement);
void destroyStatementList(StatementList * statements);
void destroyParamDeclaration(ParamDeclaration * param);
void destroySourceDeclaration(SourceDeclaration * source);
void destroyDatasetDeclaration(DatasetDeclaration * dataset);
void destroyTransformDeclaration(TransformDeclaration * transform);
void destroySinkDeclaration(SinkDeclaration * sink);
void destroyWriteStatement(WriteStatement * write);
void destroyProperty(Property * property);
void destroyPropertyList(PropertyList * properties);
void destroyTransformOperation(TransformOperation * operation);
void destroyTransformOperationList(TransformOperationList * operations);
void destroyFilterOperation(FilterOperation * filter);
void destroyWithColumnOperation(WithColumnOperation * withColumn);
void destroySelectOperation(SelectOperation * select);
void destroyColumnAssignment(ColumnAssignment * assignment);
void destroyColumnAssignmentList(ColumnAssignmentList * assignments);
void destroyColumnList(ColumnList * columns);
void destroyOrderByItems(OrderByItem * items);
void destroyOrderByList(OrderByList * list);
void destroyOrderByOperation(OrderByOperation * orderBy);
void destroyLimitOperation(LimitOperation * limit);
void destroyJoinOperation(JoinOperation * join);
void destroyGroupByOperation(GroupByOperation * groupBy);
void destroyWindowOperation(WindowOperation * windowOp);
void destroyExpressionList(ExpressionList * list);
void destroyUdfParam(UdfParam* param);
void destroyUdfParamList(UdfParamList* list);
void destroyUdfDeclaration(UdfDeclaration* udf);

#endif
