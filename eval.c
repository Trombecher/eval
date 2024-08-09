#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef enum TokenType {
    TT_EOI = 0,
    TT_NUMBER = 1,
    TT_PLUS = 2,
    TT_MINUS = 3,
    TT_STAR = 4,
    TT_SLASH = 5,
    TT_PERCENT = 6,
    TT_STAR_STAR = 7
} TokenType;

typedef struct Token {
    // The type of the token.
    TokenType ty;

    // If ty == TT_NUMBER, this field contains a number
    // and therefore is safe to access.
    double maybe_number;
} Token;

#define advance() *next_ptr += 1

// Advances the next_ptr and writes the token into the token_ptr.
void next_token(char const** const next_ptr, Token* const token_ptr) {
    char next;

    while(1) {
        next = **next_ptr;

        switch(next) {
            case 0:
                token_ptr->ty = TT_EOI;
                return;
            case ' ': // Skip whitespace
            case '\n':
            case '\t':
            case '\f':
            case '\r':
                advance();
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                token_ptr->ty = TT_NUMBER;
                double num = (double) (next - '0');

                advance();
                while(1) {
                    next = **next_ptr;

                    switch(next) {
                        case '0':
                        case '1':
                        case '2':
                        case '3':
                        case '4':
                        case '5':
                        case '6':
                        case '7':
                        case '8':
                        case '9':
                            num *= 10;
                            num += (double) (next - '0');
                            advance();
                            continue;
                        default:
                            break;
                    }

                    break;
                }
                
                token_ptr->maybe_number = num;
                return;
            case '+':
                advance();
                token_ptr->ty = TT_PLUS;
                return;
            case '-':
                advance();
                token_ptr->ty = TT_MINUS;
                return;
            case '*':
                advance();
                if(**next_ptr == '*') {
                    token_ptr->ty = TT_STAR_STAR;
                    advance();
                } else {
                    token_ptr->ty = TT_STAR;
                }
                return;
            case '%':
                advance();
                token_ptr->ty = TT_PERCENT;
                return;
            case '/':
                advance();
                token_ptr->ty = TT_SLASH;
                return;
            default:
                printf("Invalid character '%c'\n", next);
                exit(-1);
        }
    }
}

// 2 + 23 * 34 / 34 - 4 ** 2

typedef enum Operator {
    OP_ADD = 2,
    OP_SUB = 3,
    OP_MUL = 4,
    OP_DIV = 5,
    OP_REM = 6,
    OP_EXP = 7
} Operator;

const char BP_VALUES[16] = {
    // Padding to allow for fast indexing
    // via the value of the operator
    0, 0,
    0, 0,

    // Usable values
    20, 21,
    20, 21,
    22, 23,
    22, 23,
    22, 23,
    24, 25,
};

static inline char get_left_bp(Operator const op) {
    return BP_VALUES[op << 2];
}

static inline char get_right_bp(Operator const op) {
    return BP_VALUES[(op << 2) + 1];
}

// Forward declare `Expression` struct.
typedef struct Expression Expression;

typedef struct BinaryExpression {
    Expression* left;
    Expression* right;
} BinaryExpression;

// This must be a constant expr and the compiler
// is not smart enough to accept a plain const char :(
#define ET_NUMBER 1

typedef struct Expression {
    // This field can be either an operator
    // or ET_NUMBER.
    char op_or_ty;

    // If the field `op_or_ty` is an operator,
    // the `expr` field is valid to access;
    // otherwise (if ET_NUMBER) `number` is safe to access.
    union {
        double number;
        BinaryExpression expr;
    } value;
} Expression;

// Given a token iterator, consisting of the `next` pointer
// and the `token` out pointer, it builds an AST
// and returns an owned pointer to the root expression.
//
// The caller is responsible for freeing the AST.
Expression* parse_expression(
    char const** next,
    Token* const token,
    char const min_bp
) {
    next_token(next, token);

    Expression* left_side;

    switch(token->ty) {
        case TT_NUMBER:
            left_side = malloc(sizeof(Expression));
            left_side->op_or_ty = ET_NUMBER;
            left_side->value.number = token->maybe_number;
            break;
        default:
            printf("invalid token with id = %d", token->ty);
            exit(-1);
    }

    while(1) {
        next_token(next, token);

        switch(token->ty) {
            case TT_EOI:
                return left_side;
            case TT_PLUS:
            case TT_MINUS:
            case TT_STAR:
            case TT_SLASH:
            case TT_PERCENT:
            case TT_STAR_STAR:
                Operator const op = (Operator) token->ty;

                if(get_left_bp(op) < min_bp) return left_side;

                Expression* const right = parse_expression(
                    next,
                    token,
                    get_right_bp(op)
                );

                Expression* left = left_side;
                left_side = malloc(sizeof(Expression));
                left_side->op_or_ty = op;
                left_side->value.expr.left = left;
                left_side->value.expr.right = right;
                break;
            default:
                printf("%d not impl", token->ty);
                exit(-1);
        }
    }
}

// Frees the AST recursively.
void free_ast(Expression const* const expr) {
    // Free children
    switch(expr->op_or_ty) {
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_REM:
            BinaryExpression const e = expr->value.expr;
            free_ast(e.left);
            free_ast(e.right);
            break;
        default:
            break;
    }

    // Free pointer
    free((void*) expr);
}

double eval(Expression const* const expr) {
    // SAFETY: THIS IS ONLY SAFE TO ACCESS IF AN OPERATOR WAS FOUND.
    BinaryExpression const e = expr->value.expr;

    switch(expr->op_or_ty) {
        case ET_NUMBER: // TODO: fix this
            return expr->value.number;
        case OP_ADD:
            return eval(e.left) + eval(e.right);
        case OP_SUB:
            return eval(e.left) - eval(e.right);
        case OP_MUL:
            return eval(e.left) * eval(e.right);
        case OP_DIV:
            return eval(e.left) / eval(e.right);
        case OP_REM:
            // Truncate operands to u64s and then perform modulus.
            return (double) (((long long) eval(e.left)) % ((long long) eval(e.right)));
        case OP_EXP:
            return pow(eval(e.left), eval(e.right));
        default:
            printf("not impl");
            exit(-1);
    }
}

int main(int const arg_count, char const* const* const args) {
    if(arg_count < 2) {
        printf("Syntax: eval \"<expr>\"\n");
        exit(-1);
    }

    char const* next = args[1];
    Token token;

    Expression* const expr = parse_expression(&next, &token, 0);
    printf("%f\n", eval(expr));
    free_ast(expr);
}