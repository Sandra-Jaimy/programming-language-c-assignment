// Sandra Jaimy 241ADB123
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -lm -o calc calc.c
// PROGRAM is C
//
// -----------------------------------------------------------------------------
//  PYTHONIC ARITHMETIC PARSER IN C
// -----------------------------------------------------------------------------
//  This program evaluates arithmetic expressions (like Python) from text files.
//  It supports integers, floats, + - * / ** operators, parentheses, and comments.
//  It reports the first error with its exact 1-based character position.
//  For directories (-d), all .txt files are processed, and output is written to
//  <basename>_Sandra_Jaimy_241ADB123.txt in an output folder.
//
//  Language: Standard C17
//  Author  : Sandra Jaimy
//  Student : 241ADB123
//
// -----------------------------------------------------------------------------
//  IMPLEMENTATION OVERVIEW
// -----------------------------------------------------------------------------
//  1. TOKENIZER — converts input characters into typed tokens (+, -, numbers, etc.)
//  2. PARSER/EVALUATOR — recursive descent parser that directly computes results.
//  3. ERROR HANDLING — tracks first invalid token position for ERROR:<pos> output.
//  4. FILE HANDLING — supports single file or directory (-d), output folder (-o).
//  5. FORMATTING — prints integer if result integral; otherwise prints as double.
//
//  Precedence & associativity:
//    ** (right-associative)
//    unary + -
//    * /
//    + -
// -----------------------------------------------------------------------------
//
//  References and inspiration:
//  - Based on Python operator precedence rules
//  - Tokenizer/Parser design from recursive descent patterns (K&R, Wikipedia)
//  - Prompt reference: "recursive descent parser in C for arithmetic" (ChatGPT)
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

#define USER_FIRST "Sandra"
#define USER_LAST  "Jaimy"
#define USER_ID    "241ADB123"

#define MAX_EXPR_LEN 10000  // Allow long multi-line files (Grade 10 requirement)

// -----------------------------------------------------------------------------
//  TOKEN DEFINITIONS
// -----------------------------------------------------------------------------
typedef enum {
    T_NUM, T_PLUS, T_MINUS, T_MUL, T_DIV, T_POW,
    T_LPAREN, T_RPAREN, T_END, T_INVALID
} TokenType;

// Each token stores its type, numeric value (if number), and starting char index
typedef struct {
    TokenType type;
    double value;
    int pos;
} Token;

// -----------------------------------------------------------------------------
//  GLOBAL PARSER STATE
// -----------------------------------------------------------------------------
static const char *src;   // pointer to input string
static int pos = 0;       // current index in source string (0-based)
static Token current;     // currently active token
static int error_pos = 0; // first error position (1-based index)

// -----------------------------------------------------------------------------
//  TOKENIZER — SCAN NEXT TOKEN FROM SOURCE
// -----------------------------------------------------------------------------

// Skip spaces and tabs (whitespace ignored between tokens)
void skip_ws() {
    while (isspace(src[pos])) pos++;
}

// Read next token from input
Token next_token() {
    skip_ws();
    Token t = {T_END, 0, pos + 1};  // default token
    char c = src[pos];
    if (c == '\0') return t;

    // Parse numbers (int or float)
    if (isdigit(c) || c == '.') {
        char *end;
        errno = 0;
        t.value = strtod(&src[pos], &end);
        t.type = T_NUM;
        t.pos = pos + 1;
        pos = end - src;
        return t;
    }

    // Operators and parentheses
    if (c == '+') { t.type = T_PLUS; t.pos = ++pos; return t; }
    if (c == '-') { t.type = T_MINUS; t.pos = ++pos; return t; }
    if (c == '*') {
        // Check for '' (power)
        if (src[pos + 1] == '*') {
            t.type = T_POW; t.pos = pos + 1; pos += 2;
        } else { t.type = T_MUL; t.pos = ++pos; }
        return t;
    }
    if (c == '/') { t.type = T_DIV; t.pos = ++pos; return t; }
    if (c == '(') { t.type = T_LPAREN; t.pos = ++pos; return t; }
    if (c == ')') { t.type = T_RPAREN; t.pos = ++pos; return t; }

    // Invalid character
    t.type = T_INVALID;
    t.pos = pos + 1;
    return t;
}

// Advance to the next token
void advance() { current = next_token(); }

// Record first error position
void fail(int position) {
    if (!error_pos) error_pos = position;
}

// -----------------------------------------------------------------------------
//  RECURSIVE DESCENT PARSER
//  Grammar (with unary operators):
//    expr    := term { ('+' | '-') term }
//    term    := factor { ('*' | '/') factor }
//    factor  := unary
//    unary   := ('+' | '-') unary | power
//    power   := primary { '' power }   // right-associative
//    primary := NUMBER | '(' expr ')'
// -----------------------------------------------------------------------------

double parse_expr(); // forward declaration

// Parse primary elements: numbers or parentheses
double parse_primary() {
    if (error_pos) return 0;
    if (current.type == T_NUM) {
        double v = current.value;
        advance();
        return v;
    }
    if (current.type == T_LPAREN) {
        int start = current.pos;
        advance();
        double v = parse_expr();
        if (current.type != T_RPAREN) {
            fail(start);  // unmatched '('
            return 0;
        }
        advance();
        return v;
    }
    // Unexpected token
    fail(current.pos);
    return 0;
}

// Parse exponentiation (), right-associative
double parse_power() {
    if (error_pos) return 0;
    double base = parse_primary();
    if (current.type == T_POW) {
        int ppos = current.pos;
        advance();
        double exp = parse_power(); // recursion for right-associativity
        base = pow(base, exp);
        if (errno == EDOM || errno == ERANGE)
            fail(ppos);  // invalid domain or overflow
    }
    return base;
}

// Parse unary operators + and -
double parse_factor() {
    if (error_pos) return 0;
    if (current.type == T_PLUS) {
        advance();
        return parse_factor();
    }
    if (current.type == T_MINUS) {
        advance();
        return -parse_factor();
    }
    return parse_power();
}

// Parse * and / operators (left-associative)
double parse_term() {
    if (error_pos) return 0;
    double v = parse_factor();
    while (!error_pos && (current.type == T_MUL || current.type == T_DIV)) {
        TokenType op = current.type;
        int p = current.pos;
        advance();
        double rhs = parse_factor();
        if (op == T_MUL)
            v *= rhs;
        else {
            if (rhs == 0.0) { fail(p); return 0; } // division by zero
            v /= rhs;
        }
    }
    return v;
}

// Parse + and - operators (left-associative)
double parse_expr() {
    if (error_pos) return 0;
    double v = parse_term();
    while (!error_pos && (current.type == T_PLUS || current.type == T_MINUS)) {
        TokenType op = current.type;
        advance();
        double rhs = parse_term();
        if (op == T_PLUS) v += rhs;
        else v -= rhs;
    }
    return v;
}

// -----------------------------------------------------------------------------
//  EXPRESSION EVALUATION ENTRY POINT
// -----------------------------------------------------------------------------
void evaluate_expression(const char *input, const char *outfile) {
    src = input;
    pos = 0;
    error_pos = 0;
    advance();

    double result = parse_expr();

    // Detect leftover tokens
    if (!error_pos && current.type != T_END)
        fail(current.pos);

    FILE *out = fopen(outfile, "w");
    if (!out) { perror("output"); return; }

    // If error occurred, print ERROR:<pos>
    if (error_pos) {
        fprintf(out, "ERROR:%d\n", error_pos);
    } else {
        // Output formatting: print as integer if value is integral
        long long int_val = llround(result);
        if (fabs(result - int_val) < 1e-12)
            fprintf(out, "%lld\n", int_val);
        else
            fprintf(out, "%.15g\n", result);
    }
    fclose(out);
}

// -----------------------------------------------------------------------------
//  FILE HANDLING UTILITIES
// -----------------------------------------------------------------------------

// Read entire file into memory
char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

// Process a single input file and generate corresponding output
void process_file(const char *input_path, const char *outdir) {
    // Derive output filename: <basename>_Sandra_Jaimy_241ADB123.txt
    const char *fname = strrchr(input_path, '/');
    fname = fname ? fname + 1 : input_path;
    char basename[256];
    strncpy(basename, fname, sizeof(basename));
    char *dot = strrchr(basename, '.');
    if (dot) *dot = '\0';

    char outfile[512];
    snprintf(outfile, sizeof(outfile), "%s/%s_%s_%s_%s.txt",
             outdir, basename, USER_FIRST, USER_LAST, USER_ID);

    char *content = read_file(input_path);
    if (!content) {
        fprintf(stderr, "Cannot read %s\n", input_path);
        return;
    }

    // Remove comment lines starting with '#'
    char *filtered = malloc(strlen(content) + 1);
    filtered[0] = '\0';
    const char *line = content;
    while (*line) {
        while (isspace(*line)) line++;
        if (*line == '#') {
            // Skip the comment line
            while (*line && *line != '\n') line++;
            if (*line == '\n') line++;
            continue;
        }
        const char *start = line;
        while (*line && *line != '\n') line++;
        strncat(filtered, start, line - start);
        if (*line == '\n') strncat(filtered, "\n", 1), line++;
    }

    evaluate_expression(filtered, outfile);
    free(filtered);
    free(content);
}

// -----------------------------------------------------------------------------
//  MAIN PROGRAM ENTRY
// -----------------------------------------------------------------------------
int main(int argc, char **argv) {
    const char *indir = NULL;
    const char *outdir = NULL;
    const char *single = NULL;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) && i + 1 < argc)
            indir = argv[++i];
        else if ((strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output-dir") == 0) && i + 1 < argc)
            outdir = argv[++i];
        else
            single = argv[i];
    }

    // Create default output directory if none provided
    char default_out[256];
    if (!outdir) {
        if (single) {
            const char *b = strrchr(single, '/');
            b = b ? b + 1 : single;
            char name[128];
            strncpy(name, b, sizeof(name));
            char *dot = strrchr(name, '.');
            if (dot) *dot = '\0';
            snprintf(default_out, sizeof(default_out), "%s_%s_%s_%s", name, USER_FIRST, USER_LAST, USER_ID);
        } else
            snprintf(default_out, sizeof(default_out), "labs_%s_%s_%s", USER_FIRST, USER_LAST, USER_ID);
        outdir = default_out;
    }

    mkdir(outdir, 0775); // Create output directory if missing

    // Batch mode: process all .txt files in directory
    if (indir) {
        DIR *d = opendir(indir);
        if (!d) { perror("dir"); return 1; }
        struct dirent *e;
        while ((e = readdir(d))) {
            if (strstr(e->d_name, ".txt")) {
                char path[512];
                snprintf(path, sizeof(path), "%s/%s", indir, e->d_name);
                process_file(path, outdir);
            }
        }
        closedir(d);
    }
    // Single file mode
    else if (single) {
        process_file(single, outdir);
    }
    // Invalid invocation
    else {
        fprintf(stderr, "Usage: calc [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] input.txt\n");
        return 1;
    }

    return 0;
}
