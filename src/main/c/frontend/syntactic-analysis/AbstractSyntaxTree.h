#ifndef ABSTRACT_SYNTAX_TREE_HEADER
#define ABSTRACT_SYNTAX_TREE_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/ModuleDestructor.h"
#include <stdlib.h>

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

// DSL Types
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

/**
 * Node types for the Abstract Syntax Tree (AST).
 */

enum ExpressionType {
	ADDITION,
	DIVISION,
	FACTOR,
	MULTIPLICATION,
	SUBTRACTION,
	// DSL Expression types
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

// DSL Enums
enum StatementType {
	PARAM_STMT,
	SOURCE_STMT,
	DATASET_STMT,
	TRANSFORM_STMT,
	SINK_STMT,
	WRITE_STMT
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
	LESS_THAN
};

enum LogicalOperator {
	AND_OP,
	OR_OP
};

struct Constant {
	union {
		int intValue;
		char* stringValue;
		bool boolValue;
	};
	ConstantType type;
	int value; // For backend compatibility
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
			Expression* argument;
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

// DSL Structures
struct Statement {
	union {
		ParamDeclaration* paramDecl;
		SourceDeclaration* sourceDecl;
		DatasetDeclaration* datasetDecl;
		TransformDeclaration* transformDecl;
		SinkDeclaration* sinkDecl;
		WriteStatement* writeStmt;
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
	};
	enum {
		FILTER_OP,
		WITH_COLUMN_OP,
		SELECT_OP
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
	char* columnName;
	ColumnList* next;
};

/**
 * Node recursive super-duper-trambolik-destructors.
 */

void destroyConstant(Constant * constant);
void destroyExpression(Expression * expression);
void destroyFactor(Factor * factor);
void destroyProgram(Program * program);

// DSL Destructors
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

#endif
