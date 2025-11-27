#include "AbstractSyntaxTree.h"

/* MODULE INTERNAL STATE */

static Logger * _logger = NULL;

/** Shutdown module's internal state. */
void _shutdownAbstractSyntaxTreeModule() {
	if (_logger != NULL) {
		logDebugging(_logger, "Destroying module: AbstractSyntaxTree...");
		destroyLogger(_logger);
		_logger = NULL;
	}
}

ModuleDestructor initializeAbstractSyntaxTreeModule() {
	_logger = createLogger("AbstractSyntaxTree");
	return _shutdownAbstractSyntaxTreeModule;
}

/* PUBLIC FUNCTIONS */

void destroyConstant(Constant * constant) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (constant != NULL) {
		if (constant->type == STRING_CONST && constant->stringValue != NULL) {
			free(constant->stringValue);
		}
		free(constant);
	}
}

void destroyExpression(Expression * expression) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (expression != NULL) {
	switch (expression->type) {
		case ADDITION:
		case DIVISION:
		case MULTIPLICATION:
		case MODULO:
		case SUBTRACTION:
		case COMPARISON:
		case LOGICAL:
			destroyExpression(expression->leftExpression);
			destroyExpression(expression->rightExpression);
				break;
			case FACTOR:
				destroyFactor(expression->factor);
				break;
			case IDENTIFIER_EXPR:
				if (expression->identifier != NULL) {
					free(expression->identifier);
				}
				break;
		case FUNCTION_CALL:
			if (expression->functionCall.functionName != NULL) {
				free(expression->functionCall.functionName);
			}
			destroyExpressionList(expression->functionCall.arguments);
			break;
		default:
			break;
	}
	free(expression);
}
}

void destroyFactor(Factor * factor) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (factor != NULL) {
		switch (factor->type) {
			case CONSTANT:
				destroyConstant(factor->constant);
				break;
			case EXPRESSION:
				destroyExpression(factor->expression);
				break;
			case IDENTIFIER_FACTOR:
				if (factor->identifier != NULL) {
					free(factor->identifier);
				}
				break;
			default:
				break;
		}
		free(factor);
	}
}

void destroyProgram(Program * program) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (program != NULL) {
		if (program->type == EXPRESSION_PROGRAM && program->expression != NULL) {
			destroyExpression(program->expression);
		} else if (program->type == STATEMENT_LIST_PROGRAM && program->statements != NULL) {
			destroyStatementList(program->statements);
		}
		free(program);
	}
}



void destroyStatement(Statement * statement) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (statement != NULL) {
		switch (statement->type) {
			case PARAM_STMT:
				destroyParamDeclaration(statement->paramDecl);
				break;
			case SOURCE_STMT:
				destroySourceDeclaration(statement->sourceDecl);
				break;
			case DATASET_STMT:
				destroyDatasetDeclaration(statement->datasetDecl);
				break;
			case TRANSFORM_STMT:
				destroyTransformDeclaration(statement->transformDecl);
				break;
		case SINK_STMT:
			destroySinkDeclaration(statement->sinkDecl);
			break;
		case WRITE_STMT:
			destroyWriteStatement(statement->writeStmt);
			break;
		case UDF_STMT:
			destroyUdfDeclaration(statement->udfDecl);
			break;
		default:
			break;
	}
	free(statement);
}
}

void destroyStatementList(StatementList * statements) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (statements != NULL) {
		destroyStatement(statements->statement);
		destroyStatementList(statements->next);
		free(statements);
	}
}

void destroyParamDeclaration(ParamDeclaration * param) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (param != NULL) {
		if (param->name != NULL) free(param->name);
		if (param->value != NULL) free(param->value);
		free(param);
	}
}

void destroySourceDeclaration(SourceDeclaration * source) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (source != NULL) {
		if (source->name != NULL) free(source->name);
		destroyPropertyList(source->properties);
		free(source);
	}
}

void destroyDatasetDeclaration(DatasetDeclaration * dataset) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (dataset != NULL) {
		if (dataset->name != NULL) free(dataset->name);
		if (dataset->sourceName != NULL) free(dataset->sourceName);
		free(dataset);
	}
}

void destroyTransformDeclaration(TransformDeclaration * transform) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (transform != NULL) {
		if (transform->name != NULL) free(transform->name);
		if (transform->sourceName != NULL) free(transform->sourceName);
		destroyTransformOperationList(transform->operations);
		free(transform);
	}
}

void destroySinkDeclaration(SinkDeclaration * sink) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (sink != NULL) {
		if (sink->name != NULL) free(sink->name);
		destroyPropertyList(sink->properties);
		free(sink);
	}
}

void destroyWriteStatement(WriteStatement * write) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (write != NULL) {
		if (write->datasetName != NULL) free(write->datasetName);
		if (write->sinkName != NULL) free(write->sinkName);
		free(write);
	}
}

void destroyProperty(Property * property) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (property != NULL) {
		if (property->key != NULL) free(property->key);
		if (property->value != NULL) free(property->value);
		free(property);
	}
}

void destroyPropertyList(PropertyList * properties) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (properties != NULL) {
		destroyProperty(properties->property);
		destroyPropertyList(properties->next);
		free(properties);
	}
}

void destroyTransformOperation(TransformOperation * operation) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (operation != NULL) {
	switch (operation->type) {
		case FILTER_OP:
			destroyFilterOperation(operation->filter);
			break;
		case WITH_COLUMN_OP:
			destroyWithColumnOperation(operation->withColumn);
			break;
		case SELECT_OP:
			destroySelectOperation(operation->select);
			break;
		case ORDER_BY_OP:
			destroyOrderByOperation(operation->orderBy);
			break;
		case LIMIT_OP:
			destroyLimitOperation(operation->limitOp);
			break;
		case JOIN_OP:
			destroyJoinOperation(operation->join);
			break;
		case GROUP_BY_OP:
			destroyGroupByOperation(operation->groupBy);
			break;
		case WINDOW_OP:
			destroyWindowOperation(operation->windowOp);
			break;
		default:
			break;
	}
	free(operation);
}
}

void destroyTransformOperationList(TransformOperationList * operations) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (operations != NULL) {
		destroyTransformOperation(operations->operation);
		destroyTransformOperationList(operations->next);
		free(operations);
	}
}

void destroyFilterOperation(FilterOperation * filter) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (filter != NULL) {
		destroyExpression(filter->condition);
		free(filter);
	}
}

void destroyWithColumnOperation(WithColumnOperation * withColumn) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (withColumn != NULL) {
		destroyColumnAssignmentList(withColumn->assignments);
		free(withColumn);
	}
}

void destroySelectOperation(SelectOperation * select) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (select != NULL) {
		destroyColumnList(select->columns);
		free(select);
	}
}

void destroyColumnAssignment(ColumnAssignment * assignment) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (assignment != NULL) {
		if (assignment->columnName != NULL) free(assignment->columnName);
		destroyExpression(assignment->expression);
		free(assignment);
	}
}

void destroyColumnAssignmentList(ColumnAssignmentList * assignments) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (assignments != NULL) {
		destroyColumnAssignment(assignments->assignment);
		destroyColumnAssignmentList(assignments->next);
		free(assignments);
	}
}

void destroyColumnList(ColumnList * columns) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (columns != NULL) {
		destroyExpression(columns->expression);
		if (columns->alias != NULL) free(columns->alias);
		destroyColumnList(columns->next);
		free(columns);
	}
}

void destroyUdfParam(UdfParam* param) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (!param) return;
	if (param->name) free(param->name);
	free(param);
}

void destroyUdfParamList(UdfParamList* list) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (!list) return;
	destroyUdfParam(list->param);
	destroyUdfParamList(list->next);
	free(list);
}

void destroyUdfDeclaration(UdfDeclaration* udf) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (!udf) return;
	if (udf->name) free(udf->name);
	destroyUdfParamList(udf->params);
	free(udf);
}

void destroyOrderByItems(OrderByItem * items) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (items != NULL) {
		destroyExpression(items->expression);
		destroyOrderByItems(items->next);
		free(items);
	}
}

void destroyOrderByList(OrderByList * list) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (list != NULL) {
		destroyOrderByItems(list->head);
		free(list);
	}
}

void destroyOrderByOperation(OrderByOperation * orderBy) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (orderBy != NULL) {
		destroyOrderByItems(orderBy->items);
		free(orderBy);
	}
}

void destroyLimitOperation(LimitOperation * limit) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (limit != NULL) {
		free(limit);
	}
}

void destroyJoinOperation(JoinOperation * join) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (join != NULL) {
		if (join->target != NULL) free(join->target);
		destroyExpression(join->condition);
		free(join);
	}
}

void destroyGroupByOperation(GroupByOperation * groupBy) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (groupBy != NULL) {
		destroyColumnList(groupBy->keys);
		destroyColumnAssignmentList(groupBy->aggregations);
		free(groupBy);
	}
}

void destroyWindowOperation(WindowOperation * windowOp) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (windowOp != NULL) {
		destroyColumnList(windowOp->partitionBy);
		destroyOrderByItems(windowOp->orderBy);
		destroyColumnAssignmentList(windowOp->computations);
		free(windowOp);
	}
}

void destroyExpressionList(ExpressionList * list) {
	logDebugging(_logger, "Executing destructor: %s", __FUNCTION__);
	if (list != NULL) {
		destroyExpression(list->expression);
		destroyExpressionList(list->next);
		free(list);
	}
}
