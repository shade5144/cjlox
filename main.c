#include "arenalloc.h"
#include "eval.h"
#include "expression.h"
#include "hashtable.h"
#include "parser.h"
#include "statement.h"
#include "tokenizer.h"
#include "tokens.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* CLI and linking everything goes here */

/*
TODO:
- During tokenization, comments aren't counted for lines. Is this fine?
- Refactor: A tech debt seems to be the lack of separation between persistent data and intermediate tokens/parsing 
  artifacts. This will need a full lookthrough and refactor
- Figure out why tokenization gets weirded out for identifier; Seems like UB
- Variable scopes
- Right now, environments grow if size == capacity(by 2 times) and shrink if
size == [capacity / 2](gets halved). Only
  upon shrinking are the allocated backing arrays for hashtables freed. This
  behaviour has to be tested appropriately.
- Shrinking of the environment's tables could be fairly aggressive,
considering that deeply nested scopes are an
  improbability and considered bad practice anyways
- Add +=, -=, *= and /=
- Alternate environment implementation
- Functions
- Garbage Collection?
- Make a testing framework and test all cases, especially for errors
- Parsing errors should hard stop execution. Static analysis isn't an option for a tree-walk interpreter probably
*/

Arena *g_exp_arena;
Arena *g_obj_arena;
Arena *g_stmt_arena;
Arena *g_scratch_arena;
Hash_Table *g_string_set;
Environment *g_environment;

char *readFileIntoString(char *filename) {
  char *buffer = 0;
  long length;
  FILE *f = fopen(filename, "rb");

  if (f) {
    fseek(f, 0, SEEK_END);
    length = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = malloc(length + 1);

    if (buffer) {
      fread(buffer, 1, length, f);
      buffer[length] = '\0';
    }
    fclose(f);
  }

  return buffer;
}

int main(int argc, char **argv) {
  char *src;
  int repl = 0;

  if (argc == 1) {
    src = calloc(1, 4096);
    repl = 1;
  } else {
    src = readFileIntoString(argv[1]);
  }

  Arena scratch_arena;
  arenaInit(&scratch_arena, 1);

  g_scratch_arena = &scratch_arena;

  Arena token_arena;
  arenaInit(&token_arena, 8); // Maximum of 32 million tokens

  Token_Vector token_list;

  token_list.capacity = 32;
  token_list.tokens = (Token **)(malloc(sizeof(Token *) * token_list.capacity));
  token_list.length = 0;

  Hash_Table string_tab;
  string_tab.type = TAB_STRINGSET;
  string_tab.capacity = 8;
  string_tab.string_arr =
      (String_Entry *)calloc(string_tab.capacity, sizeof(String_Entry));
  string_tab.table_size = 0;

  g_string_set = &string_tab;

  Arena evmt_arena;
  arenaInit(&evmt_arena, 1);

  // Calloc sets the type of HashTable to TAB_KEY_VAL(zero)
  Environment evmt;
  g_environment = &evmt;
  g_environment->tables = (Hash_Table *)calloc(8, sizeof(Hash_Table));
  g_environment->capacity = 8;
  g_environment->size = 0;
  g_environment->cur_scope = 0;

  // Initialize global scope
  g_environment->tables[0].hash_arr =
      (Hash_Entry *)(calloc(8, sizeof(Hash_Entry)));
  g_environment->tables[0].type = TAB_KEY_VAL;
  g_environment->tables[0].capacity = 8;
  g_environment->tables[0].table_size = 0;

  cstr test;
  test.length = strlen(src);
  test.data = src;

  if (repl) {
    printf("Welcome to the lox REPL!\n");
    printf("Type your code along as many lines as necessary\n");
    printf("Press enter on an empty line to execute your code\n");
  }

  char *reset = src;

  do {
    int lines = 0;
    int length = 0;

    if(repl) {
      if (!lines) {
        printf("\033[1;34m>>> \033[0m");
      } else {
        printf("\033[1;33m... \033[0m");
      }

      if (!fgets(src, 4096, stdin)) 
      {
        printf("\033[1;31mREPL Error: Bad Input\n");
        return 0;
      }

      if(src[0] == '\n')
      {
        break;
      }

      if((src - reset) >= 4095)
      {
        printf("\033[1;31mREPL Error: Input Length Exceeded\n");
        return 0;
      }
    
      length += strlen(src);
      src += (long)(strlen(src));
      lines++;

      test.length = length - 1;
      reset[length - 1] = '\0';
      printf("%s\n", reset);
    }

    if(!tokenizeText(&test, &token_list, &scratch_arena, &token_arena))
    {
        src = reset;
        token_list.length = 0;

        arenaFreeAll(&token_arena);
        arenaFreeAll(&scratch_arena);
        continue;
    }

    if(repl && token_list.tokens[0]->tok_type == TOK_QUIT)        
    {
        printf("Quitting REPL...\n");
        return 0;
    }

    printTokList(&token_list);

    if(argc != 1) 
    {
        free(src);
    }

    Parser parser;

    parser.tok_list = &token_list;
    parser.index = 0;

    Arena exp_arena;
    g_exp_arena = &exp_arena;
    arenaInit(g_exp_arena, 8);

    Arena obj_arena;
    g_obj_arena = &obj_arena;
    arenaInit(g_obj_arena, 8);

    Arena stmt_arena;
    g_stmt_arena = &stmt_arena;
    arenaInit(g_stmt_arena, 8); 

    Statement *root = stmt(&parser);

    /* Parse statements into an AST */
    while(stmt(&parser) != NULL)
      ;

    while(root != NULL)
    {
        root = evalStatement(root);
    }

    src = reset;

    token_list.length = 0;

    /* This preserves variable state for REPL */
    if (!repl)
    {
      arenaFreeAll(&token_arena);
      arenaFreeAll(&scratch_arena);
      arenaFreeAll(g_exp_arena);
      arenaFreeAll(g_stmt_arena);
    }

    } while(repl);

}
