#ifndef HASHTABLE_H
#define HASHTABLE_H

#include "cstr.h"
#include "object.h"
#include <stdint.h>

#define REHASH_THRESHOLD 75

typedef enum { TAB_KEY_VAL, TAB_STRINGSET } Table_Type;

typedef struct {
  cstr literal;
  uint64_t hash;
} String_Entry;

typedef struct {
  String_Entry key;
  Object *val;
} Hash_Entry;

// If (size * REHASH_THRESHOLD) / capacity >= REHASH_THRESHOLD, we rehash
typedef struct {
  union {
    Hash_Entry *hash_arr;
    String_Entry *string_arr;
  };

  uint32_t table_size;
  uint32_t capacity;
  Table_Type type;
} Hash_Table;

typedef struct {
  Hash_Table *tables;
  uint32_t cur_scope;
  uint32_t size;
  uint32_t capacity;
} Environment;

uint64_t fnv_64(char *str, uint64_t hval);
int insertIntoTable(
    Hash_Table *hashtable, cstr *buf,
    void *val); // Return 1 if already present else insert and return 0
void *lookupFromTable(Hash_Table *hashtable, cstr *buf);
void printHashtable(Hash_Table *hashtable);

void addScopeToEvmt(Environment *evmt);
void removeScopeFromEvmt(Environment *evmt);
int addObjectInScope(Environment *evmt, cstr *id, Object *obj);
void *lookupHierarchical(Environment *evmt, cstr *buf);
#endif