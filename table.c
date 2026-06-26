#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "object.h"
#include "table.h"
#include "value.h"

#define TABLE_MAX_LOAD 0.75

void initTable(Table* table)
{
    table -> count = 0;
    table -> capacity = 0;
    table -> entries = NULL;
}

void freeTable(Table* table)
{
    FREE_ARRAY(Entry, table -> entries, table -> capacity);
    initTable(table);
}

// Finds an empty bucket in the hash table, according to the linear probing algorithm and a given key to search for.
static Entry* findEntry(Entry* entries, int capacity, ObjString* key)
{
    // Maps the string's hash code to a starting index within the allocated array bounds.
    uint32_t index = key -> hash & (capacity - 1);

    // Track the first tombstone we encounter so we can reuse it for insertion later.
    Entry* tombstone = NULL;

    // Loops continuously until a matching key or an available empty slot is discovered.
    for (;;)
    {
        // Gets a reference to the specific bucket located at the current index.
        Entry* entry = &entries[index];

        // Checks if the bucket matches the target key or if it represents an empty slot.
        if (entry -> key == NULL)
        {
            // If the value is nil, this slot is completely pristine and terminates the lookup chain.
            if (IS_NIL(entry -> value))
            {
                // Return the cached tombstone to recycle its space, otherwise return this empty bucket.
                return tombstone != NULL ? tombstone : entry;
            }
            else
            {
                // A non-nil value means this is a tombstone; cache it if it's the first one we've hit.
                if (tombstone == NULL) tombstone = entry;
            }
        }
        // If the bucket's key pointer matches our search key, the target entry has been located.
        else if (entry -> key == key)
        {
            // Return the matching entry immediately for reading or writing.
            return entry;
        }

        // Advances to the next bucket using linear probing and wraps around at the array boundary.
        index = (index + 1) & (capacity - 1);
    }
}

bool tableGet(Table* table, ObjString* key, Value* value)
{
    if (table -> count == 0) return false;

    Entry* entry = findEntry(table -> entries, table -> capacity, key);
    if (entry -> key == NULL) return false;

    *value = entry -> value;
    return true;
}

// Allocates a new array of buckets for the hash table, initializes them to empty,
// and updates the table's capacity and array pointers.
static void adjustCapacity(Table* table, int capacity)
{
    // Allocates a fresh contiguous block of entry buckets on the heap with the new capacity.
    Entry* entries = ALLOCATE(Entry, capacity);

    // Loops through every bucket in the newly allocated array to clear old data leftovers.
    for (int i = 0; i < capacity; i++)
    {
        // Sets each bucket's key to null to mark it as completely unused and empty.
        entries[i].key = NULL;

        // Initializes each bucket's value to nil to ensure the slot is pristine.
        entries[i].value = NIL_VAL;
    }


    // Rebuild the hash table from the old one, to ensure that already inserted items do not change their location when the hash table reaches capacity and grows.
    table -> count = 0;
    for (int i = 0; i < table -> capacity; i++)
    {
        Entry* entry = &table -> entries[i];
        if (entry -> key == NULL) continue;

        Entry* dest = findEntry(entries, capacity, entry -> key);
        dest -> key = entry -> key;
        dest -> value = entry -> value;
        table -> count++;
    }

    // Swaps out the old entries array pointer for the newly prepared block of buckets. Ensuring to free the old table.
    FREE_ARRAY(Entry, table -> entries, table -> capacity);
    table -> entries = entries;
    // Updates the table's metadata to reflect its expanded internal storage size.
    table -> capacity = capacity;
}

bool tableSet(Table* table, ObjString* key, Value value)
{
    // If the hash table has exceeded the maximum size for an acceptable load factor.
    if (table -> count + 1 > table -> capacity * TABLE_MAX_LOAD)
    {
        int capacity = GROW_CAPACITY(table -> capacity);
        adjustCapacity(table, capacity);
    }

    // Locates the existing bucket for the key or the closest available slot using linear probing.
    Entry* entry = findEntry(table -> entries, table -> capacity, key);

    // Checks if the key is brand new to the table by seeing if the bucket is empty.
    bool isNewKey = entry -> key == NULL;

    // Increments the total count of active elements if a new key is being added.
    if (isNewKey && IS_NIL(entry -> value)) table -> count++;

    // Stores the string key and its associated Lox value into the target bucket.
    entry -> key = key;
    entry -> value = value;

    // Returns true if a new key was created, or false if an existing key's value was overwritten.
    return isNewKey;
}

bool tableDelete(Table* table, ObjString* key)
{
    if (table -> count == 0) return false;

    // Find the entry.
    Entry* entry = findEntry(table -> entries, table -> capacity, key);
    if (entry -> key == NULL) return false;

    // Place a tombstone in the entry.
    entry -> key = NULL;
    entry -> value = BOOL_VAL(true);
    return true;
}

void tableAddAll(Table* from, Table* to)
{
    for (int i = 0; i < from -> capacity; i++)
    {
        Entry* entry = &from -> entries[i];
        if (entry -> key != NULL)
        {
            tableSet(to, entry -> key, entry -> value);
        }
    }
}

ObjString* tableFindString(Table* table, const char* chars, int length, uint32_t hash)
{
    if (table -> count == 0) return NULL;

    uint32_t index = hash & (table -> capacity - 1);
    for (;;)
    {
        Entry* entry = &table -> entries[index];
        if (entry -> key == NULL)
        {
            // Stop if we find an empty non-tombstone entry.
            if (IS_NIL(entry -> value)) return NULL;
        }
        else if (entry -> key -> length == length && entry -> key -> hash == hash && memcmp(entry -> key -> chars, chars, length) == 0)
        {
            return entry -> key;
        }
        index = (index + 1) & (table -> capacity - 1);
    }
}

void tableRemoveWhite(Table* table)
{
    for (int i = 0; i < table -> capacity; i++)
    {
        Entry* entry = &table -> entries[i];
        if (entry -> key != NULL && !entry -> key -> obj.isMarked)
        {
            tableDelete(table, entry -> key);
        }
    }
}

void markTable(Table* table)
{
    for (int i = 0; i < table -> capacity; i++)
    {
        Entry* entry = &table -> entries[i];
        markObject((Obj*)entry -> key);
        markValue(entry -> value);
    }
}
