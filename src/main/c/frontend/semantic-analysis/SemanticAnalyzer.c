#include "SemanticAnalyzer.h"
#include "../../support/logging/Logger.h"
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

/* ======================= Symbol table ======================= */

typedef enum {
    SYMBOL_PARAM,
    SYMBOL_SOURCE,
    SYMBOL_DATASET,
    SYMBOL_TRANSFORM,
    SYMBOL_SINK
} SymbolKind;

typedef struct Symbol {
    char* name;          
    SymbolKind kind;
    void* astNode;       
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
    return true;
}

/* ======================= Semantic context ======================= */

typedef struct SemanticContext {
    SymbolTable symbols;
    Logger* logger;
    bool hasErrors;
} SemanticContext;

static SemanticContext* createSemanticContext() {
    SemanticContext* ctx = calloc(1, sizeof(SemanticContext));
    initSymbolTable(&ctx->symbols);
    ctx->logger = createLogger("SemanticAnalyzer");
    ctx->hasErrors = false;
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

}

/* === Segunda pasada: validar referencias === */

static void validateDataset(SemanticContext* ctx, DatasetDeclaration* decl) {
    Symbol* src = findSymbol(&ctx->symbols, decl->sourceName);

    // Tolerante si el símbolo referenciado no está presente en esta unidad.
    if (!src) return;

    if (src->kind != SYMBOL_SOURCE && src->kind != SYMBOL_DATASET && src->kind != SYMBOL_TRANSFORM) {
        reportSemanticError(ctx,
            "Dataset '%s' cannot be created FROM '%s' (invalid kind).",
            decl->name, decl->sourceName);
    }
}

static void validateTransform(SemanticContext* ctx, TransformDeclaration* decl) {
    Symbol* src = findSymbol(&ctx->symbols, decl->sourceName);
    
    // Tolerante si el símbolo referenciado no está presente en esta unidad.
    if (!src) return;

    if (src->kind != SYMBOL_DATASET && src->kind != SYMBOL_TRANSFORM) {
        reportSemanticError(ctx,
            "Transform '%s' cannot be applied to '%s' (only DATASET or TRANSFORM are allowed).",
            decl->name, decl->sourceName);
    }

}

static void validateWrite(SemanticContext* ctx, WriteStatement* stmt) {


    Symbol* ds = findSymbol(&ctx->symbols, stmt->datasetName);
    if (ds) {
        if (ds->kind != SYMBOL_DATASET && ds->kind != SYMBOL_TRANSFORM) {
            reportSemanticError(ctx,
                "WRITE can only output DATASET or TRANSFORM, but '%s' is of a different kind.",
                stmt->datasetName);
        }
    }

    Symbol* sink = findSymbol(&ctx->symbols, stmt->sinkName);
    if (sink) {
        if (sink->kind != SYMBOL_SINK) {
            reportSemanticError(ctx,
                "WRITE can only output INTO a SINK, but '%s' is not a sink.",
                stmt->sinkName);
        }
    }
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
            case WRITE_STMT:
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
