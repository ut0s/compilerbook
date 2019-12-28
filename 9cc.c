#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  TK_RESERVED, // keywords or punctuators
  TK_NUM,      // integer literals
  TK_EOF,      // end-of-file markers
} TokenKind;

// token type
typedef struct Token Token;
struct Token {
  TokenKind kind; // token kind
  Token* next;    // next token
  long val;       // if kind is TK_NUM, its value
  char* str;      // token string
};

// input program
char* user_input;

// current token
Token* token;

// reports an error and exit.
void error(char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// report an error location and exit.
void error_at(char* loc, char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int pos = loc - user_input;
  fprintf(stderr, "%s\n", user_input);
  fprintf(stderr, "%*s", pos, ""); // print pos spaces
  fprintf(stderr, "^ ");
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");
  exit(1);
}

// consumes the current token if it matches `op`.
bool consume(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op) {
    return false;
  }
  token = token->next;
  return true;
}

// ensure that the current token is `op`.
void expect(char op) {
  if (token->kind != TK_RESERVED || token->str[0] != op) {
    error_at(token->str, "expected: '%c'", op);
  }
  token = token->next;
}

// ensure that the current token is TK_NUM.
long expect_number(void) {
  if (token->kind != TK_NUM) {
    error_at(token->str, "expected a number");
  }
  long val = token->val;
  token    = token->next;
  return val;
}

bool at_eof(void) {
  return token->kind == TK_EOF;
}

// create a new token and add it as the next token of `cur`
Token* new_token(TokenKind kind, Token* cur, char* str) {
  Token* tok = calloc(1, sizeof(Token));
  tok->kind  = kind;
  tok->str   = str;
  cur->next  = tok;
  return tok;
}

// tokenize `p` and returns new tokens;
Token* tokenize() {
  char* p    = user_input;
  Token head = {};
  Token* cur = &head;

  while (*p) {
    // skip whitespace characters.
    if (isspace(*p)) {
      p++;
      continue;
    }

    // punctuator
    if (*p == '+' || *p == '-') {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }

    // integer literal
    if (isdigit(*p)) {
      cur      = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    error_at(p, "expected a number");
  }

  new_token(TK_EOF, cur, p);
  return head.next;
}

int main(int argc, char** argv) {
  if (argc != 2) {
    error("%s : invalid of number of arguments", argv[0]);
  }

  user_input = argv[1];
  token      = tokenize();

  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // the first token must be a number.
  printf("  mov rax, %ld\n", expect_number());

  // ... followed by either `+ <number>` or `- <number>`.
  while (!at_eof()) {
    if (consume('+')) {
      printf("  add rax, %ld\n", expect_number());
      continue;
    }

    expect('-');
    printf("  sub rax, %ld\n", expect_number());
  }

  printf("  ret\n");
  return 0;
}
