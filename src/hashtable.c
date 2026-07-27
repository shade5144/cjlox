#include "hashtable.h"
#include "object.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FNV_64_PRIME ((uint64_t)0x100000001b3ULL)

#define FNV64(string) fnv_64(string, (uint64_t)(0x84222325cbf29ce4))

extern Arena *g_scratch_arena;

uint64_t fnv_64(char *str, uint64_t hval) {
  unsigned char *s = (unsigned char *)str; /* unsigned string */

  while (*s) {
    hval ^= (uint64_t)*s++;
    hval *= FNV_64_PRIME;
  }

  return hval;
}

int insertWithLinearProbe(Hash_Table *hashtable, cstr *buf, void *val) {
  uint64_t buf_hash = FNV64(buf->data);
  uint64_t index = buf_hash & (hashtable->capacity - 1);

  int inserted = 0;

  while (!inserted) {
    char *entry;

    if (hashtable->type == TAB_KEY_VAL) {
      entry = hashtable->hash_arr[index].key.literal.data;
    } else {
      entry = hashtable->string_arr[index].literal.data;
    }

    if (entry == NULL) {
      if (hashtable->type == TAB_KEY_VAL) {
        hashtable->hash_arr[index].key.literal.data = buf->data;
        hashtable->hash_arr[index].key.literal.length = buf->length;
        hashtable->hash_arr[index].key.hash = buf_hash;
        hashtable->hash_arr[index].val = val;
      } else {
        hashtable->string_arr[index].literal.data = buf->data;
        hashtable->string_arr[index].literal.length = buf->length;
        hashtable->string_arr[index].hash = buf_hash;
      }

      inserted = 1;
    } else {
      if (strcmp(buf->data, entry) != 0) {
        index = (index + 1) & (hashtable->capacity -
                               1); // Assume capacity is always a power of 2
      } else {
        // Value already present. Do nothing
        return 0;
      }
    }
  }

  return 1;
}

int insertIntoTable(Hash_Table *hashtable, cstr *buf, void *val) {
  if (insertWithLinearProbe(hashtable, buf, val)) {
    hashtable->table_size += 1;

    // Rehashing logic goes here
    uint32_t fullness_percent =
        (hashtable->table_size * 100) / hashtable->capacity;

    if (fullness_percent >= REHASH_THRESHOLD) {
      // For now malloc new array. Figure out something else if needed later
      uint32_t old_capacity = hashtable->capacity;
      hashtable->capacity *= 2;

      size_t entry_size = sizeof(String_Entry);

      if (hashtable->type == TAB_KEY_VAL) {
        entry_size = sizeof(Hash_Entry);
      }

      char *new_arr = calloc(entry_size, hashtable->capacity);

      if (hashtable->type == TAB_KEY_VAL) {
        Hash_Entry *old_arr = hashtable->hash_arr;

        hashtable->hash_arr = (Hash_Entry *)new_arr;

        for (uint32_t i = 0; i < old_capacity; i++) {
          if (old_arr[i].key.literal.data != NULL) {
            insertWithLinearProbe(hashtable, &old_arr[i].key.literal,
                                  old_arr[i].val);
          }
        }

        free(old_arr);
      } else {
        String_Entry *old_arr = hashtable->string_arr;

        hashtable->string_arr = (String_Entry *)new_arr;

        for (uint32_t i = 0; i < old_capacity; i++) {
          if (old_arr[i].literal.data != NULL) {
            insertWithLinearProbe(hashtable, &old_arr[i].literal, NULL);
          }
        }

        free(old_arr);
      }
    }

    return 1;
  }

  return 0;
}

void *lookupFromTable(Hash_Table *hashtable, cstr *buf) {
  uint64_t buf_hash = FNV64(buf->data);
  uint64_t index = buf_hash & (hashtable->capacity - 1);

  if (hashtable->type == TAB_KEY_VAL) {
    while (1) {
      if ((hashtable->hash_arr[index].key.literal.data == NULL) ||
          buf->data == hashtable->hash_arr[index].key.literal.data) {
        /* We don't return NULL here in the failure case, so the key.literal.data has to be checked against NULL */
        return &hashtable->hash_arr[index];
      }

      index = (index + 1) & (hashtable->capacity -
                             1); // Assume capacity is always a power of 2
    }
  } else {
    while (1) {
      String_Entry *entry = &hashtable->string_arr[index];

      if (entry->literal.data == NULL) {
        return NULL;
      }

      if (entry->literal.length == buf->length && entry->hash == buf_hash &&
          !memcmp(entry->literal.data, buf->data, buf->length)) {
        return entry->literal.data;
      }

      index = (index + 1) & (hashtable->capacity -
                             1); // Assume capacity is always a power of 2
    }
  }
}

void printHashtable(Hash_Table *hashtable) {
  if (hashtable->type == TAB_KEY_VAL) {
    for (uint32_t i = 0; i < hashtable->capacity; i++) {
      if (hashtable->hash_arr[i].key.literal.data == NULL) {
        printf("(NULL)\n");
      } else {
        // Need to add a formatObject here
        printf("(%s %ld %016lx-> ",
               hashtable->hash_arr[i].key.literal.data,
               hashtable->hash_arr[i].key.literal.length,
               hashtable->hash_arr[i].key.hash);
        formatObject(hashtable->hash_arr[i].val);
        printf(")\n");
      }
    }
  } else {
    for (uint32_t i = 0; i < hashtable->capacity; i++) {
      if (hashtable->string_arr[i].literal.data == NULL) {
        printf("(NULL)\n");
      } else {
        printf("(%s %ld %016lx)\n", hashtable->string_arr[i].literal.data,
               hashtable->string_arr[i].literal.length,
               hashtable->string_arr[i].hash);
      }
    }
  }
}

void addScopeToEvmt(Environment *evmt) {
  evmt->size += 1;

  if (evmt->size == evmt->capacity) {
    evmt->capacity *= 2;

    evmt->tables = (Hash_Table *)realloc(evmt->tables,
                                         sizeof(Hash_Table) * evmt->capacity);
  }

  int i = evmt->size;

  if (evmt->tables[i].hash_arr != NULL) {
    return;
  }

  evmt->tables[i].hash_arr = (Hash_Entry *)calloc(4, sizeof(Hash_Entry));
  evmt->tables[i].capacity = 4;
  evmt->tables[i].table_size = 4;
}

void removeScopeFromEvmt(Environment *evmt) {
  evmt->size -= 1;

  if (evmt->size == (evmt->capacity / 2)) {
    for (uint32_t i = evmt->size; i < evmt->capacity; i++) {
      free(evmt->tables[i].hash_arr);
      evmt->tables[i].hash_arr = NULL;
    }

    evmt->capacity /= 2;

    evmt->tables = (Hash_Table *)realloc(evmt->tables,
                                         sizeof(Hash_Table) * evmt->capacity);
  }
}

int addObjectInScope(Environment *evmt, cstr *id, Object *obj) {
  Hash_Table *cur_table = &evmt->tables[evmt->cur_scope];

  return insertIntoTable(cur_table, id, obj);
}