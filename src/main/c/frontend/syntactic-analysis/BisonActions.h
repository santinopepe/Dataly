#ifndef BISON_ACTIONS_HEADER
#define BISON_ACTIONS_HEADER

#include "../../support/logging/Logger.h"
#include "../../support/type/CompilerState.h"
#include "../../support/type/ModuleDestructor.h"
#include "../../support/type/TokenLabel.h"
#include "AbstractSyntaxTree.h"
#include "BisonParser.h"
#include <stdlib.h>

/** Initialize module's internal state. */
ModuleDestructor initializeBisonActionsModule();

/**
 * Bison semantic actions.
 */

Constant * IntegerConstantSemanticAction(const int value);
Constant * StringConstantSemanticAction(TokenLabel value);
Constant * BooleanConstantSemanticAction(bool value);
Constant * NullConstantSemanticAction();

Expression * ArithmeticExpressionSemanticAction(Expression * leftExpression, Expression * rightExpression, ExpressionType type);
Expression * ComparisonExpressionSemanticAction(Expression * left, Expression * right, ComparisonOperator op);
Expression * LogicalExpressionSemanticAction(Expression * left, Expression * right, LogicalOperator op);
Expression * NotExpressionSemanticAction(Expression * expr);
Expression * FactorExpressionSemanticAction(Factor * factor);
Expression * FunctionCallExpressionSemanticAction(TokenLabel name, ExpressionList * arguments);

Factor * ConstantFactorSemanticAction(Constant * constant);
Factor * ExpressionFactorSemanticAction(Expression * expression);
Factor * IdentifierFactorSemanticAction(TokenLabel identifier);

Program * ExpressionProgramSemanticAction(Expression * expression);
Program * StatementsSemanticAction(StatementList * statements);

// DSL Statement list functions
StatementList * CreateStatementsSemanticAction(Statement * statement);
StatementList * AddStatementSemanticAction(StatementList * statements, Statement * statement);

// DSL Statement functions
Statement * ParamDeclarationSemanticAction(ParamDeclaration * param);
Statement * SourceDeclarationSemanticAction(SourceDeclaration * source);
Statement * DatasetDeclarationSemanticAction(DatasetDeclaration * dataset);
Statement * TransformDeclarationSemanticAction(TransformDeclaration * transform);
Statement * SinkDeclarationSemanticAction(SinkDeclaration * sink);
Statement * WriteStatementSemanticAction(WriteStatement * write);
Statement * UdfDeclarationSemanticAction(UdfDeclaration * udf);

// DSL Declaration functions
ParamDeclaration * CreateParamSemanticAction(TokenLabel name, TokenLabel value);
SourceDeclaration * CreateSourceSemanticAction(TokenLabel name, PropertyList * properties);
DatasetDeclaration * CreateDatasetSemanticAction(TokenLabel name, TokenLabel sourceName);
TransformDeclaration * CreateTransformSemanticAction(TokenLabel name, TokenLabel sourceName, TransformOperationList * operations);
SinkDeclaration * CreateSinkSemanticAction(TokenLabel name, PropertyList * properties);
WriteStatement * CreateWriteSemanticAction(TokenLabel datasetName, TokenLabel sinkName);

// DSL Property functions
PropertyList * AddSourcePropertySemanticAction(PropertyList * properties, Property * property);
PropertyList * AddSinkPropertySemanticAction(PropertyList * properties, Property * property);
Property * CreatePropertySemanticAction(TokenLabel key, TokenLabel value);

// DSL Transform operation functions
TransformOperationList * AddTransformOperationSemanticAction(TransformOperationList * operations, TransformOperation * operation);
TransformOperation * FilterOperationSemanticAction(FilterOperation * filter);
TransformOperation * WithColumnOperationSemanticAction(WithColumnOperation * withColumn);
TransformOperation * SelectOperationSemanticAction(SelectOperation * select);
TransformOperation * OrderByOperationSemanticAction(OrderByOperation * orderBy);
TransformOperation * LimitOperationSemanticAction(LimitOperation * limit);
TransformOperation * JoinOperationSemanticAction(JoinOperation * join);
TransformOperation * GroupByOperationSemanticAction(GroupByOperation * groupBy);
TransformOperation * WindowOperationSemanticAction(WindowOperation * windowOp);

FilterOperation * CreateFilterSemanticAction(Expression * condition);
WithColumnOperation * CreateWithColumnSemanticAction(ColumnAssignmentList * assignments);
SelectOperation * CreateSelectSemanticAction(ColumnList * columns);
OrderByOperation * CreateOrderBySemanticAction(OrderByItem * items);
LimitOperation * CreateLimitSemanticAction(int limit);
JoinOperation * CreateJoinSemanticAction(enum JoinType type, TokenLabel target, Expression * condition);
GroupByOperation * CreateGroupBySemanticAction(ColumnList * keys, ColumnAssignmentList * aggregations);
WindowOperation * CreateWindowSemanticAction(ColumnList * partitionBy, OrderByItem * orderBy, ColumnAssignmentList * computations);
OrderByItem * CreateOrderByItemSemanticAction(Expression * expr, bool ascending);
OrderByItem * AddOrderByItemSemanticAction(OrderByItem * list, OrderByItem * item);

// DSL Column functions
ColumnAssignmentList * CreateColumnAssignmentsSemanticAction(ColumnAssignment * assignment);
ColumnAssignmentList * AddColumnAssignmentSemanticAction(ColumnAssignmentList * assignments, ColumnAssignment * assignment);
ColumnAssignment * CreateColumnAssignmentSemanticAction(TokenLabel name, Expression * expression);

ColumnList * CreateColumnListSemanticAction(Expression * expression, TokenLabel alias);
ColumnList * AddColumnSemanticAction(ColumnList * columns, Expression * expression, TokenLabel alias);

ExpressionList * CreateExpressionListSemanticAction(Expression * expression);
ExpressionList * AddExpressionSemanticAction(ExpressionList * list, Expression * expression);

UdfParam * CreateUdfParamSemanticAction(TokenLabel name, UdfParamType type);
UdfParamList * CreateUdfParamListSemanticAction(UdfParam * param);
UdfParamList * AddUdfParamSemanticAction(UdfParamList * list, UdfParam * param);
UdfDeclaration * CreateUdfDeclarationSemanticAction(TokenLabel name, UdfParamList * params, UdfParamType returnType);

#endif
