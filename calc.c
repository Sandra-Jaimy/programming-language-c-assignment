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
    // Handle integer exponents efficiently
    if (exponent == 0.0) return 1.0;
    if (exponent == 1.0) return base;
    if (exponent == 2.0) return base * base;
    if (exponent == 3.0) return base * base * base;
    
    // For negative exponents
    if (exponent < 0) {
        return 1.0 / custom_pow(base, -exponent);
    }
    
    // For fractional exponents, use approximation
    if (exponent != (int)exponent) {
        // Simple approximation using exp and log - but we don't have those either
        // Fall back to iterative method for demonstration
        // In a real implementation, you'd want proper math functions
        double result = 1.0;
        int int_part = (int)exponent;
        double frac_part = exponent - int_part;
        
        result = custom_pow(base, int_part);
        // Approximate fractional power (this is simplified)
        if (frac_part > 0) {
            result *= 1.0 + frac_part * (base - 1.0);
        }
        return result;
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
    tokenize(state);  // Load first token
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
        if (state->input[state->pos] == '\n') {
            state->char_count++;
        } else {
            state->char_count++;
        }
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
    +
    // Check for end of input
    if (state->pos >= state->length) {
        state->current_token.type = TOK_EOF;
        state->current_token.start_pos = state->char_count;
        state->current_token.length = 0;
        return;
    }
    
    char current_char = state->input[state->pos];
    int start_char_count = state->char_count;
    
    // Handle numbers (integers and floats)
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

// Parse primary expressions: numbers or parenthesized expressions
double parse_primary(ParserState *state) {
    if (state->has_error) return 0.0;
    
    if (state->current_token.type == TOK_NUMBER) {
        double value = state->current_token.value;
        tokenize(state);
        return value;
    }
    else if (state->current_token.type == TOK_LPAREN) {
        tokenize(state);  // Consume '('
        double result = parse_expression(state);
        
        if (state->current_token.type != TOK_RPAREN) {
            fail(state, state->current_token.start_pos);
            return 0.0;
        }
        tokenize(state);  // Consume ')'
        return result;
    }
    else {
        fail(state, state->current_token.start_pos);
        return 0.0;
    }
}

// Parse power operator (right-associative)
double parse_power(ParserState *state) {
    if (state->has_error) return 0.0;
    
    double left = parse_primary(state);
    
    while (!state->has_error && state->current_token.type == TOK_POW) {
        int op_pos = state->current_token.start_pos;
        tokenize(state);  // Consume ''
        double right = parse_power(state);  // Right-associative: parse power again
        if (state->has_error) return 0.0;
        
        // Use custom power function instead of pow()
        left = custom_pow(left, right);
        
        // Check for mathematical errors
        if (left != left) {  // Check for NaN
            fail(state, op_pos);
            return 0.0;
        }
    }
    
    return left;
}

// Parse multiplication and division (left-associative)
double parse_factor(ParserState *state) {
    if (state->has_error) return 0.0;
    
    double result = parse_power(state);
    
    while (!state->has_error && 
          (state->current_token.type == TOK_STAR || state->current_token.type == TOK_SLASH)) {
        TokenType op = state->current_token.type;
        int op_pos = state->current_token.start_pos;
        tokenize(state);  // Consume operator
        
        double right = parse_power(state);
        if (state->has_error) return 0.0;
        
        if (op == TOK_STAR) {
            result *= right;
        } else { // TOK_SLASH
            if (right == 0.0) {
                fail(state, op_pos);
                return 0.0;
            }
            result /= right;
        }
    }
    
    return result;
}

// Parse addition and subtraction (left-associative)
double parse_term(ParserState *state) {
    if (state->has_error) return 0.0;
    
    double result = parse_factor(state);
    
    while (!state->has_error && 
          (state->current_token.type == TOK_PLUS || state->current_token.type == TOK_MINUS)) {
        TokenType op = state->current_token.type;
        tokenize(state);  // Consume operator
        
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

// Parse expression (entry point for parsing)
double parse_expression(ParserState *state) {
    return parse_term(state);
}

// Check if line is a comment (starts with # after optional whitespace)
int is_comment_line(const char *line) {
    while (*line && isspace(*line)) {
        line++;
    }
    return *line == '#';
}

// Process a single input file
void process_file(const char *input_path, const char *output_path) {
    FILE *input_file = fopen(input_path, "r");
    if (!input_file) {
        fprintf(stderr, "Error: Cannot open input file %s\n", input_path);
        return;
    }
    
    // Read entire file content
    char *content = NULL;
    size_t content_size = 0;
    size_t content_length = 0;
    
    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), input_file)) {
        // Skip comment lines
        if (is_comment_line(buffer)) {
            continue;
        }
        
        size_t buffer_len = strlen(buffer);
        if (content_length + buffer_len + 1 > content_size) {
            content_size = content_size ? content_size * 2 : 1024;
            char *new_content = realloc(content, content_size);
            if (!new_content) {
                fprintf(stderr, "Error: Memory allocation failed\n");
                free(content);
                fclose(input_file);
                return;
            }
            content = new_content;
        }
        
        memcpy(content + content_length, buffer, buffer_len);
        content_length += buffer_len;
    }
    
    if (content && content_length > 0) {
        content[content_length] = '\0';
    }
    
    fclose(input_file);
    
    // Parse and evaluate
    ParserState state;
    if (content && content_length > 0) {
        init_parser(&state, content, content_length);
        double result = parse_expression(&state);
        
        // Check if we parsed the entire input
        if (!state.has_error && state.current_token.type != TOK_EOF) {
            fail(&state, state.current_token.start_pos);
        }
        
        // Write output
        FILE *output_file = fopen(output_path, "w");
        if (output_file) {
            if (state.has_error) {
                fprintf(output_file, "ERROR:%d\n", state.error_pos);
            } else {
                // Check if result is integer
                long long int_result = (long long)result;
                if (result == (double)int_result && result < 1e18 && result > -1e18) {
                    fprintf(output_file, "%lld\n", int_result);
                } else {
                    fprintf(output_file, "%.15g\n", result);
                }
            }
            fclose(output_file);
        } else {
            fprintf(stderr, "Error: Cannot create output file %s\n", output_path);
        }
    } else {
        // Empty file or only comments
        FILE *output_file = fopen(output_path, "w");
        if (output_file) {
            fprintf(output_file, "ERROR:1\n");
            fclose(output_file);
        }
    }
    
    free(content);
}

// Create directory if it doesn't exist
int ensure_directory(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        // Directory doesn't exist, create it
        #ifdef _WIN32
            return mkdir(path);
        #else
            return mkdir(path, 0775);
        #endif
    }
    return 0;
}

// Extract base filename without extension
void get_basename(const char *filename, char *result, size_t size) {
    const char *base = filename;
    const char *dot = strrchr(filename, '.');
    const char *slash = strrchr(filename, '/');
    
    if (slash) base = slash + 1;
    
    if (dot && dot > base) {
        size_t len = dot - base;
        if (len < size - 1) {
            strncpy(result, base, len);
            result[len] = '\0';
        } else {
            strncpy(result, base, size - 1);
            result[size - 1] = '\0';
        }
    } else {
        strncpy(result, base, size - 1);
        result[size - 1] = '\0';
    }
}

// Main function with command line argument processing
int main(int argc, char *argv[]) {
    char *input_dir = NULL;
    char *output_dir = NULL;
    char *input_file = NULL;
    char default_output_dir[512] = "";
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "--dir") == 0) {
            if (i + 1 < argc) {
                input_dir = argv[++i];
            } else {
                fprintf(stderr, "Error: %s requires a directory argument\n", argv[i]);
                return 1;
            }
        }
        else if (strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "--output-dir") == 0) {
            if (i + 1 < argc) {
                output_dir = argv[++i];
            } else {
                fprintf(stderr, "Error: %s requires a directory argument\n", argv[i]);
                return 1;
            }
        }
        else {
            input_file = argv[i];
        }
    }
    
    if (!input_dir && !input_file) {
        fprintf(stderr, "Usage: %s [-d DIR | --dir DIR] [-o OUTDIR | --output-dir OUTDIR] input.txt\n", argv[0]);
        return 1;
    }
    
    // Handle single file processing
    if (input_file) {
        char base_name[256];
        get_basename(input_file, base_name, sizeof(base_name));
        
        if (!output_dir) {
            // Create default output directory: <input_base>_sandra_241ADB123
            snprintf(default_output_dir, sizeof(default_output_dir), "%s_sandra_241ADB123", base_name);
            output_dir = default_output_dir;
        }
        
        if (ensure_directory(output_dir) != 0 && errno != EEXIST) {
            fprintf(stderr, "Error: Cannot create output directory %s\n", output_dir);
            return 1;
        }
        
        char output_path[512];
        snprintf(output_path, sizeof(output_path), "%s/%.200s_sandra_jaimy_241ADB123.txt", 
                 output_dir, base_name);
        
        process_file(input_file, output_path);
    }
    
    // Handle directory processing (for Grade 9+)
    if (input_dir) {
        DIR *dir = opendir(input_dir);
        if (!dir) {
            fprintf(stderr, "Error: Cannot open directory %s\n", input_dir);
            return 1;
        }
        
        if (!output_dir) {
            // Create default output directory
            snprintf(default_output_dir, sizeof(default_output_dir), "%s_sandra_241ADB123", input_dir);
            output_dir = default_output_dir;
        }
        
        if (ensure_directory(output_dir) != 0 && errno != EEXIST) {
            fprintf(stderr, "Error: Cannot create output directory %s\n", output_dir);
            closedir(dir);
            return 1;
        }
        
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            const char *name = entry->d_name;
            const char *dot = strrchr(name, '.');
            
            // Process only .txt files
            if (dot && strcmp(dot, ".txt") == 0) {
                char input_path[512];
                char output_path[512];
                char base_name[256];
                
                snprintf(input_path, sizeof(input_path), "%s/%s", input_dir, name);
                get_basename(name, base_name, sizeof(base_name));
                
                snprintf(output_path, sizeof(output_path), "%s/%.200s_sandra_jaimy_241ADB123.txt", 
                         output_dir, base_name);
                
                process_file(input_path, output_path);
            }
        }
        
        closedir(dir);
    }
    
    return 0;
}