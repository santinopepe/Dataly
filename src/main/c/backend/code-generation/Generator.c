#include "Generator.h"
#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../support/logging/Logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>

/* ============ Estado interno del módulo ============ */

static Logger * _logger = NULL;
static const char* joinTypeToString(enum JoinType type);
static const char* joinTypeToPandasHow(enum JoinType type);
static const char* udfTypeToString(UdfParamType type);

static void _shutdownGeneratorModule() {
    if (_logger != NULL) {
        logDebugging(_logger, "Destroying module: Generator...");
        destroyLogger(_logger);
        _logger = NULL;
    }
}

ModuleDestructor initializeGeneratorModule() {
    _logger = createLogger("Generator");
    return _shutdownGeneratorModule;
}

/* ============ Abstracción de salida ============ */

static FILE * _dotFile = NULL;
static FILE * _manifestFile = NULL;
static FILE * _pythonFile = NULL;

static void openOutputs() {
    _dotFile = fopen("pipeline.dot", "w");
    _manifestFile = fopen("manifest.json", "w");
    _pythonFile = fopen("run_pipeline.py", "w");
    if (!_dotFile || !_manifestFile || !_pythonFile) {
        logError(_logger, "Cannot open output files pipeline.dot / manifest.json / run_pipeline.py");
    }
}

static void closeOutputs() {
    if (_dotFile) fclose(_dotFile);
    if (_manifestFile) fclose(_manifestFile);
    if (_pythonFile) fclose(_pythonFile);
}

static void dotOut(const char *fmt, ...) {
    if (!_dotFile) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(_dotFile, fmt, args);
    va_end(args);
}

static void manifestOut(const char *fmt, ...) {
    if (!_manifestFile) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(_manifestFile, fmt, args);
    va_end(args);
}

static void pyOut(const char *fmt, ...) {
    if (!_pythonFile) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(_pythonFile, fmt, args);
    va_end(args);
}

/* ============ Helpers de strings para Python ============ */

static void sanitizeStringLiteral(const char* input, char* output, size_t outSize) {
    if (!input || outSize == 0) {
        if (outSize > 0) output[0] = '\0';
        return;
    }

    const char* start = input;
    size_t len = strlen(input);
    while (len >= 2 &&
           ((start[0] == '"' && start[len - 1] == '"') ||
            (start[0] == '\'' && start[len - 1] == '\''))) {
        start += 1;
        len -= 2;
    }

    size_t j = 0;
    for (size_t i = 0; i < len && j + 1 < outSize; ++i) {
        char c = start[i];
        if (c == '\\' || c == '\'') {
            if (j + 2 >= outSize) break;
            output[j++] = '\\';
        }
        output[j++] = c;
    }
    output[j] = '\0';
}

static void emitPyStringLiteral(const char* raw) {
    char buf[1024];
    sanitizeStringLiteral(raw, buf, sizeof(buf));
    pyOut("'%s'", buf);
}

static void emitJsonString(const char* raw) {
    if (!raw) { manifestOut("\"\""); return; }

    const char* start = raw;
    size_t len = strlen(raw);
    while (len >= 2 &&
           ((start[0] == '"' && start[len - 1] == '"') ||
            (start[0] == '\'' && start[len - 1] == '\''))) {
        start += 1;
        len -= 2;
    }

    manifestOut("\"");
    for (size_t i = 0; i < len; ++i) {
        unsigned char c = (unsigned char)start[i];
        switch (c) {
            case '"': manifestOut("\\\""); break;
            case '\\': manifestOut("\\\\"); break;
            case '\b': manifestOut("\\b"); break;
            case '\f': manifestOut("\\f"); break;
            case '\n': manifestOut("\\n"); break;
            case '\r': manifestOut("\\r"); break;
            case '\t': manifestOut("\\t"); break;
            default:
                if (c < 0x20) {
                    manifestOut("\\u%04x", c);
                } else {
                    manifestOut("%c", c);
                }
        }
    }
    manifestOut("\"");
}

static void toUpperSnake(const char* input, char* output, size_t outSize) {
    size_t j = 0;
    for (size_t i = 0; input && input[i] && j + 1 < outSize; ++i) {
        char c = input[i];
        if (c == '-') c = '_';
        output[j++] = (char)toupper((unsigned char)c);
    }
    output[j] = '\0';
}

/* ============ Schema helpers for manifest ============ */

typedef struct {
    char* name;
} ColumnInfo;

typedef struct {
    ColumnInfo* columns;
    size_t count;
} SchemaInfo;

typedef struct {
    char* name;
    SchemaInfo schema;
} NamedSchema;

static void freeSchemaInfo(SchemaInfo* schema) {
    if (!schema) return;
    for (size_t i = 0; i < schema->count; i++) {
        free(schema->columns[i].name);
    }
    free(schema->columns);
    schema->columns = NULL;
    schema->count = 0;
}

static void addColumnName(SchemaInfo* schema, const char* name) {
    if (!schema || !name) return;
    for (size_t i = 0; i < schema->count; i++) {
        if (schema->columns[i].name && strcmp(schema->columns[i].name, name) == 0) return;
    }
    schema->columns = realloc(schema->columns, (schema->count + 1) * sizeof(ColumnInfo));
    schema->columns[schema->count].name = strdup(name);
    schema->count += 1;
}

static SchemaInfo copySchemaInfo(const SchemaInfo* src) {
    SchemaInfo out = {0};
    if (!src) return out;
    out.columns = calloc(src->count, sizeof(ColumnInfo));
    out.count = src->count;
    for (size_t i = 0; i < src->count; i++) {
        if (src->columns[i].name) out.columns[i].name = strdup(src->columns[i].name);
    }
    return out;
}

static int findSchemaIndex(NamedSchema* arr, size_t count, const char* name) {
    if (!arr || !name) return -1;
    for (size_t i = 0; i < count; i++) {
        if (arr[i].name && strcmp(arr[i].name, name) == 0) return (int)i;
    }
    return -1;
}

static void writeProperties(PropertyList *properties) {
    manifestOut("{");
    bool first = true;
    for (PropertyList *pl = properties; pl != NULL; pl = pl->next) {
        Property *p = pl->property;
        if (!p || !p->key || !p->value) continue;
        if (!first) manifestOut(", ");
        first = false;
        manifestOut("\"%s\": ", p->key);
        emitJsonString(p->value);
    }
    manifestOut("}");
}

static const char* joinTypeToString(enum JoinType type) {
    switch (type) {
        case JOIN_LEFT: return "left";
        case JOIN_RIGHT: return "right";
        case JOIN_FULL: return "full";
        case JOIN_SEMI: return "semi";
        case JOIN_ANTI: return "anti";
        case JOIN_INNER:
        default: return "inner";
    }
}

static const char* joinTypeToPandasHow(enum JoinType type) {
    switch (type) {
        case JOIN_LEFT: return "left";
        case JOIN_RIGHT: return "right";
        case JOIN_FULL: return "outer";
        case JOIN_SEMI: return "inner"; 
        case JOIN_ANTI: return "left";  
        case JOIN_INNER:
        default: return "inner";
    }
}

static const char* udfTypeToString(UdfParamType type) {
    switch (type) {
        case UDF_TYPE_INT: return "int";
        case UDF_TYPE_STRING: return "string";
        case UDF_TYPE_BOOL: return "bool";
        case UDF_TYPE_DECIMAL: return "decimal";
        case UDF_TYPE_UNKNOWN:
        default: return "unknown";
    }
}

static void writeOperations(TransformOperationList *operations) {
    manifestOut("[");
    bool first = true;
    for (TransformOperationList *it = operations; it != NULL; it = it->next) {
        TransformOperation *op = it->operation;
        if (!op) continue;
        if (!first) manifestOut(", ");
        first = false;
        switch (op->type) {
            case FILTER_OP:
                manifestOut("{\"type\":\"filter\"}");
                break;
            case WITH_COLUMN_OP:
                manifestOut("{\"type\":\"withColumn\"}");
                break;
            case SELECT_OP:
                manifestOut("{\"type\":\"select\"}");
                break;
            case ORDER_BY_OP:
                manifestOut("{\"type\":\"orderBy\"}");
                break;
            case LIMIT_OP:
                manifestOut("{\"type\":\"limit\",\"value\":%d}", op->limitOp ? op->limitOp->limit : 0);
                break;
            case JOIN_OP:
                manifestOut("{\"type\":\"join\",\"target\":\"%s\",\"joinType\":\"%s\"}",
                            op->join ? (op->join->target ? op->join->target : "") : "",
                            op->join ? joinTypeToString(op->join->joinType) : "inner");
                break;
            case GROUP_BY_OP:
                manifestOut("{\"type\":\"groupBy\"}");
                break;
            case WINDOW_OP:
                manifestOut("{\"type\":\"window\"}");
                break;
        }
    }
    manifestOut("]");
}

static SchemaInfo schemaFromSelectOp(const SchemaInfo* base, SelectOperation* select) {
    SchemaInfo out = {0};
    size_t idx = 0;
    for (ColumnList* col = select ? select->columns : NULL; col != NULL; col = col->next, idx++) {
        const char* name = col->alias;
        if (!name || strlen(name) == 0) {
            if (col->expression && col->expression->type == FACTOR &&
                col->expression->factor->type == IDENTIFIER_FACTOR) {
                name = col->expression->factor->identifier;
            } else {
                char gen[32];
                snprintf(gen, sizeof(gen), "col_%zu", idx);
                char* heapName = strdup(gen);
                addColumnName(&out, heapName);
                free(heapName);
                continue;
            }
        }
        addColumnName(&out, name);
    }
    if (out.count == 0 && base) {
        out = copySchemaInfo(base);
    }
    return out;
}

static SchemaInfo schemaAfterWithColumnOp(SchemaInfo base, WithColumnOperation* withColumn) {
    for (ColumnAssignmentList* ca = withColumn ? withColumn->assignments : NULL; ca != NULL; ca = ca->next) {
        ColumnAssignment* a = ca->assignment;
        if (!a) continue;
        addColumnName(&base, a->columnName);
    }
    return base;
}

static SchemaInfo schemaAfterJoinOp(const SchemaInfo* left, const SchemaInfo* right) {
    SchemaInfo out = copySchemaInfo(left);
    if (right) {
        for (size_t i = 0; i < right->count; i++) {
            addColumnName(&out, right->columns[i].name);
        }
    }
    return out;
}

static SchemaInfo schemaAfterGroupByOp(const SchemaInfo* input, GroupByOperation* groupBy) {
    SchemaInfo out = {0};
    for (ColumnList* k = groupBy ? groupBy->keys : NULL; k != NULL; k = k->next) {
        if (k->expression && k->expression->type == FACTOR && k->expression->factor->type == IDENTIFIER_FACTOR) {
            addColumnName(&out, k->expression->factor->identifier);
        } else {
            addColumnName(&out, "group_key");
        }
    }
    for (ColumnAssignmentList* a = groupBy ? groupBy->aggregations : NULL; a != NULL; a = a->next) {
        ColumnAssignment* agg = a->assignment;
        if (agg && agg->columnName) addColumnName(&out, agg->columnName);
    }
    if (out.count == 0 && input) {
        out = copySchemaInfo(input);
    }
    return out;
}

static SchemaInfo schemaAfterWindowOp(const SchemaInfo* input, WindowOperation* windowOp) {
    SchemaInfo out = copySchemaInfo(input);
    for (ColumnAssignmentList* ca = windowOp ? windowOp->computations : NULL; ca != NULL; ca = ca->next) {
        ColumnAssignment* a = ca->assignment;
        if (a && a->columnName) addColumnName(&out, a->columnName);
    }
    return out;
}

static void setSchemaEntry(NamedSchema** arrPtr, size_t* capPtr, size_t* countPtr, const char* name, SchemaInfo schema) {
    if (!arrPtr || !capPtr || !countPtr) return;
    if (*capPtr == 0) {
        *capPtr = 8;
        *arrPtr = calloc(*capPtr, sizeof(NamedSchema));
    }
    NamedSchema* arr = *arrPtr;
    size_t cap = *capPtr;
    size_t count = *countPtr;

    SchemaInfo stored = copySchemaInfo(&schema);

    int idx = findSchemaIndex(arr, count, name);
    if (idx >= 0) {
        free(arr[idx].name);
        freeSchemaInfo(&arr[idx].schema);
        arr[idx].name = name ? strdup(name) : NULL;
        arr[idx].schema = stored;
        return;
    }

    if (count == cap) {
        cap *= 2;
        arr = realloc(arr, cap * sizeof(NamedSchema));
    }
    arr[count].name = name ? strdup(name) : NULL;
    arr[count].schema = stored;

    *arrPtr = arr;
    *capPtr = cap;
    *countPtr = count + 1;
}

static SchemaInfo* getSchemaEntry(NamedSchema* arr, size_t count, const char* name) {
    int idx = findSchemaIndex(arr, count, name);
    return (idx >= 0) ? &arr[idx].schema : NULL;
}

static void computeSchemas(NamedSchema** arrayPtr, size_t* countPtr, Program* program) {
    size_t cap = 0;
    size_t count = 0;
    NamedSchema* arr = NULL;

    for (StatementList *it = program->statements; it != NULL; it = it->next) {
        Statement *stmt = it->statement;
        switch (stmt->type) {
            case SOURCE_STMT: {
                SchemaInfo s = (SchemaInfo){0}; 
                setSchemaEntry(&arr, &cap, &count, stmt->sourceDecl->name, s);
                freeSchemaInfo(&s);
                break;
            }
            case DATASET_STMT: {
                SchemaInfo* src = getSchemaEntry(arr, count, stmt->datasetDecl->sourceName);
                SchemaInfo s = src ? copySchemaInfo(src) : (SchemaInfo){0};
                setSchemaEntry(&arr, &cap, &count, stmt->datasetDecl->name, s);
                freeSchemaInfo(&s);
                break;
            }
            case TRANSFORM_STMT: {
                SchemaInfo* base = getSchemaEntry(arr, count, stmt->transformDecl->sourceName);
                SchemaInfo cur = base ? copySchemaInfo(base) : (SchemaInfo){0};
                for (TransformOperationList* opIt = stmt->transformDecl->operations; opIt != NULL; opIt = opIt->next) {
                    TransformOperation* op = opIt->operation;
                    if (!op) continue;
                    switch (op->type) {
                        case WITH_COLUMN_OP:
                            cur = schemaAfterWithColumnOp(cur, op->withColumn);
                            break;
                        case SELECT_OP: {
                            SchemaInfo tmp = schemaFromSelectOp(&cur, op->select);
                            freeSchemaInfo(&cur);
                            cur = tmp;
                            break;
                        }
                        case JOIN_OP: {
                            SchemaInfo* right = getSchemaEntry(arr, count, op->join->target);
                            SchemaInfo tmp = schemaAfterJoinOp(&cur, right);
                            freeSchemaInfo(&cur);
                            cur = tmp;
                            break;
                        }
                        case GROUP_BY_OP: {
                            SchemaInfo tmp = schemaAfterGroupByOp(&cur, op->groupBy);
                            freeSchemaInfo(&cur);
                            cur = tmp;
                            break;
                        }
                        case WINDOW_OP: {
                            SchemaInfo tmp = schemaAfterWindowOp(&cur, op->windowOp);
                            freeSchemaInfo(&cur);
                            cur = tmp;
                            break;
                        }
                        default:
                            break;
                    }
                }
                setSchemaEntry(&arr, &cap, &count, stmt->transformDecl->name, cur);
                freeSchemaInfo(&cur);
                break;
            }
            default:
                break;
        }
    }

    *arrayPtr = arr;
    *countPtr = count;
}

static void freeSchemas(NamedSchema* arr, size_t count) {
    if (!arr) return;
    for (size_t i = 0; i < count; i++) {
        free(arr[i].name);
        freeSchemaInfo(&arr[i].schema);
    }
    free(arr);
}

/* ============ Emisión de un runner en Python (pandas) ============ */

static void emitPythonHeader() {
    pyOut("# Auto-generated. Experimental executor for Dataly DSL using pandas.\n");
    pyOut("import pandas as pd\n");
    pyOut("import json\n");
    pyOut("import sys\n");
    pyOut("import os\n\n");
    pyOut("def _udf(name, *args):\n");
    pyOut("    raise NotImplementedError(f\"UDF {name} not implemented\")\n\n");
    pyOut("def main():\n");
    pyOut("    params = {}\n");
    pyOut("    datasets = {}\n");
}

static void emitPythonFooter() {
    pyOut("\nif __name__ == \"__main__\":\n");
    pyOut("    try:\n");
    pyOut("        main()\n");
    pyOut("    except NotImplementedError as e:\n");
    pyOut("        print(f\"[WARN] {e}\")\n");
    pyOut("        sys.exit(1)\n");
}

static const char* boolToPy(bool value) { return value ? "True" : "False"; }

static void expressionToPython(Expression* expr, char* buffer, size_t bufferSize, const char* dfVar) {
    if (!expr) { snprintf(buffer, bufferSize, "None"); return; }
    switch (expr->type) {
	case FACTOR:
		if (expr->factor->type == CONSTANT) {
			Constant* c = expr->factor->constant;
			switch (c->type) {
				case INTEGER_CONST: snprintf(buffer, bufferSize, "%d", c->intValue); break;
				case STRING_CONST: {
					char tmp[512];
					sanitizeStringLiteral(c->stringValue ? c->stringValue : "", tmp, sizeof(tmp));
					size_t len = strnlen(tmp, sizeof(tmp));
					if (len + 3 > bufferSize) {
						len = bufferSize > 3 ? bufferSize - 3 : 0;
					}
					if (bufferSize > 0) {
						buffer[0] = '"';
						if (len > 0) {
							memcpy(buffer + 1, tmp, len);
						}
						if (bufferSize > len + 1) {
							buffer[len + 1] = '"';
							if (bufferSize > len + 2) buffer[len + 2] = '\0';
						}
					}
					break;
				}
				case BOOLEAN_CONST: snprintf(buffer, bufferSize, "%s", boolToPy(c->boolValue)); break;
				case NULL_CONST: snprintf(buffer, bufferSize, "None"); break;
			}
		} else if (expr->factor->type == IDENTIFIER_FACTOR) {
			snprintf(buffer, bufferSize, "%s[\"%s\"]", dfVar, expr->factor->identifier);
            } else if (expr->factor->type == EXPRESSION) {
                expressionToPython(expr->factor->expression, buffer, bufferSize, dfVar);
            }
            break;
        case ADDITION:
        case SUBTRACTION:
        case MULTIPLICATION:
        case DIVISION:
        case MODULO:
        case COMPARISON:
        case LOGICAL: {
            char left[512]; char right[512];
            expressionToPython(expr->leftExpression, left, sizeof(left), dfVar);
            expressionToPython(expr->rightExpression, right, sizeof(right), dfVar);
            const char* op = "";
            if (expr->type == ADDITION) op = "+";
            else if (expr->type == SUBTRACTION) op = "-";
            else if (expr->type == MULTIPLICATION) op = "*";
            else if (expr->type == DIVISION) op = "/";
            else if (expr->type == MODULO) op = "%%";
            else if (expr->type == COMPARISON) {
                switch (expr->compOp) {
                    case EQUALS_OP: op = "=="; break;
                    case GREATER_THAN: op = ">"; break;
                    case LESS_THAN: op = "<"; break;
                    case NOT_EQUALS_OP: op = "!="; break;
                    case GREATER_EQUAL_OP: op = ">="; break;
                    case LESS_EQUAL_OP: op = "<="; break;
                    case IS_NULL_OP: op = "is None"; break;
                    case IS_NOT_NULL_OP: op = "is not None"; break;
                }
            } else if (expr->type == LOGICAL) {
                switch (expr->logOp) {
                    case AND_OP: op = "&"; break;
                    case OR_OP: op = "|"; break;
                    case NOT_OP: {
                        int nNot = snprintf(buffer, bufferSize, "~(%s)", left);
                        if (nNot < 0 || (size_t)nNot >= bufferSize) {
                            buffer[bufferSize - 1] = '\0';
                        }
                        return;
                    }
                }
            }
            int n = snprintf(buffer, bufferSize, "(%s %s %s)", left, op, right);
            if (n < 0 || (size_t)n >= bufferSize) {
                buffer[bufferSize - 1] = '\0';
            }
            break;
        }
        case IDENTIFIER_EXPR:
            snprintf(buffer, bufferSize, "%s[\"%s\"]", dfVar, expr->identifier);
            break;
        case FUNCTION_CALL: {
            if (!expr->functionCall.functionName) { snprintf(buffer, bufferSize, "None"); break; }
            const char* fname = expr->functionCall.functionName;
            ExpressionList* args = expr->functionCall.arguments;
            if (strcmp(fname, "length") == 0 && args) {
                char arg[256]; expressionToPython(args->expression, arg, sizeof(arg), dfVar);
                snprintf(buffer, bufferSize, "%s.astype(str).str.len()", arg);
            } else if (strcmp(fname, "substring") == 0 && args && args->next && args->next->next) {
                char arg[256]; char startBuf[64]; char lenBuf[64];
                expressionToPython(args->expression, arg, sizeof(arg), dfVar);
                expressionToPython(args->next->expression, startBuf, sizeof(startBuf), dfVar);
                expressionToPython(args->next->next->expression, lenBuf, sizeof(lenBuf), dfVar);
                snprintf(buffer, bufferSize, "%s.astype(str).str.slice(%s, %s + %s)", arg, startBuf, startBuf, lenBuf);
            } else if (strcmp(fname, "concat") == 0 && args && args->next) {
                char a[256]; char b[256];
                expressionToPython(args->expression, a, sizeof(a), dfVar);
                expressionToPython(args->next->expression, b, sizeof(b), dfVar);
                int n = snprintf(buffer, bufferSize, "(%s.astype(str) + %s.astype(str))", a, b);
                if (n < 0 || (size_t)n >= bufferSize) buffer[bufferSize - 1] = '\0';
            } else if (strcmp(fname, "year") == 0 && args) {
                char a[256]; expressionToPython(args->expression, a, sizeof(a), dfVar);
                snprintf(buffer, bufferSize, "pd.to_datetime(%s).dt.year", a);
            } else if (strcmp(fname, "month") == 0 && args) {
                char a[256]; expressionToPython(args->expression, a, sizeof(a), dfVar);
                snprintf(buffer, bufferSize, "pd.to_datetime(%s).dt.month", a);
            } else if (strcmp(fname, "day") == 0 && args) {
                char a[256]; expressionToPython(args->expression, a, sizeof(a), dfVar);
                snprintf(buffer, bufferSize, "pd.to_datetime(%s).dt.day", a);
            } else if (strcmp(fname, "date_diff") == 0 && args && args->next) {
                char a[256]; char bBuf[256];
                expressionToPython(args->expression, a, sizeof(a), dfVar);
                expressionToPython(args->next->expression, bBuf, sizeof(bBuf), dfVar);
                int n = snprintf(buffer, bufferSize, "(pd.to_datetime(%s) - pd.to_datetime(%s)).dt.days", a, bBuf);
                if (n < 0 || (size_t)n >= bufferSize) buffer[bufferSize - 1] = '\0';
            } else if (strcmp(fname, "regex_match") == 0 && args && args->next) {
                char a[256]; char pattern[256];
                expressionToPython(args->expression, a, sizeof(a), dfVar);
                expressionToPython(args->next->expression, pattern, sizeof(pattern), dfVar);
                int n = snprintf(buffer, bufferSize, "%s.astype(str).str.contains(%s)", a, pattern);
                if (n < 0 || (size_t)n >= bufferSize) buffer[bufferSize - 1] = '\0';
            } else if (strcmp(fname, "coalesce") == 0 && args && args->next) {
                char a[256]; char bBuf[256];
                expressionToPython(args->expression, a, sizeof(a), dfVar);
                expressionToPython(args->next->expression, bBuf, sizeof(bBuf), dfVar);
                int n = snprintf(buffer, bufferSize, "%s.fillna(%s)", a, bBuf);
                if (n < 0 || (size_t)n >= bufferSize) buffer[bufferSize - 1] = '\0';
            } else {
                snprintf(buffer, bufferSize, "_udf(\"%s\")", fname);
            }
            break;
        }
    }
}

static void emitSourcePython(SourceDeclaration* source) {
    if (!source || !source->name) return;
    char fmtBuf[64]; fmtBuf[0] = '\0';
    char pathBuf[1024]; pathBuf[0] = '\0';
    for (PropertyList* pl = source->properties; pl != NULL; pl = pl->next) {
        Property* p = pl->property;
        if (!p) continue;
        if (strcmp(p->key, "format") == 0 || strcmp(p->key, "type") == 0) {
            sanitizeStringLiteral(p->value ? p->value : "", fmtBuf, sizeof(fmtBuf));
        }
        if (strcmp(p->key, "path") == 0) {
            sanitizeStringLiteral(p->value ? p->value : "", pathBuf, sizeof(pathBuf));
        }
    }
    pyOut("    # source %s\n", source->name);
    if (pathBuf[0] != '\0') {
        const char* fmt = (fmtBuf[0] != '\0') ? fmtBuf : "csv";
        if (strcmp(fmt, "csv") == 0) {
            pyOut("    datasets[\"%s\"] = pd.read_csv(", source->name);
            emitPyStringLiteral(pathBuf);
            pyOut(".format(**params))\n");
        } else if (strcmp(fmt, "parquet") == 0) {
            pyOut("    datasets[\"%s\"] = pd.read_parquet(", source->name);
            emitPyStringLiteral(pathBuf);
            pyOut(".format(**params))\n");
        } else {
            pyOut("    datasets[\"%s\"] = pd.DataFrame()\n", source->name);
        }
    } else {
        pyOut("    datasets[\"%s\"] = pd.DataFrame()\n", source->name);
    }
}

static void emitTransformPython(TransformDeclaration* transform) {
    if (!transform || !transform->name || !transform->sourceName) return;
    pyOut("    # transform %s\n", transform->name);
    pyOut("    df = datasets.get(\"%s\").copy()\n", transform->sourceName);

    for (TransformOperationList* it = transform->operations; it != NULL; it = it->next) {
        TransformOperation* op = it->operation;
        if (!op) continue;
        switch (op->type) {
            case FILTER_OP: {
                char exprBuf[512]; expressionToPython(op->filter->condition, exprBuf, sizeof(exprBuf), "df");
                pyOut("    df = df.loc[lambda df: %s]\n", exprBuf);
                break;
            }
            case WITH_COLUMN_OP: {
                for (ColumnAssignmentList* ca = op->withColumn->assignments; ca != NULL; ca = ca->next) {
                    ColumnAssignment* a = ca->assignment;
                    if (!a) continue;
                    char exprBuf[512]; expressionToPython(a->expression, exprBuf, sizeof(exprBuf), "df");
                    pyOut("    df[\"%s\"] = %s\n", a->columnName, exprBuf);
                }
                break;
            }
            case SELECT_OP: {
                pyOut("    # select\n");
                pyOut("    _assign_map = {}\n");
                pyOut("    _select_cols = []\n");
                int idx = 0;
                for (ColumnList* col = op->select->columns; col != NULL; col = col->next, idx++) {
                    char exprBuf[512]; expressionToPython(col->expression, exprBuf, sizeof(exprBuf), "df");
                    const char* outName = col->alias;
                    if (!outName || strlen(outName) == 0) {
                        if (col->expression && col->expression->type == FACTOR &&
                            col->expression->factor->type == IDENTIFIER_FACTOR) {
                            outName = col->expression->factor->identifier;
                        } else {
                            static char tmpName[32];
                            snprintf(tmpName, sizeof(tmpName), "col_%d", idx);
                            outName = tmpName;
                        }
                    }
                    pyOut("    _assign_map[\"%s\"] = %s\n", outName, exprBuf);
                    pyOut("    _select_cols.append(\"%s\")\n", outName);
                }
                pyOut("    df = df.assign(**_assign_map)[_select_cols]\n");
                break;
            }
            case ORDER_BY_OP: {
                pyOut("    df = df.sort_values(by=[");
                bool first = true;
                for (OrderByItem* item = op->orderBy->items; item != NULL; item = item->next) {
                    if (!first) pyOut(", ");
                    first = false;
                    if (item->expression && item->expression->type == FACTOR &&
                        item->expression->factor->type == IDENTIFIER_FACTOR) {
                        pyOut("\"%s\"", item->expression->factor->identifier);
                    } else {
                        pyOut("\"_expr_%p\"", (void*)item->expression);
                    }
                }
                pyOut("], ascending=[");
                first = true;
                for (OrderByItem* item = op->orderBy->items; item != NULL; item = item->next) {
                    if (!first) pyOut(", ");
                    first = false;
                    pyOut("%s", item->ascending ? "True" : "False");
                }
                pyOut("])\n");
                break;
            }
            case LIMIT_OP:
                pyOut("    df = df.head(%d)\n", op->limitOp ? op->limitOp->limit : 0);
                break;
            case JOIN_OP: {
                pyOut("    _right = datasets.get(\"%s\")\n", op->join->target ? op->join->target : "");
                pyOut("    if _right is None:\n");
                pyOut("        raise NotImplementedError(\"JOIN target missing: %s\")\n", op->join->target ? op->join->target : "");
                const char* leftKey = NULL;
                const char* rightKey = NULL;
                if (op->join->condition && op->join->condition->type == COMPARISON &&
                    op->join->condition->compOp == EQUALS_OP) {
                    Expression* leftExpr = op->join->condition->leftExpression;
                    Expression* rightExpr = op->join->condition->rightExpression;
                    if (leftExpr && leftExpr->type == FACTOR && leftExpr->factor->type == IDENTIFIER_FACTOR) {
                        leftKey = leftExpr->factor->identifier;
                    }
                    if (rightExpr && rightExpr->type == FACTOR && rightExpr->factor->type == IDENTIFIER_FACTOR) {
                        rightKey = rightExpr->factor->identifier;
                    }
                }
                if (leftKey && rightKey) {
                    pyOut("    df = df.merge(_right, left_on=\"%s\", right_on=\"%s\", how=\"%s\", indicator=True)\n",
                          leftKey, rightKey, joinTypeToPandasHow(op->join->joinType));
                    if (op->join->joinType == JOIN_SEMI) {
                        pyOut("    df = df[df['_merge'] != 'right_only'].drop(columns=['_merge'])\n");
                    } else if (op->join->joinType == JOIN_ANTI) {
                        pyOut("    df = df[df['_merge'] == 'left_only'].drop(columns=['_merge'])\n");
                    } else {
                        pyOut("    df = df.drop(columns=['_merge'])\n");
                    }
                } else {
                    pyOut("    raise NotImplementedError(\"JOIN condition must be simple equality between columns\")\n");
                }
                break;
            }
            case GROUP_BY_OP: {
                pyOut("    # groupBy\n");
                pyOut("    _gb_keys = []\n");
                pyOut("    _agg_map = {}\n");
                for (ColumnList* key = op->groupBy->keys; key != NULL; key = key->next) {
                    if (key->expression && key->expression->type == FACTOR &&
                        key->expression->factor->type == IDENTIFIER_FACTOR) {
                        pyOut("    _gb_keys.append(\"%s\")\n", key->expression->factor->identifier);
                    } else {
                        pyOut("    raise NotImplementedError(\"GROUP BY keys must be identifiers\")\n");
                    }
                }
                for (ColumnAssignmentList* ag = op->groupBy->aggregations; ag != NULL; ag = ag->next) {
                    ColumnAssignment* a = ag->assignment;
                    if (!a) continue;
                    if (a->expression && a->expression->type == FUNCTION_CALL &&
                        a->expression->functionCall.functionName) {
                        const char* fname = a->expression->functionCall.functionName;
                        /* Soportar sum, avg, min, max, count */
                        if (strcmp(fname, "sum") == 0 || strcmp(fname, "avg") == 0 ||
                            strcmp(fname, "min") == 0 || strcmp(fname, "max") == 0 || strcmp(fname, "count") == 0) {
                            /* Usar primer argumento como columna */
                            ExpressionList* args = a->expression->functionCall.arguments;
                            if (args && args->expression && args->expression->type == FACTOR &&
                                args->expression->factor->type == IDENTIFIER_FACTOR) {
                                const char* colName = args->expression->factor->identifier;
                                const char* aggFun = fname;
                                if (strcmp(fname, "avg") == 0) aggFun = "mean";
                                if (strcmp(fname, "count") == 0) {
                                    pyOut("    _agg_map[\"%s\"] = (\"%s\", \"count\")\n", a->columnName, colName);
                                } else {
                                    pyOut("    _agg_map[\"%s\"] = (\"%s\", \"%s\")\n", a->columnName, colName, aggFun);
                                }
                            } else {
                                pyOut("    raise NotImplementedError(\"GROUP BY aggregation arguments must be identifiers\")\n");
                            }
                        } else {
                            pyOut("    raise NotImplementedError(\"Unsupported aggregation function %s\")\n", fname);
                        }
                    } else {
                        pyOut("    raise NotImplementedError(\"Aggregations must be function calls\")\n");
                    }
                }
                pyOut("    df = df.groupby(_gb_keys).agg(**_agg_map).reset_index()\n");
                break;
            }
            case WINDOW_OP: {
                pyOut("    # window\n");
                pyOut("    _window_df = df\n");
                pyOut("    _partition_cols = []\n");
                for (ColumnList* p = op->windowOp->partitionBy; p != NULL; p = p->next) {
                    if (p->expression && p->expression->type == FACTOR &&
                        p->expression->factor->type == IDENTIFIER_FACTOR) {
                        pyOut("    _partition_cols.append(\"%s\")\n", p->expression->factor->identifier);
                    }
                }
                /* orderBy */
                pyOut("    _order_cols = []\n");
                pyOut("    _order_asc = []\n");
                for (OrderByItem* item = op->windowOp->orderBy; item != NULL; item = item->next) {
                    if (item->expression && item->expression->type == FACTOR &&
                        item->expression->factor->type == IDENTIFIER_FACTOR) {
                        pyOut("    _order_cols.append(\"%s\")\n", item->expression->factor->identifier);
                        pyOut("    _order_asc.append(%s)\n", item->ascending ? "True" : "False");
                    }
                }
                pyOut("    if _order_cols:\n");
                pyOut("        _window_df = _window_df.sort_values(by=_partition_cols + _order_cols, ascending=_order_asc if _order_cols else True)\n");
                pyOut("    _assign_map = {}\n");
                for (ColumnAssignmentList* ca = op->windowOp->computations; ca != NULL; ca = ca->next) {
                    ColumnAssignment* a = ca->assignment;
                    if (!a) continue;
                    if (a->expression && a->expression->type == FUNCTION_CALL && a->expression->functionCall.functionName) {
                        const char* fname = a->expression->functionCall.functionName;
                        if (strcmp(fname, "row_number") == 0) {
                            pyOut("    _assign_map[\"%s\"] = _window_df.groupby(_partition_cols).cumcount() + 1\n", a->columnName);
                        } else if ((strcmp(fname, "rank") == 0 || strcmp(fname, "dense_rank") == 0)) {
                            pyOut("    if not _order_cols:\n");
                            pyOut("        raise NotImplementedError(\"WINDOW rank functions require orderBy\")\n");
                            pyOut("    _assign_map[\"%s\"] = _window_df.groupby(_partition_cols)[_order_cols[0]].rank(method=\"%s\")\n",
                                  a->columnName, strcmp(fname, "dense_rank") == 0 ? "dense" : "average");
                        } else if (strcmp(fname, "lead") == 0 || strcmp(fname, "lag") == 0) {
                            pyOut("    if not _order_cols:\n");
                            pyOut("        raise NotImplementedError(\"WINDOW lead/lag require orderBy\")\n");
                            pyOut("    _shift = 1\n");
                            pyOut("    _col_ref = None\n");
                            if (a->expression->functionCall.arguments && a->expression->functionCall.arguments->expression &&
                                a->expression->functionCall.arguments->expression->type == FACTOR &&
                                a->expression->functionCall.arguments->expression->factor->type == IDENTIFIER_FACTOR) {
                                pyOut("    _col_ref = \"%s\"\n", a->expression->functionCall.arguments->expression->factor->identifier);
                            } else {
                                pyOut("    raise NotImplementedError(\"lead/lag require column identifier as first argument\")\n");
                            }
                            if (a->expression->functionCall.arguments && a->expression->functionCall.arguments->next) {
                                pyOut("    try:\n");
                                pyOut("        _shift = int(");
                                char offBuf[256];
                                expressionToPython(a->expression->functionCall.arguments->next->expression, offBuf, sizeof(offBuf), "_window_df");
                                pyOut("%s)\n", offBuf);
                                pyOut("    except Exception:\n");
                                pyOut("        _shift = 1\n");
                            }
                            pyOut("    if _col_ref is not None:\n");
                            pyOut("        _assign_map[\"%s\"] = _window_df.groupby(_partition_cols)[_col_ref].shift(%s)\n",
                                  a->columnName, strcmp(fname, "lead") == 0 ? "-_shift" : "_shift");
                        } else {
                            pyOut("    raise NotImplementedError(\"WINDOW function %s not supported\")\n", fname);
                        }
                    } else {
                        char exprBuf[512]; expressionToPython(a->expression, exprBuf, sizeof(exprBuf), "_window_df");
                        pyOut("    _assign_map[\"%s\"] = %s\n", a->columnName, exprBuf);
                    }
                }
                pyOut("    if _assign_map:\n");
                pyOut("        _window_df = _window_df.assign(**_assign_map)\n");
                pyOut("    df = _window_df\n");
                break;
            }
        }
    }

    pyOut("    datasets[\"%s\"] = df\n", transform->name);
}

static void emitSinkPython(SinkDeclaration* sink) {
    if (!sink || !sink->name) return;
    const char* fmt = NULL;
    const char* path = NULL;
    const char* mode = "overwrite";
    for (PropertyList* pl = sink->properties; pl != NULL; pl = pl->next) {
        Property* p = pl->property;
        if (!p) continue;
        if (strcmp(p->key, "format") == 0) fmt = p->value;
        if (strcmp(p->key, "path") == 0) path = p->value;
        if (strcmp(p->key, "mode") == 0) mode = p->value;
    }
    pyOut("    # sink %s\n", sink->name);
    pyOut("    _sink_%s = {\"format\": ", sink->name);
    emitPyStringLiteral(fmt ? fmt : "");
    pyOut(", \"path\": ");
    emitPyStringLiteral(path ? path : "");
    pyOut(".format(**params), \"mode\": ");
    emitPyStringLiteral(mode ? mode : "");
    pyOut("}\n");
}

static void emitWritePython(WriteStatement* write) {
    if (!write) return;
    pyOut("    # write %s into %s\n", write->datasetName, write->sinkName);
    pyOut("    df = datasets.get(\"%s\")\n", write->datasetName);
    pyOut("    sink = locals().get(f\"_sink_%s\")\n", write->sinkName);
    pyOut("    if df is None or sink is None:\n");
    pyOut("        print(f\"[WARN] Missing dataset or sink for write %s -> %s\")\n", write->datasetName, write->sinkName);
    pyOut("    else:\n");
    pyOut("        if sink[\"format\"] == \"csv\":\n");
    pyOut("            df.to_csv(sink[\"path\"], index=False)\n");
    pyOut("        elif sink[\"format\"] == \"parquet\":\n");
    pyOut("            df.to_parquet(sink[\"path\"], index=False)\n");
    pyOut("        else:\n");
    pyOut("            print(f\"[WARN] Unsupported sink format {sink['format']}\")\n");
}

static void generateDot(Program *program) {
    dotOut("digraph pipeline {\n");

    for (StatementList *it = program->statements; it != NULL; it = it->next) {
        Statement *stmt = it->statement;

        switch (stmt->type) {
            case SOURCE_STMT:
                dotOut("    \"%s\" [shape=box, style=filled, fillcolor=\"lightblue\"];\n",
                       stmt->sourceDecl->name);
                break;
            case DATASET_STMT:
                dotOut("    \"%s\" [shape=ellipse, style=filled, fillcolor=\"lightgreen\"];\n",
                       stmt->datasetDecl->name);
                dotOut("    \"%s\" -> \"%s\";\n",
                       stmt->datasetDecl->sourceName,
                       stmt->datasetDecl->name);
                break;
            case TRANSFORM_STMT:
                dotOut("    \"%s\" [shape=diamond, style=filled, fillcolor=\"yellow\"];\n",
                       stmt->transformDecl->name);
                dotOut("    \"%s\" -> \"%s\";\n",
                       stmt->transformDecl->sourceName,
                       stmt->transformDecl->name);
                for (TransformOperationList *opIt = stmt->transformDecl->operations; opIt != NULL; opIt = opIt->next) {
                    TransformOperation *op = opIt->operation;
                    if (op != NULL && op->type == JOIN_OP && op->join != NULL && op->join->target != NULL) {
                        dotOut("    \"%s\" -> \"%s\";\n",
                               op->join->target,
                               stmt->transformDecl->name);
                    }
                }
                break;
            case SINK_STMT:
                dotOut("    \"%s\" [shape=box, style=filled, fillcolor=\"orange\"];\n",
                       stmt->sinkDecl->name);
                break;
            case WRITE_STMT:
                dotOut("    \"%s\" -> \"%s\";\n",
                       stmt->writeStmt->datasetName,
                       stmt->writeStmt->sinkName);
                break;
            case PARAM_STMT:
                break;
        }
    }

    dotOut("}\n");
}

static void generateManifest(Program *program) {
    manifestOut("{\n");
    manifestOut("  \"parameters\": [\n");
    bool first = true;

    NamedSchema* schemas = NULL;
    size_t schemaCount = 0;
    computeSchemas(&schemas, &schemaCount, program);

    // ---------- parameters ----------
    for (StatementList *it = program->statements; it != NULL; it = it->next) {
        Statement *stmt = it->statement;
        if (stmt->type == PARAM_STMT) {
            if (!first) manifestOut(",\n");
            first = false;
            manifestOut("    { \"name\": \"%s\", \"value\": ", stmt->paramDecl->name);
            emitJsonString(stmt->paramDecl->value);
            manifestOut(" }");
        }
    }
    manifestOut("\n  ],\n");

    // ---------- nodes ----------
    manifestOut("  \"nodes\": [\n");
    first = true;
    for (StatementList *it = program->statements; it != NULL; it = it->next) {
        Statement *stmt = it->statement;
        const char *kind = NULL;
        const char *name = NULL;
        const char *sourceName = NULL;
        PropertyList *properties = NULL;

        switch (stmt->type) {
            case SOURCE_STMT:
                kind = "source"; name = stmt->sourceDecl->name; break;
            case DATASET_STMT:
                kind = "dataset"; name = stmt->datasetDecl->name; sourceName = stmt->datasetDecl->sourceName; break;
            case TRANSFORM_STMT:
                kind = "transform"; name = stmt->transformDecl->name; sourceName = stmt->transformDecl->sourceName; break;
            case SINK_STMT:
                kind = "sink"; name = stmt->sinkDecl->name; break;
            case UDF_STMT:
                kind = "udf"; name = stmt->udfDecl->name; break;
            default:
                break;
        }

        if (kind != NULL) {
            if (!first) manifestOut(",\n");
            first = false;
            manifestOut("    { \"name\": \"%s\", \"kind\": \"%s\"", name, kind);
            if (sourceName != NULL) {
                manifestOut(", \"from\": \"%s\"", sourceName);
            }
            if (stmt->type == SOURCE_STMT) {
                properties = stmt->sourceDecl->properties;
            } else if (stmt->type == SINK_STMT) {
                properties = stmt->sinkDecl->properties;
            }
            if (properties != NULL) {
                manifestOut(", \"properties\": ");
                writeProperties(properties);
            }
            if (stmt->type == UDF_STMT) {
                manifestOut(", \"params\": [");
                bool pf = true;
                for (UdfParamList* p = stmt->udfDecl->params; p != NULL; p = p->next) {
                    if (!pf) manifestOut(", ");
                    pf = false;
                    manifestOut("{\"name\": \"%s\", \"type\": \"%s\"}", p->param->name, udfTypeToString(p->param->type));
                }
                manifestOut("], \"returns\": \"%s\"", udfTypeToString(stmt->udfDecl->returnType));
            }
            /* schema */
            for (size_t i = 0; i < schemaCount; i++) {
                if (schemas[i].name && strcmp(schemas[i].name, name) == 0) {
                    manifestOut(", \"schema\": [");
                    bool sf = true;
                    for (size_t c = 0; c < schemas[i].schema.count; c++) {
                        if (!sf) manifestOut(", ");
                        sf = false;
                        manifestOut("{\"name\": \"%s\"}", schemas[i].schema.columns[c].name ? schemas[i].schema.columns[c].name : "");
                    }
                    manifestOut("]");
                    break;
                }
            }
            if (stmt->type == TRANSFORM_STMT) {
                manifestOut(", \"operations\": ");
                writeOperations(stmt->transformDecl->operations);
            }
            manifestOut(" }");
        }
    }
    manifestOut("\n  ],\n");

    // ---------- edges ----------
    manifestOut("  \"edges\": [\n");
    first = true;
    for (StatementList *it = program->statements; it != NULL; it = it->next) {
        Statement *stmt = it->statement;

        const char *from = NULL;
        const char *to   = NULL;

        switch (stmt->type) {
            case DATASET_STMT:
                from = stmt->datasetDecl->sourceName;
                to   = stmt->datasetDecl->name;
                break;
            case TRANSFORM_STMT:
                from = stmt->transformDecl->sourceName;
                to   = stmt->transformDecl->name;
                break;
            case WRITE_STMT:
                from = stmt->writeStmt->datasetName;
                to   = stmt->writeStmt->sinkName;
                break;
            default:
                break;
        }

        if (from && to) {
            if (!first) manifestOut(",\n");
            first = false;
            manifestOut("    { \"from\": \"%s\", \"to\": \"%s\" }", from, to);
        }

        if (stmt->type == TRANSFORM_STMT) {
            for (TransformOperationList *opIt = stmt->transformDecl->operations; opIt != NULL; opIt = opIt->next) {
                TransformOperation *op = opIt->operation;
                if (op != NULL && op->type == JOIN_OP && op->join != NULL && op->join->target != NULL) {
                    if (!first) manifestOut(",\n");
                    first = false;
                    manifestOut("    { \"from\": \"%s\", \"to\": \"%s\" }",
                                op->join->target,
                                stmt->transformDecl->name);
                }
            }
        }
    }
    manifestOut("\n  ]\n");
    manifestOut("}\n");

    freeSchemas(schemas, schemaCount);
}

static void generatePython(Program *program) {
    emitPythonHeader();

    for (StatementList *it = program->statements; it != NULL; it = it->next) {
        Statement *stmt = it->statement;
        switch (stmt->type) {
            case PARAM_STMT:
                char envName[128];
                toUpperSnake(stmt->paramDecl->name, envName, sizeof(envName));
                pyOut("    params[\"%s\"] = os.getenv(\"%s\", ", stmt->paramDecl->name, envName);
                emitPyStringLiteral(stmt->paramDecl->value);
                pyOut(")\n");
                break;
            case SOURCE_STMT:
                emitSourcePython(stmt->sourceDecl);
                break;
            case DATASET_STMT:
                pyOut("    datasets[\"%s\"] = datasets.get(\"%s\")\n", stmt->datasetDecl->name, stmt->datasetDecl->sourceName);
                break;
            case TRANSFORM_STMT:
                emitTransformPython(stmt->transformDecl);
                break;
            case SINK_STMT:
                emitSinkPython(stmt->sinkDecl);
                break;
            case WRITE_STMT:
                emitWritePython(stmt->writeStmt);
                break;
        }
    }

    emitPythonFooter();
}
/* ============ Orquestador público ============ */

CompilationStatus executeGenerator(CompilerState *compilerState) {
    Program *program = compilerState->abstractSyntaxtTree;
    if (!program) {
        logError(_logger, "Cannot generate code: AST is NULL.");
        return FAILED;
    }

    if (program->type != STATEMENT_LIST_PROGRAM) {
        if (program->type == EXPRESSION_PROGRAM) {
            logDebugging(_logger, "Expression-only program: nothing to generate for Dataly backend.");
            return SUCCEEDED;
        }
        logError(_logger, "Unsupported program type for Dataly backend.");
        return FAILED;
    }

    openOutputs();
    if (!_dotFile || !_manifestFile) {
        closeOutputs();
        return FAILED;
    }

    logDebugging(_logger, "Generating DOT graph...");
    generateDot(program);

    logDebugging(_logger, "Generating manifest.json...");
    generateManifest(program);

    logDebugging(_logger, "Generating run_pipeline.py (experimental executor)...");
    generatePython(program);

    closeOutputs();
    logInformation(_logger, "Code generation completed.");
    return SUCCEEDED;
}
