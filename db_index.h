#pragma once

//	Index data structure after DbArena object

typedef struct {
	uint64_t numEntries[1];	// number of keys in index
} DbIndex;

// database cursor handle extension to index

typedef enum {
	CursorNone,
	CursorLeftEof,
	CursorRightEof,
	CursorPosBefore,	// cursor is before a key
	CursorPosAt			// cursor is at a key
} PosState;

//	DbCursor handle extension

typedef struct {
	void *key;			// cursor key bytes
	uint32_t xtra;		// size of user data
	uint32_t keyLen;	// cursor key length
	PosState state:8;	// cursor position state enum
	uint8_t foundKey;	// cursor position found the key
	uint8_t binaryFlds;	// index keys have fields
	uint8_t deDup;		// cursor will deDuplicate result set
} DbCursor;

#define dbindex(map) ((DbIndex *)(map->arena + 1))

uint64_t dbAllocDocStore(Handle *docHndl, uint32_t amt, bool zeroit);
DbStatus dbCloseCursor(DbCursor *cursor, DbMap *map);
DbStatus dbInstallIndexes(Handle *docHndl);

DbStatus dbInsertKey (Handle *idxHndl, void *keyBytes, uint32_t keyLen);

DbStatus dbFindKey(DbCursor *cursor, DbMap *map, void *key, uint32_t keyLen, CursorOp op);
DbStatus dbNextKey(DbCursor *cursor, DbMap *map);
DbStatus dbPrevKey(DbCursor *cursor, DbMap *map);
DbStatus dbRightKey(DbCursor *cursor, DbMap *map);
DbStatus dbLeftKey(DbCursor *cursor, DbMap *map);
DbStatus dbCloseCursor(DbCursor *cursor, DbMap *map);
