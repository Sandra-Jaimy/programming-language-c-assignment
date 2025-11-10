// Sandra Jaimy 241ADB123
// Compile with: gcc -O2 -Wall -Wextra -std=c17 -o calc calc.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>

// Token types for our lexer
typedef enum {
    TOK_EOF,
    TOK_NUMBER,
    TOK_PLUS,
    TOK_MINUS,
    TOK_STAR,
    TOK_SLASH,
    TOK_POW,
    TOK_LPAREN,
    TOK_RPAREN
} TokenType;

// Token structure with position information
typedef struct {
    TokenType type;
    double value;        // For numbers
    int start_pos;       // 1-based starting position in input
    int length;          // Token length
} Token;

// Parser state structure
typedef struct {
    const char *input;    // Input string
    int length;           // Input length
    int pos;              // Current position (0-based)
    int char_count;       // Character count (1-based, for error reporting)
    Token current_token;  // Current token being processed
    int has_error;        // Error flag
    int error_pos;        // Position of first error
} ParserState;

// Custom power function to avoid math.h dependency
double custom_pow(double base, double exponent) {
    if (exponent == 0.0) return 1.0;
    if (exponent == 1.0) return base;
    
    // For negative exponents
    if (exponent < 0) {
        return 1.0 / custom_pow(base, -exponent);
    }
    
    // Integer exponent case
    double result = 1.0;
    int exp_int = (int)exponent;
    for (int i = 0; i < exp_int; i++) {
        result *= base;
    }
    return result;
}

// Function prototypes
void tokenize(ParserState *state);
double parse_expression(ParserState *state);
double parse_term(ParserState *state);
double parse_factor(ParserState *state);
double parse_power(ParserState *state);
double parse_primary(ParserState *state);
void fail(ParserState *state, int pos);
void skip_whitespace(ParserState *state);
int is_operator_char(char c);

// Initialize parser state
void init_parser(ParserState *state, const char *input, int length) {
    state->input = input;
    state->length = length;
    state->pos = 0;
    state->char_count = 1;
    state->has_error = 0;
    state->error_pos = -1;
    tokenize(state);
}

// Report error with position
void fail(ParserState *state, int pos) {
    if (!state->has_error) {
        state->has_error = 1;
        state->error_pos = pos;
    }
}

// Skip whitespace and update position counter
void skip_whitespace(ParserState *state) {
    while (state->pos < state->length && isspace(state->input[state->pos])) {
        state->char_count++;
        state->pos++;
    }
}

// Check if character is an operator
int is_operator_char(char c) {
    return c == '+' || c == '-' || c == '*' || c == '/' || c == '(' || c == ')';
}

// Tokenize the input - convert character stream to tokens
void tokenize(ParserState *state) {
    skip_whitespace(state);
    
    if (state->pos >= state->length) {
        state->current_token.type = TOK_EOF;
        state->current_token.start_pos = state->char_count;
        state->current_token.length = 0;
        return;
    }
    
    char current_char = state->input[state->pos];
    int start_char_count = state->char_count;
    
    // Handle numbers
    if (isdigit(current_char) || current_char == '.') {
        char *endptr;
        double value = strtod(state->input + state->pos, &endptr);
        
        if (endptr == state->input + state->pos) {
            fail(state, state->char_count);
            state->current_token.type = TOK_EOF;
            return;
        }
        
        int token_length = endptr - (state->input + state->pos);
        state->current_token.type = TOK_NUMBER;
        state->current_token.value = value;
        state->current_token.start_pos = start_char_count;
        state->current_token.length = token_length;
        
        state->pos += token_length;
        state->char_count += token_length;
        return;
    }
    
    // Handle operators and parentheses
    state->current_token.start_pos = start_char_count;
    state->current_token.length = 1;
    
    switch (current_char) {
        case '+':
            state->current_token.type = TOK_PLUS;
            break;
        case '-':
            state->current_token.type = TOK_MINUS;
            break;
        case '*':
            if (state->pos + 1 < state->length && state->input[state->pos + 1] == '*') {
                state->current_token.type = TOK_POW;
                state->current_token.length = 2;
                state->pos++;
                state->char_count++;
            } else {
                state->current_token.type = TOK_STAR;
            }
            break;
        case '/':
            state->current_token.type = TOK_SLASH;
            break;
        case '(':
            state->current_token.type = TOK_LPAREN;
            break;
        case ')':
            state->current_token.type = TOK_RPAREN;
            break;
        default:
            fail(state, state->char_count);
            state->current_token.type = TOK_EOF;
            return;
    }
    
    state->pos++;
    state->char_count++;
}

// Parse primary expressions
double parse_primary(ParserState *state) {
    if (state->has_error) return 0.0;
    
    if (state->current_token.type == TOK_NUMBER) {
        double value = state->current_token.value;
        tokenize(state);
        return value;
    }
    else if (state->current_token.type == TOK_LPAREN) {
        tokenize(state);
        double result = parse_expression(state);
        
        if (state->current_token.type != TOK_RPAREN) {
            fail(state, state->current_token.start_pos);
            return 0.0;
        }
        tokenize(state);
        return result;
    }
    else {
        fail(state, state->current_token.start_pos);
        return 0.0;
    }
}

// Parse power operator
double parse_power(ParserState *state) {
    if (state->has_error) return 0.0;
    
    double left = parse_primary(state);
    
    while (!state->has_error && state->current_token.type == TOK_POW) {
        tokenize(state);
        double right = parse_power(state);
        if (state->has_error) return 0.0;
        
        left = custom_pow(left, right);
    }
    
    return left;
}

// Parse multiplication and division
double parse_factor(ParserState *state) {
    if (state->has_error) return 0.0;
    
    double result = parse_power(state);
    
    while (!state->has_error && 
          (state->current_token.type == TOK_STAR || state->current_token.type == TOK_SLASH)) {
        TokenType op = state->current_token.type;
        tokenize(state);
        
        double right = parse_power(state);
        if (state->has_error) return 0.0;
        
        if (op == TOK_STAR) {
            result *= right;
        } else {
            if (right == 0.0) {
                fail(state, state->current_token.start_pos);
                return 0.0;
            }
            result /= right;
        }
    }
    
    return result;
}

// Parse addition and subtraction
double parse_term(ParserState *state) {
    if (state->has_error) return 0.0;
    
    double result = parse_factor(state);
    
    while (!state->has_error && 
          (state->current_token.type == TOK_PLUS || state->current_token.type == TOK_MINUS)) {
        TokenType op = state->current_token.type;
        tokenize(state);
        
        double right = parse_factor(state);
        if (state->has_error) return 0.0;
        
        if (op == TOK_PLUS) {
            result += right;
        } else {
            result -= right;
        }
    }
    
    return result;
}

// Parse expression
double parse_expression(ParserState *state) {
    return parse_term(state);
}

// Simple main function for testing
int main() {
    char input[256];
    printf("Enter expression: ");
    
    if (fgets(input, sizeof(input), stdin)) {
        // Remove newline - FIXED: use strcspn
        input[strcspn(input, "\n")] = 0;
        
        ParserState state;
        init_parser(&state, input, strlen(input));  // FIXED: init_parser not init parser
        double result = parse_expression(&state);
        
        if (!state.has_error && state.current_token.type == TOK_EOF) {
            printf("Result: %.2f\n", result);
        } else {
            printf("ERROR at position %d\n", state.error_pos);
        }
    }
    
    return 0;
}