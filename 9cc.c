#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//
// tokenizer
//

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

// Tokenize `user_input` and returns new tokens.
Token *tokenize(void) {
  char *p = user_input;
  Token head = {};
  Token* cur = &head;

  while (*p) {
    // skip whitespace characters.
    if (isspace(*p)) {
      p++;
      continue;
    }

    // punctuators
    if (ispunct(*p)) {
      cur = new_token(TK_RESERVED, cur, p++);
      continue;
    }

    // integer literal
    if (isdigit(*p)) {
      cur      = new_token(TK_NUM, cur, p);
      cur->val = strtol(p, &p, 10);
      continue;
    }

    error_at(p, "invalid token");
  }

  new_token(TK_EOF, cur, p);
  return head.next;
}

//
// parser
//

typedef enum {
  ND_ADD, // +
  ND_SUB, // -
  ND_MUL, // *
  ND_DIV, // /
  ND_NUM, // integer
} NodeKind;

// AST node type
typedef struct Node Node;
struct Node {
  NodeKind kind; // node kind
  Node* lhs;     // left-hand side
  Node* rhs;     // right-hand side
  long val;      // used if kind == ND_NUM
};

static Node* new_node(NodeKind kind) {
  Node* node = calloc(1, sizeof(Node));
  node->kind = kind;
  return node;
}

static Node* new_binary(NodeKind kind, Node* lhs, Node* rhs) {
  Node* node = new_node(kind);
  node->lhs  = lhs;
  node->rhs  = rhs;
  return node;
}

static Node *new_num(int val) {
  Node *node = new_node(ND_NUM);
  node->val = val;
  return node;
}

static Node* expr(void);
static Node* mul(void);
static Node* primary(void);

// expr = mul ("+" mul | "-" mul)*
static Node* expr(void) {
  Node* node = mul();

  for (;;) {
    if (consume('+')) {
      node = new_binary(ND_ADD, node, mul());
    } else if (consume('-')) {
      node = new_binary(ND_SUB, node, mul());
    } else {
      return node;
    }
  }
}

// mul = primary ("*" primary | "/" primary)*
static Node* mul(void) {
  Node *node = primary();

  for (;;) {
    if (consume('*')) {
      node = new_binary(ND_MUL, node, mul());
    } else if (consume('/')) {
      node = new_binary(ND_DIV, node, mul());
    } else {
      return node;
    }
  }
}

// primary = "(" expr ")" | num
static Node* primary(void) {
  if (consume('(')) {
    Node* node = expr();
    expect(')');
    return node;
  }

  return new_num(expect_number());
}

//
// code generator
//
static void gen(Node* node) {
  if (node->kind == ND_NUM) {
    printf("  push %ld\n", node->val);
    return;
  }

  gen(node->lhs);
  gen(node->rhs);

  printf("  pop rdi\n");
  printf("  pop rax\n");

  switch (node->kind) {
    case ND_ADD:
      printf("  add rax, rdi\n");
      break;
    case ND_SUB:
      printf("  sub rax, rdi\n");
      break;
    case ND_MUL:
      printf("  imul rax, rdi\n");
      break;
    case ND_DIV:
      printf("  cqo\n");
      printf("  idiv rdi\n");
      break;
  }

  printf("  push rax\n");
}

int main(int argc, char** argv) {
  if (argc != 2) {
    error("%s : invalid of number of arguments", argv[0]);
  }

  // tokenize and parse.
  user_input = argv[1];
  token      = tokenize();
  Node* node = expr();

  // Print out the first half of assembly.
  printf(".intel_syntax noprefix\n");
  printf(".global main\n");
  printf("main:\n");

  // traverse the AST to emit assembly.
  gen(node);

  // a result must be at the top of the stack, so pop it to RAX to make it a program exit code.
  printf("  pop rax\n");
  printf("  ret\n");
  return 0;
}
