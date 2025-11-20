#include "Generator.h"
#include "../../frontend/syntactic-analysis/AbstractSyntaxTree.h"
#include "../../support/logging/Logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============ Estado interno del módulo ============ */

static Logger * _logger = NULL;

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

static void openOutputs() {
    _dotFile = fopen("pipeline.dot", "w");
    _manifestFile = fopen("manifest.json", "w");
    if (!_dotFile || !_manifestFile) {
        logError(_logger, "Cannot open output files pipeline.dot / manifest.json");
        // Podrías manejar errores mejor si querés
    }
}

static void closeOutputs() {
    if (_dotFile) fclose(_dotFile);
    if (_manifestFile) fclose(_manifestFile);
}

/* Helpers para escribir (podrías envolverlos más si querés) */

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

/* ============ Recorridos del AST ============ */

/**
 * Acá asumimos la misma forma de Program que usaste en el semántico:
 *   - program->type == STATEMENT_LIST_PROGRAM
 *   - program->statements es una lista de StatementList
 *   - Statement tiene .type y punteros a paramDecl, sourceDecl, datasetDecl, ...
 *
 * Vamos a hacer:
 *   - generateDot(program)
 *   - generateManifest(program)
 */

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
                // edge source -> dataset
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
                break;
            case SINK_STMT:
                dotOut("    \"%s\" [shape=box, style=filled, fillcolor=\"orange\"];\n",
                       stmt->sinkDecl->name);
                break;
            case WRITE_STMT:
                // edge dataset/transform -> sink
                dotOut("    \"%s\" -> \"%s\";\n",
                       stmt->writeStmt->datasetName,
                       stmt->writeStmt->sinkName);
                break;
            case PARAM_STMT:
                // Podés decidir si aparecen en el grafo o no
                break;
        }
    }

    dotOut("}\n");
}

static void generateManifest(Program *program) {
    // Manifest muy simple; podés enriquecerlo después (propiedades, tipos, etc).
    manifestOut("{\n");
    manifestOut("  \"parameters\": [\n");
    bool first = true;

    // ---------- parameters ----------
    for (StatementList *it = program->statements; it != NULL; it = it->next) {
        Statement *stmt = it->statement;
        if (stmt->type == PARAM_STMT) {
            if (!first) manifestOut(",\n");
            first = false;
            manifestOut("    { \"name\": \"%s\" }", stmt->paramDecl->name);
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

        switch (stmt->type) {
            case SOURCE_STMT:
                kind = "source"; name = stmt->sourceDecl->name; break;
            case DATASET_STMT:
                kind = "dataset"; name = stmt->datasetDecl->name; break;
            case TRANSFORM_STMT:
                kind = "transform"; name = stmt->transformDecl->name; break;
            case SINK_STMT:
                kind = "sink"; name = stmt->sinkDecl->name; break;
            default:
                break;
        }

        if (kind != NULL) {
            if (!first) manifestOut(",\n");
            first = false;
            manifestOut("    { \"name\": \"%s\", \"kind\": \"%s\" }", name, kind);
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
    }
    manifestOut("\n  ]\n");
    manifestOut("}\n");
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
			//Programas de solo expresión no generan código para el backend Dataly.
            logDebugging(_logger, "Expression-only program: nothing to generate for Dataly backend.");
            return SUCCEEDED;
        }
        /* Unknown program kind -> fail. */
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

    closeOutputs();
    logInformation(_logger, "Code generation completed.");
    return SUCCEEDED;
}
