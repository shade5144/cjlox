#include "tokenizer.h"
#include "arenalloc.h"
#include "cstr.h"
#include "hashtable.h"
#include "loxerror.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEM_ALIGNMENT 8

extern Hash_Table *g_string_set;

// TODO:
// - Handle unexpected characters when tokenizing

Token_Type binarySearchKeyword(char *string, char *keywords[NUM_KEYWORDS]) {
  long long start = 0; // Using native word size equals more speed..?
  long long end = NUM_KEYWORDS - 1;
  long long mid;
  int comp;

  while (start <= end) {
    mid = (start + end) / 2;

    comp = strcmp(keywords[mid], string);

    if (comp == 0) {
      return mid;
    } else if (comp < 0) {
      start = mid + 1;
    } else {
      end = mid - 1;
    }
  }

  return -1;
}

Token *createToken(Token_Type type, cstr *tok_literal, Arena *tok_arena) {

  Token *new = (Token *)arenaAlloc(tok_arena, sizeof(Token), MEM_ALIGNMENT);

  new->tok_type = type;

  if (tok_literal != NULL) {
    char *buf;

    buf = (char *)lookupFromTable(g_string_set, tok_literal);

    if (buf) {
      new->tok_string.data = buf;
      new->tok_string.length = tok_literal->length;

      return new;
    }

    new->tok_string.data = (char *)arenaAlloc(
        tok_arena, sizeof(char) * tok_literal->length + 1, MEM_ALIGNMENT);
    memcpy(new->tok_string.data, tok_literal->data, tok_literal->length);

    new->tok_string.data[tok_literal->length] = '\0';
    new->tok_string.length = tok_literal->length;

    insertIntoTable(g_string_set, &new->tok_string, NULL);
  }

  return new;
}

// If I am using an arena's store of memory here, because in Linux demand paging
// is done, I don't need any sort of reallocation logic. But this might be
// flawed, so we shall get back to this later
void pushTokenVector(Token_Vector *vec, Token *tok) {
  if (vec->length == vec->capacity) {
    vec->capacity *= 2;

    vec->tokens =
        (Token **)realloc(vec->tokens, sizeof(Token *) * vec->capacity);
  }

  int i = vec->length;

  vec->tokens[i] = tok;

  vec->length += 1;
}

char nextChar(cstr *line, size_t index) {
  if ((index + 1) >= line->length) {
    return '\0';
  }

  return line->data[index + 1];
}

int isDigit(char c) { return (c >= '0') && (c <= '9'); }

int isAlpha(char c) {
  return (c == '_') || ((c >= 'A') && (c <= 'Z')) || ((c >= 'a' && c <= 'z'));
}

int tokenizeText(cstr *text, Token_Vector *tok_list, Arena *scratch_arena,
                 Arena *token_arena) {
  char *keywords[NUM_KEYWORDS] = {
      "and",   "class", "else",   "false", "for",  "fun",  "if",  "nil",  "or",
      "print", "quit",  "return", "super", "this", "true", "var", "while"};

  cstr tok_string_buf;

  char *string_obj_buf;
  int string_obj_buf_ind = 0;

  size_t i = 0;
  char c;
  char next_char;

  int cur_line = 0;
  int cur_line_start = 0;
  int cur_tok_ind = 0;

  int string_line = 0;
  int string_ind = 0;

  Token_Type tok_type_buf;

  while ((c = text->data[i]) != '\0') {
    // printf("Vector: \n");
    // for(int i = 0; i < tok_list->length; i++)
    // {
    //     printf("%d %p\n", i, tok_list->tokens[i]);
    // }
    tok_string_buf.length = 0;
    tok_string_buf.data = (char *)arenaAlloc(scratch_arena, 32, MEM_ALIGNMENT);

    switch (c) {
    case '(': {
      tok_type_buf = TOK_LEFT_PAREN;
      strcpy(tok_string_buf.data, "(");
      tok_string_buf.length = 2;
      break;
    }
    case ')': {
      tok_type_buf = TOK_RIGHT_PAREN;
      strcpy(tok_string_buf.data, ")");
      tok_string_buf.length = 2;
      break;
    }
    case '{': {
      tok_type_buf = TOK_LEFT_BRACE;
      strcpy(tok_string_buf.data, "{");
      tok_string_buf.length = 2;
      break;
    }

    case '}': {
      tok_type_buf = TOK_RIGHT_BRACE;
      strcpy(tok_string_buf.data, "}");
      tok_string_buf.length = 2;
      break;
    }
    case ',': {
      tok_type_buf = TOK_COMMA;
      strcpy(tok_string_buf.data, ",");
      tok_string_buf.length = 2;
      break;
    }

    case '.': {
      tok_type_buf = TOK_DOT;
      strcpy(tok_string_buf.data, ".");
      tok_string_buf.length = 2;
      break;
    }

    case '-': {
      tok_type_buf = TOK_MINUS;
      strcpy(tok_string_buf.data, "-");
      tok_string_buf.length = 2;
      break;
    }

    case '+': {
      tok_type_buf = TOK_PLUS;
      strcpy(tok_string_buf.data, "+");
      tok_string_buf.length = 2;
      break;
    }
    case ';': {
      tok_type_buf = TOK_SEMICOLON;
      strcpy(tok_string_buf.data, ";");
      tok_string_buf.length = 2;
      break;
    }

    case '*': {
      tok_type_buf = TOK_STAR;
      strcpy(tok_string_buf.data, "*");
      tok_string_buf.length = 2;
      break;
    }

    case '/': {
      next_char = nextChar(text, i);

      if (next_char == '/') {
        // Add code to skip iteration here
        while (text->data[i] != '\n' && text->data[i] != '\0') {
          i++;
        }

        i++;
        continue;
      } else {
        tok_type_buf = TOK_SLASH;
        strcpy(tok_string_buf.data, "/");
        tok_string_buf.length = 2;
      }
      break;
    }

    case '!': {
      next_char = nextChar(text, i);

      if (next_char == '=') {

        tok_type_buf = TOK_BANG_EQUAL;
        strcpy(tok_string_buf.data, "!=");
        tok_string_buf.length = 3;
        i++;
      } else {
        tok_type_buf = TOK_BANG;
        strcpy(tok_string_buf.data, "!");
        tok_string_buf.length = 2;
      }

      break;
    }

    case '=': {

      next_char = nextChar(text, i);

      if (next_char == '=') {
        tok_type_buf = TOK_EQUAL_EQUAL;
        strcpy(tok_string_buf.data, "==");
        tok_string_buf.length = 3;
        i++;
      } else {
        tok_type_buf = TOK_EQUAL;
        strcpy(tok_string_buf.data, "=");
        tok_string_buf.length = 2;
      }

      break;
    }

    case '>': {
      next_char = nextChar(text, i);

      if (next_char == '=') {
        tok_type_buf = TOK_GREATER_EQUAL;
        strcpy(tok_string_buf.data, ">=");
        tok_string_buf.length = 3;
        i++;
      } else {
        tok_type_buf = TOK_GREATER;
        strcpy(tok_string_buf.data, ">");
        tok_string_buf.length = 2;
      }

      break;
    }

    case '<': {
      next_char = nextChar(text, i);

      if (next_char == '=') {
        tok_type_buf = TOK_LESS_EQUAL;
        strcpy(tok_string_buf.data, "<=");
        tok_string_buf.length = 3;
        i++;
      } else {
        tok_type_buf = TOK_LESS;
        strcpy(tok_string_buf.data, "<");
        tok_string_buf.length = 2;
      }

      break;
    }

    case '\n': {
      cur_line++;
      cur_line_start = (i + 1);
    }
    case ' ':
    case '\t': {
      if (i - cur_line_start > 0) {
        cur_tok_ind = i - cur_line_start + 1;
      }

      i++;
      continue;
    }

    case '"': {
      tok_type_buf = TOK_STRING;

      i++;

      string_obj_buf = (char *)arenaAlloc(scratch_arena, 4096, MEM_ALIGNMENT);
      string_obj_buf_ind = 0;

      string_line = cur_line;
      string_ind = cur_tok_ind;

      while (text->data[i] != '\0' && text->data[i] != '"') {
        string_obj_buf[string_obj_buf_ind] = text->data[i];

        if (text->data[i] == '\n') {
          cur_line++;
          cur_line_start = (i + 1);
        }

        string_obj_buf_ind++;
        i++;
      }

      if (text->data[i] == '\0') {
        formatError("String literal not terminated", string_line, string_ind);
        return 0;
      }

      if (i - cur_line_start > 0) {
        cur_tok_ind = i - cur_line_start + 1;
      }

      string_obj_buf[string_obj_buf_ind] = '\0';

      tok_string_buf.data = string_obj_buf;
      tok_string_buf.length = string_obj_buf_ind;

      break;
    }

    default: {
      if (isDigit(c)) {
        tok_type_buf = TOK_NUMBER;

        string_obj_buf = arenaAlloc(scratch_arena, 128, MEM_ALIGNMENT);
        size_t number_obj_ind = 0;

        while (text->data[i] != '\0' &&
               (isDigit(text->data[i]) || text->data[i] == '.')) {
          string_obj_buf[number_obj_ind] = text->data[i];

          number_obj_ind++;
          i++;
        }

        string_obj_buf[number_obj_ind] = '\0';

        strcpy(tok_string_buf.data, string_obj_buf);
        tok_string_buf.length = number_obj_ind;

        i--;
      }

      else if (isAlpha(c)) {
        tok_type_buf = TOK_IDENTIFIER;

        string_obj_buf = arenaAlloc(scratch_arena, 128, MEM_ALIGNMENT);

        size_t identifier_obj_ind = 0;

        // Add error if identifier starts with digit
        while (text->data[i] != '\0' &&
               (isAlpha(text->data[i]) || isDigit(text->data[i]))) {
          string_obj_buf[identifier_obj_ind] = text->data[i];

          identifier_obj_ind++;
          i++;
        }

        string_obj_buf[identifier_obj_ind] = '\0';

        int ident_or_keyword = binarySearchKeyword(string_obj_buf, keywords);

        if (ident_or_keyword != -1) {
          tok_type_buf = ident_or_keyword;
        }

        strcpy(tok_string_buf.data, string_obj_buf);
        tok_string_buf.length = identifier_obj_ind;

        i--;

        break;
      }

      else {
        // printf("Unexpected character\n"); // Add lox-error handling here
        tok_type_buf = INVALID_TOKEN;
        break;
      }
    }
    }

    // buf.data = arenaAlloc(scratch_arena, sizeof(double), MEM_ALIGNMENT);

    Token *cur_tok = createToken(tok_type_buf, &tok_string_buf, token_arena);

    if (tok_type_buf != TOK_STRING) {
      cur_tok->line = cur_line;
      cur_tok->index = cur_tok_ind;
    } else {
      cur_tok->line = string_line;
      cur_tok->index = string_ind;
    }

    pushTokenVector(tok_list, cur_tok);

    arenaFreeAll(scratch_arena);

    cur_tok_ind = i - cur_line_start + 1;

    i++;
  }

  Token *end_of_file = createToken(TOK_EOF, NULL, token_arena);
  end_of_file->index = cur_tok_ind;
  end_of_file->line = cur_line;
  pushTokenVector(tok_list, end_of_file);

  return 1;
}

void printTokList(Token_Vector *tok_list) {
  static char *token_names[41] = {"TOK_AND",
                           "TOK_CLASS",
                           "TOK_ELSE",
                           "TOK_FALSE",
                           "TOK_FOR",
                           "TOK_FUN",
                           "TOK_IF",
                           "TOK_NIL",
                           "TOK_OR",
                           "TOK_PRINT",
                           "TOK_QUIT",
                           "TOK_RETURN",
                           "TOK_SUPER",
                           "TOK_THIS",
                           "TOK_TRUE",
                           "TOK_VAR",
                           "TOK_WHILE",
                           "TOK_IDENTIFIER",
                           "TOK_STRING",
                           "TOK_NUMBER",
                           "TOK_LEFT_PAREN",
                           "TOK_RIGHT_PAREN",
                           "TOK_LEFT_BRACE",
                           "TOK_RIGHT_BRACE",
                           "TOK_COMMA",
                           "TOK_DOT",
                           "TOK_MINUS",
                           "TOK_PLUS",
                           "TOK_SEMICOLON",
                           "TOK_SLASH",
                           "TOK_STAR",
                           "TOK_BANG",
                           "TOK_BANG_EQUAL",
                           "TOK_EQUAL",
                           "TOK_EQUAL_EQUAL",
                           "TOK_GREATER",
                           "TOK_GREATER_EQUAL",
                           "TOK_LESS",
                           "TOK_LESS_EQUAL",
                           "TOK_EOF",
                           "INVALID_TOKEN"};

  Token *buf;

  for (int i = 0; i < tok_list->length; i++) {
    buf = tok_list->tokens[i];

    printf("%d | ", i);
    if (buf->tok_type != TOK_EOF) {
      printf("Token String: %s Token Type: ", buf->tok_string.data);
      formatTokenType(buf->tok_type, token_names);
      printf("at %d:%d\n", buf->line, buf->index);
    } else {
      printf("Token String: EOF Token Type: TOK_EOF at %d:%d\n", buf->line,
             buf->index);
    }
  }
}
