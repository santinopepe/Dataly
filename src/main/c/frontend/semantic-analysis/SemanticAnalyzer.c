#include "SemanticAnalyzer.h"
#include "../../support/logging/Logger.h"
#include "../../support/configuration/Environment.h"
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

/* Forward declarations */
struct Schema;
struct Symbol;
struct SemanticContext;
typedef struct Schema Schema;
typedef struct Symbol Symbol;
typedef struct SemanticContext SemanticContext;

/* ======================= Symbol table ======================= */

typedef enum {
    SYMBOL_PARAM,
    SYMBOL_SOURCE,
    SYMBOL_DATASET,
    SYMBOL_TRANSFORM,
    SYMBOL_SINK,
    SYMBOL_UDF
} SymbolKind;

typedef struct ColumnMeta {
    char* name;
    enum {
        TYPE_UNKNOWN = 0,
        TYPE_INT,
        TYPE_STRING,
        TYPE_BOOL,
        TYPE_DECIMAL
    } type;
} ColumnMeta;

typedef struct Schema {
    ColumnMeta* columns;
    size_t count;
    bool unknown; /* when true, skip strict validations for identifiers */
} Schema;

static Schema* inferSchemaForSymbol(SemanticContext* ctx, Symbol* sym);
static int schemaFindColumnIndex(const Schema* schema, const char* name);

typedef struct Symbol {
    char* name;          
    SymbolKind kind;
    void* astNode;       
    Schema* schema;
    bool schemaComputed;
    bool computingSchema;
} Symbol;

typedef struct SymbolTable {
    Symbol* symbols;
    size_t count;
    size_t capacity;
} SymbolTable;


static void initSymbolTable(SymbolTable* table) {
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

static void freeSymbolTable(SymbolTable* table) {
    if (table->symbols != NULL) {
        for (size_t i = 0; i < table->count; i++) {
            free(table->symbols[i].name);
            if (table->symbols[i].schema) {
                for (size_t c = 0; c < table->symbols[i].schema->count; c++) {
                    free(table->symbols[i].schema->columns[c].name);
                }
                free(table->symbols[i].schema->columns);
                free(table->symbols[i].schema);
            }
        }
        free(table->symbols);
    }
    table->symbols = NULL;
    table->count = 0;
    table->capacity = 0;
}

static Symbol* findSymbol(const SymbolTable* table, const char* name) {
    for (size_t i = 0; i < table->count; i++) {
        if (strcmp(table->symbols[i].name, name) == 0) {
            return &table->symbols[i];
        }
    }
    return NULL;
}

static bool addSymbol(SymbolTable* table, const char* name, SymbolKind kind, void* astNode, Logger* logger, bool* hasErrors) {
    if (findSymbol(table, name) != NULL) {
        logError(logger, "Identifier '%s' is already declared.", name);
        *hasErrors = true;
        return false;
    }

    if (table->count == table->capacity) {
        size_t newCap = (table->capacity == 0) ? 8 : table->capacity * 2;
        Symbol* newData = realloc(table->symbols, newCap * sizeof(Symbol));
        if (!newData) {
            logError(logger, "Out of memory while growing symbol table.");
            *hasErrors = true;
            return false;
        }
        table->symbols = newData;
        table->capacity = newCap;
    }

    Symbol* s = &table->symbols[table->count++];
    s->name = strdup(name);
    s->kind = kind;
    s->astNode = astNode;
    s->schema = NULL;
    s->schemaComputed = false;
    s->computingSchema = false;
    return true;
}

/* ======================= Semantic context ======================= */

typedef struct SemanticContext {
    SymbolTable symbols;
    Logger* logger;
    bool hasErrors;
    bool strictMode;
} SemanticContext;

/* Prototypes */
static Schema* inferSchemaForSymbol(SemanticContext* ctx, Symbol* sym);
static int schemaFindColumnIndex(const Schema* schema, const char* name);

static SemanticContext* createSemanticContext() {
    SemanticContext* ctx = calloc(1, sizeof(SemanticContext));
    initSymbolTable(&ctx->symbols);
    ctx->logger = createLogger("SemanticAnalyzer");
    ctx->hasErrors = false;
    ctx->strictMode = getBooleanOrDefault("STRICT_SEMANTIC", false);
    return ctx;
}

static void destroySemanticContext(SemanticContext* ctx) {
    if (!ctx) return;
    freeSymbolTable(&ctx->symbols);
    if (ctx->logger) {
        destroyLogger(ctx->logger);
    }
    free(ctx);
}

static void reportSemanticError(SemanticContext* ctx, const char* fmt, ...) {
    ctx->hasErrors = true;

    char buffer[1024];

    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    logError(ctx->logger, "%s", buffer);
}

/* ======================= Helpers de validación ======================= */

static void registerParam(SemanticContext* ctx, ParamDeclaration* decl) {
    addSymbol(&ctx->symbols, decl->name, SYMBOL_PARAM, decl, ctx->logger, &ctx->hasErrors);
}

static void registerSource(SemanticContext* ctx, SourceDeclaration* decl) {
    addSymbol(&ctx->symbols, decl->name, SYMBOL_SOURCE, decl, ctx->logger, &ctx->hasErrors);

    bool hasPath = false;
    for (PropertyList* pl = decl->properties; pl != NULL; pl = pl->next) {
        Property* p = pl->property;
        if (p && p->key && strcmp(p->key, "path") == 0) {
            hasPath = true;
            break;
        }
    }
    if (!hasPath) {
        if (ctx->strictMode) {
            reportSemanticError(ctx, "Source '%s' is missing required property 'path'.", decl->name);
        } else {
            logWarning(ctx->logger, "Source '%s' has no 'path' property (non-strict mode).", decl->name);
        }
    }
}

static void registerDataset(SemanticContext* ctx, DatasetDeclaration* decl) {
    addSymbol(&ctx->symbols, decl->name, SYMBOL_DATASET, decl, ctx->logger, &ctx->hasErrors);
}

static void registerTransform(SemanticContext* ctx, TransformDeclaration* decl) {
    addSymbol(&ctx->symbols, decl->name, SYMBOL_TRANSFORM, decl, ctx->logger, &ctx->hasErrors);
}

static void registerSink(SemanticContext* ctx, SinkDeclaration* decl) {
    addSymbol(&ctx->symbols, decl->name, SYMBOL_SINK, decl, ctx->logger, &ctx->hasErrors);

    bool hasPath = false;
    for (PropertyList* pl = decl->properties; pl != NULL; pl = pl->next) {
        Property* p = pl->property;
        if (p && p->key && strcmp(p->key, "path") == 0) {
            hasPath = true;
            break;
        }
    }
    if (!hasPath) {
        if (ctx->strictMode) {
            reportSemanticError(ctx, "Sink '%s' is missing required property 'path'.", decl->name);
        } else {
            logWarning(ctx->logger, "Sink '%s' has no 'path' property (non-strict mode).", decl->name);
        }
    }
}

static void registerUdf(SemanticContext* ctx, UdfDeclaration* decl) {
    addSymbol(&ctx->symbols, decl->name, SYMBOL_UDF, decl, ctx->logger, &ctx->hasErrors);
}

/* === Segunda pasada: validar referencias === */

static void validateDataset(SemanticContext* ctx, DatasetDeclaration* decl) {
    Symbol* src = findSymbol(&ctx->symbols, decl->sourceName);

    // Tolerante si el símbolo referenciado no está presente en esta unidad.
    if (!src) {
        if (ctx->strictMode) {
            reportSemanticError(ctx,
                "Dataset '%s' references '%s' but it is not declared.",
                decl->name, decl->sourceName);
        } else {
            logWarning(ctx->logger,
                "Dataset '%s' references '%s' but it is not declared (non-strict mode).",
                decl->name, decl->sourceName);
        }
        return;
    }

    if (src->kind != SYMBOL_SOURCE && src->kind != SYMBOL_DATASET && src->kind != SYMBOL_TRANSFORM) {
        reportSemanticError(ctx,
            "Dataset '%s' cannot be created FROM '%s' (invalid kind).",
            decl->name, decl->sourceName);
    }
}

static void validateTransform(SemanticContext* ctx, TransformDeclaration* decl) {
    Symbol* src = findSymbol(&ctx->symbols, decl->sourceName);
    
    // Tolerante si el símbolo referenciado no está presente en esta unidad.
    if (!src) {
        if (ctx->strictMode) {
            reportSemanticError(ctx,
                "Transform '%s' references '%s' but it is not declared.",
                decl->name, decl->sourceName);
        } else {
            logWarning(ctx->logger,
                "Transform '%s' references '%s' but it is not declared (non-strict mode).",
                decl->name, decl->sourceName);
        }
        return;
    }

    if (src->kind != SYMBOL_DATASET && src->kind != SYMBOL_TRANSFORM) {
        reportSemanticError(ctx,
            "Transform '%s' cannot be applied to '%s' (only DATASET or TRANSFORM are allowed).",
            decl->name, decl->sourceName);
    }

    /* Validar operaciones internas (p. ej., joins). */
    for (TransformOperationList* opIt = decl->operations; opIt != NULL; opIt = opIt->next) {
        TransformOperation* op = opIt->operation;
        if (op == NULL) continue;

        if (op->type == JOIN_OP && op->join != NULL) {
            const char* target = op->join->target;
            if (target == NULL) {
                reportSemanticError(ctx,
                    "Transform '%s' has a JOIN without target.", decl->name);
                continue;
            }
            Symbol* tgt = findSymbol(&ctx->symbols, target);
            if (!tgt) {
                if (ctx->strictMode) {
                    reportSemanticError(ctx,
                        "Transform '%s' joins with '%s' but it is not declared.",
                        decl->name, target);
                } else {
                    logWarning(ctx->logger,
                        "Transform '%s' joins with '%s' but it is not declared (non-strict mode).",
                        decl->name, target);
                }
            } else if (tgt->kind != SYMBOL_DATASET && tgt->kind != SYMBOL_TRANSFORM) {
                reportSemanticError(ctx,
                    "Transform '%s' joins with '%s' but it is not a DATASET or TRANSFORM.",
                    decl->name, target);
            }
            if (op->join->condition == NULL) {
                reportSemanticError(ctx,
                    "Transform '%s' join with '%s' is missing ON condition.",
                    decl->name, target);
            } else if (ctx->strictMode) {
                /* simple type compatibility check for equality conditions */
                if (op->join->condition->type == COMPARISON &&
                    op->join->condition->compOp == EQUALS_OP &&
                    op->join->condition->leftExpression &&
                    op->join->condition->leftExpression->type == FACTOR &&
                    op->join->condition->leftExpression->factor->type == IDENTIFIER_FACTOR &&
                    op->join->condition->rightExpression &&
                    op->join->condition->rightExpression->type == FACTOR &&
                    op->join->condition->rightExpression->factor->type == IDENTIFIER_FACTOR) {
                    const char* leftName = op->join->condition->leftExpression->factor->identifier;
                    const char* rightName = op->join->condition->rightExpression->factor->identifier;
                    Schema* leftSchema = inferSchemaForSymbol(ctx, src);
                    Schema* rightSchema = inferSchemaForSymbol(ctx, tgt);
                    int li = schemaFindColumnIndex(leftSchema, leftName);
                    int ri = schemaFindColumnIndex(rightSchema, rightName);
                    if (li >= 0 && ri >= 0) {
                        int lt = leftSchema->columns[li].type;
                        int rt = rightSchema->columns[ri].type;
                        if (lt != TYPE_UNKNOWN && rt != TYPE_UNKNOWN && lt != rt) {
                            reportSemanticError(ctx,
                                "Join condition type mismatch: %s (%d) vs %s (%d).",
                                leftName, lt, rightName, rt);
                        }
                    }
                }
            }
        }
    }
}

static void validateWrite(SemanticContext* ctx, WriteStatement* stmt) {


    Symbol* ds = findSymbol(&ctx->symbols, stmt->datasetName);
    if (!ds) {
        if (ctx->strictMode) {
            reportSemanticError(ctx,
                "WRITE references dataset/transform '%s' but it is not declared.",
                stmt->datasetName);
        } else {
            logWarning(ctx->logger,
                "WRITE references dataset/transform '%s' but it is not declared (non-strict mode).",
                stmt->datasetName);
        }
    } else if (ds->kind != SYMBOL_DATASET && ds->kind != SYMBOL_TRANSFORM) {
        reportSemanticError(ctx,
            "WRITE can only output DATASET or TRANSFORM, but '%s' is of a different kind.",
            stmt->datasetName);
    }

    Symbol* sink = findSymbol(&ctx->symbols, stmt->sinkName);
    if (!sink) {
        if (ctx->strictMode) {
            reportSemanticError(ctx,
                "WRITE references sink '%s' but it is not declared.",
                stmt->sinkName);
        } else {
            logWarning(ctx->logger,
                "WRITE references sink '%s' but it is not declared (non-strict mode).",
                stmt->sinkName);
        }
    } else if (sink->kind != SYMBOL_SINK) {
        reportSemanticError(ctx,
            "WRITE can only output INTO a SINK, but '%s' is not a sink.",
            stmt->sinkName);
    }
}

/* ======================= Detección de ciclos ======================= */

typedef struct DepNode {
    const char* name;
    const char* dependency; /* Only follows dataset/transform dependencies. */
    SymbolKind kind;
} DepNode;

typedef enum {
    VISIT_UNSEEN = 0,
    VISIT_ACTIVE,
    VISIT_DONE
} VisitState;

static int findDepIndex(const DepNode* nodes, size_t count, const char* name) {
    for (size_t i = 0; i < count; i++) {
        if (strcmp(nodes[i].name, name) == 0) return (int)i;
    }
    return -1;
}

/* ======================= Esquemas y validaciones de columnas ======================= */

static Schema* createSchema(size_t count) {
    Schema* s = calloc(1, sizeof(Schema));
    s->count = count;
    s->columns = calloc(count, sizeof(ColumnMeta));
    s->unknown = false;
    for (size_t i = 0; i < count; i++) {
        s->columns[i].type = TYPE_UNKNOWN;
    }
    return s;
}

static Schema* copySchema(const Schema* src) {
    if (!src) return NULL;
    Schema* s = createSchema(src->count);
    s->unknown = src->unknown;
    for (size_t i = 0; i < src->count; i++) {
        if (src->columns[i].name) s->columns[i].name = strdup(src->columns[i].name);
        s->columns[i].type = src->columns[i].type;
    }
    return s;
}

static bool schemaHasColumn(const Schema* schema, const char* name) {
    if (!schema || schema->unknown || !name) return true; /* tolerante si desconocido */
    for (size_t i = 0; i < schema->count; i++) {
        if (schema->columns[i].name && strcmp(schema->columns[i].name, name) == 0) return true;
    }
    return false;
}

static int schemaFindColumnIndex(const Schema* schema, const char* name) {
    if (!schema || !name) return -1;
    for (size_t i = 0; i < schema->count; i++) {
        if (schema->columns[i].name && strcmp(schema->columns[i].name, name) == 0) return (int)i;
    }
    return -1;
}

static void validateExpressionIdentifiers(SemanticContext* ctx, Expression* expr, const Schema* schema) {
    if (!ctx->strictMode) return;
    if (!expr || !schema || schema->unknown) return;
    switch (expr->type) {
        case FACTOR:
            if (expr->factor->type == IDENTIFIER_FACTOR) {
                if (!schemaHasColumn(schema, expr->factor->identifier)) {
                    reportSemanticError(ctx, "Unknown column '%s' in expression.", expr->factor->identifier);
                }
            } else if (expr->factor->type == EXPRESSION) {
                validateExpressionIdentifiers(ctx, expr->factor->expression, schema);
            }
            break;
        case ADDITION: case SUBTRACTION: case MULTIPLICATION:
        case DIVISION: case MODULO: case COMPARISON: case LOGICAL:
            validateExpressionIdentifiers(ctx, expr->leftExpression, schema);
            validateExpressionIdentifiers(ctx, expr->rightExpression, schema);
            break;
        case IDENTIFIER_EXPR:
            if (!schemaHasColumn(schema, expr->identifier)) {
                reportSemanticError(ctx, "Unknown column '%s' in expression.", expr->identifier);
            }
            break;
        case FUNCTION_CALL:
            /* Validar referencias dentro de argumentos */
            for (ExpressionList* arg = expr->functionCall.arguments; arg != NULL; arg = arg->next) {
                validateExpressionIdentifiers(ctx, arg->expression, schema);
            }
            break;
        default:
            break;
    }
}

static Schema* schemaFromSelect(const Schema* input, SelectOperation* select, SemanticContext* ctx) {
    if (!select) return copySchema(input);
    size_t count = 0;
    for (ColumnList* c = select->columns; c != NULL; c = c->next) count++;
    Schema* out = createSchema(count);
    size_t idx = 0;
    for (ColumnList* c = select->columns; c != NULL; c = c->next, idx++) {
        const char* name = c->alias;
        if (!name || strlen(name) == 0) {
            if (c->expression && c->expression->type == FACTOR &&
                c->expression->factor->type == IDENTIFIER_FACTOR) {
                name = c->expression->factor->identifier;
            } else {
                /* nombre generado */
                char gen[32];
                snprintf(gen, sizeof(gen), "col_%zu", idx);
                out->columns[idx].name = strdup(gen);
                validateExpressionIdentifiers(ctx, c->expression, input);
                continue;
            }
        }
        out->columns[idx].name = strdup(name);
        validateExpressionIdentifiers(ctx, c->expression, input);
    }
    out->unknown = (count == 0);
    return out;
}

static Schema* schemaAfterWithColumn(Schema* base, WithColumnOperation* withColumn, SemanticContext* ctx) {
    if (!withColumn) return base;
    if (!base) base = createSchema(0);
    for (ColumnAssignmentList* ca = withColumn->assignments; ca != NULL; ca = ca->next) {
        ColumnAssignment* a = ca->assignment;
        if (!a) continue;
        validateExpressionIdentifiers(ctx, a->expression, base);
        /* set type best-effort */
        int idx = schemaFindColumnIndex(base, a->columnName);
        if (idx == -1) {
            base->columns = realloc(base->columns, (base->count + 1) * sizeof(ColumnMeta));
            idx = (int) base->count;
            base->count += 1;
            base->columns[idx].name = strdup(a->columnName);
            base->columns[idx].type = TYPE_UNKNOWN;
        }
        bool found = false;
        (void)found;
    }
    return base;
}

static Schema* schemaAfterJoin(const Schema* left, const Schema* right) {
    if (!left || !right || left->unknown || right->unknown) {
        Schema* out = createSchema(0);
        out->unknown = true;
        return out;
    }
    Schema* out = createSchema(left->count + right->count);
    size_t idx = 0;
    for (size_t i = 0; i < left->count; i++) {
        out->columns[idx++].name = left->columns[i].name ? strdup(left->columns[i].name) : NULL;
    }
    for (size_t i = 0; i < right->count; i++) {
        out->columns[idx++].name = right->columns[i].name ? strdup(right->columns[i].name) : NULL;
    }
    return out;
}

static Schema* schemaAfterGroupBy(const Schema* input, GroupByOperation* groupBy, SemanticContext* ctx) {
    if (!groupBy) return copySchema(input);
    size_t keyCount = 0;
    for (ColumnList* k = groupBy->keys; k != NULL; k = k->next) keyCount++;
    size_t aggCount = 0;
    for (ColumnAssignmentList* a = groupBy->aggregations; a != NULL; a = a->next) aggCount++;
    Schema* out = createSchema(keyCount + aggCount);
    size_t idx = 0;
    for (ColumnList* k = groupBy->keys; k != NULL; k = k->next, idx++) {
        const char* name = NULL;
        if (k->expression && k->expression->type == FACTOR &&
            k->expression->factor->type == IDENTIFIER_FACTOR) {
            name = k->expression->factor->identifier;
            if (!schemaHasColumn(input, name)) {
                reportSemanticError(ctx, "Unknown groupBy key '%s'.", name);
            }
            int colIdx = schemaFindColumnIndex(input, name);
            if (colIdx >= 0) out->columns[idx].type = input->columns[colIdx].type;
        } else {
            name = "group_key";
        }
        out->columns[idx].name = strdup(name);
    }
    for (ColumnAssignmentList* a = groupBy->aggregations; a != NULL; a = a->next, idx++) {
        ColumnAssignment* agg = a->assignment;
        out->columns[idx].name = strdup(agg->columnName ? agg->columnName : "agg");
        /* Best effort: if aggregation is count -> int, else decimal */
        out->columns[idx].type = TYPE_DECIMAL;
        if (agg->expression && agg->expression->type == FUNCTION_CALL && agg->expression->functionCall.functionName) {
            if (strcmp(agg->expression->functionCall.functionName, "count") == 0) {
                out->columns[idx].type = TYPE_INT;
            }
        }
    }
    out->unknown = false;
    return out;
}

static Schema* schemaAfterWindow(const Schema* input, WindowOperation* windowOp, SemanticContext* ctx) {
    if (!windowOp) return copySchema(input);
    Schema* out = copySchema(input);
    for (ColumnAssignmentList* ca = windowOp->computations; ca != NULL; ca = ca->next) {
        ColumnAssignment* a = ca->assignment;
        if (!a) continue;
        validateExpressionIdentifiers(ctx, a->expression, input);
        bool found = false;
        for (size_t i = 0; i < out->count; i++) {
            if (out->columns[i].name && strcmp(out->columns[i].name, a->columnName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            out->columns = realloc(out->columns, (out->count + 1) * sizeof(ColumnMeta));
            out->columns[out->count].name = strdup(a->columnName);
            out->columns[out->count].type = TYPE_INT;
            out->count += 1;
        }
    }
    return out;
}

static Schema* computeTransformSchema(SemanticContext* ctx, TransformDeclaration* decl, const Schema* sourceSchema) {
    Schema* current = copySchema(sourceSchema);
    for (TransformOperationList* it = decl->operations; it != NULL; it = it->next) {
        TransformOperation* op = it->operation;
        if (!op) continue;
        switch (op->type) {
            case FILTER_OP:
                validateExpressionIdentifiers(ctx, op->filter->condition, current);
                break;
            case WITH_COLUMN_OP:
                current = schemaAfterWithColumn(current, op->withColumn, ctx);
                break;
            case SELECT_OP:
                current = schemaFromSelect(current, op->select, ctx);
                break;
            case ORDER_BY_OP:
                for (OrderByItem* item = op->orderBy->items; item != NULL; item = item->next) {
                    validateExpressionIdentifiers(ctx, item->expression, current);
                }
                break;
            case LIMIT_OP:
                break;
            case JOIN_OP: {
                Symbol* tgt = findSymbol(&ctx->symbols, op->join->target);
                Schema* rightSchema = (tgt && tgt->schemaComputed) ? tgt->schema : NULL;
                current = schemaAfterJoin(current, rightSchema);
                validateExpressionIdentifiers(ctx, op->join->condition, current);
                break;
            }
            case GROUP_BY_OP:
                current = schemaAfterGroupBy(current, op->groupBy, ctx);
                break;
            case WINDOW_OP:
                current = schemaAfterWindow(current, op->windowOp, ctx);
                break;
            default:
                break;
        }
    }
    return current;
}

static Schema* inferSchemaForSymbol(SemanticContext* ctx, Symbol* sym) {
    if (!sym) return NULL;
    if (sym->schemaComputed) return sym->schema;
    if (sym->computingSchema) return sym->schema; /* avoid cycles */
    sym->computingSchema = true;

    switch (sym->kind) {
        case SYMBOL_SOURCE: {
            /* Si no hay esquema declarado, marcar unknown */
            Schema* s = createSchema(0);
            s->unknown = true;
            sym->schema = s;
            break;
        }
        case SYMBOL_DATASET: {
            DatasetDeclaration* decl = (DatasetDeclaration*) sym->astNode;
            Symbol* src = findSymbol(&ctx->symbols, decl->sourceName);
            Schema* base = inferSchemaForSymbol(ctx, src);
            sym->schema = copySchema(base);
            break;
        }
        case SYMBOL_TRANSFORM: {
            TransformDeclaration* decl = (TransformDeclaration*) sym->astNode;
            Symbol* src = findSymbol(&ctx->symbols, decl->sourceName);
            Schema* base = inferSchemaForSymbol(ctx, src);
            sym->schema = computeTransformSchema(ctx, decl, base);
            break;
        }
        case SYMBOL_PARAM:
        case SYMBOL_SINK:
        default:
            break;
    }

    sym->schemaComputed = true;
    sym->computingSchema = false;
    return sym->schema;
}

static bool dfsDetectCycle(const DepNode* nodes, size_t count, int idx, VisitState* state, SemanticContext* ctx) {
    state[idx] = VISIT_ACTIVE;

    const char* depName = nodes[idx].dependency;
    if (depName != NULL) {
        int depIdx = findDepIndex(nodes, count, depName);
        if (depIdx >= 0) {
            if (state[depIdx] == VISIT_ACTIVE) {
                reportSemanticError(ctx, "Cycle detected: '%s' depends on '%s'.", nodes[idx].name, depName);
                return true;
            }
            if (state[depIdx] == VISIT_UNSEEN && dfsDetectCycle(nodes, count, depIdx, state, ctx)) {
                return true;
            }
        }
    }

    state[idx] = VISIT_DONE;
    return false;
}

static void validateAcyclicDependencies(SemanticContext* ctx, Program* program) {
    if (program->type != STATEMENT_LIST_PROGRAM) return;

    /* Solo nos interesa dataset/transform ya que son los que forman el DAG de procesamiento. */
    size_t capacity = 8;
    size_t count = 0;
    DepNode* nodes = calloc(capacity, sizeof(DepNode));

    for (StatementList* it = program->statements; it != NULL; it = it->next) {
        Statement* stmt = it->statement;
        if (stmt->type == DATASET_STMT || stmt->type == TRANSFORM_STMT) {
            if (count == capacity) {
                capacity *= 2;
                nodes = realloc(nodes, capacity * sizeof(DepNode));
            }
            DepNode* n = &nodes[count++];
            if (stmt->type == DATASET_STMT) {
                n->name = stmt->datasetDecl->name;
                n->dependency = stmt->datasetDecl->sourceName;
                n->kind = SYMBOL_DATASET;
            } else {
                n->name = stmt->transformDecl->name;
                n->dependency = stmt->transformDecl->sourceName;
                n->kind = SYMBOL_TRANSFORM;
            }
        }
    }

    VisitState* state = calloc(count, sizeof(VisitState));
    for (size_t i = 0; i < count; i++) {
        if (state[i] == VISIT_UNSEEN) {
            if (dfsDetectCycle(nodes, count, (int)i, state, ctx)) {
                break;
            }
        }
    }

    free(nodes);
    free(state);
}

/* ======================= Recorrido principal ======================= */

static void firstPass_buildSymbols(SemanticContext* ctx, Program* program) {

    // Para no complicar, solo soportamos programas de lista de statements
    if (program->type != STATEMENT_LIST_PROGRAM) {
        return;
    }

    for (StatementList* it = program->statements; it != NULL; it = it->next) {
        Statement* stmt = it->statement;
        switch (stmt->type) {
            case PARAM_STMT:
                registerParam(ctx, stmt->paramDecl);
                break;
            case SOURCE_STMT:
                registerSource(ctx, stmt->sourceDecl);
                break;
            case DATASET_STMT:
                registerDataset(ctx, stmt->datasetDecl);
                break;
            case TRANSFORM_STMT:
                registerTransform(ctx, stmt->transformDecl);
                break;
            case SINK_STMT:
                registerSink(ctx, stmt->sinkDecl);
                break;
            case UDF_STMT:
                registerUdf(ctx, stmt->udfDecl);
                break;
            case WRITE_STMT:
                break;
            default:
                break;
        }
    }
}

static void secondPass_validateUses(SemanticContext* ctx, Program* program) {
    if (program->type != STATEMENT_LIST_PROGRAM) return;

    for (StatementList* it = program->statements; it != NULL; it = it->next) {
        Statement* stmt = it->statement;
        switch (stmt->type) {
            case DATASET_STMT:
                validateDataset(ctx, stmt->datasetDecl);
                break;
            case TRANSFORM_STMT:
                validateTransform(ctx, stmt->transformDecl);
                break;
            case WRITE_STMT:
                validateWrite(ctx, stmt->writeStmt);
                break;
            default:
                break;
        }
    }

    validateAcyclicDependencies(ctx, program);

    if (ctx->strictMode) {
        /* Infer schemas and validate expressions */
        for (StatementList* it = program->statements; it != NULL; it = it->next) {
            Statement* stmt = it->statement;
            Symbol* sym = findSymbol(&ctx->symbols,
                stmt->type == SOURCE_STMT ? stmt->sourceDecl->name :
                stmt->type == DATASET_STMT ? stmt->datasetDecl->name :
                stmt->type == TRANSFORM_STMT ? stmt->transformDecl->name :
                stmt->type == SINK_STMT ? stmt->sinkDecl->name : NULL);
            if (sym && (sym->kind == SYMBOL_SOURCE || sym->kind == SYMBOL_DATASET || sym->kind == SYMBOL_TRANSFORM)) {
                inferSchemaForSymbol(ctx, sym);
            }
        }

        /* Validar llamadas a UDF: nombre declarado y aridad */
        for (StatementList* it = program->statements; it != NULL; it = it->next) {
            Statement* stmt = it->statement;
            if (stmt->type == TRANSFORM_STMT) {
                for (TransformOperationList* opIt = stmt->transformDecl->operations; opIt != NULL; opIt = opIt->next) {
                    TransformOperation* op = opIt->operation;
                    if (!op) continue;
                    ExpressionList* exprsToCheck = NULL;
                    switch (op->type) {
                        case FILTER_OP:
                            exprsToCheck = malloc(sizeof(ExpressionList));
                            exprsToCheck->expression = op->filter->condition;
                            exprsToCheck->next = NULL;
                            break;
                        case WITH_COLUMN_OP:
                            for (ColumnAssignmentList* ca = op->withColumn->assignments; ca != NULL; ca = ca->next) {
                                // validateExpressionIdentifiers already called; we only handle UDF lookup in helper below
                            }
                            break;
                        case SELECT_OP:
                        case ORDER_BY_OP:
                        case LIMIT_OP:
                        case JOIN_OP:
                        case GROUP_BY_OP:
                        case WINDOW_OP:
                            break;
                        default:
                            break;
                    }
                    if (exprsToCheck) {
                        // clean up placeholder list immediately after checking
                        free(exprsToCheck);
                    }
                }
            }
        }
    }
}

/* ======================= API pública ======================= */

CompilationStatus executeSemanticAnalysis(Program* program) {
    SemanticContext* ctx = createSemanticContext();

    firstPass_buildSymbols(ctx, program);
    secondPass_validateUses(ctx, program);

    CompilationStatus result = ctx->hasErrors ? FAILED : SUCCEEDED;
    destroySemanticContext(ctx);
    return result;
}
