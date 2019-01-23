#pragma once
#include "../db.h"
#include "../db_object.h"
#include "../db_handle.h"
#include "../db_arena.h"
#include "../db_map.h"
#include "../db_api.h"
#include "../db_cursor.h"
#include "../db_frame.h"
#include "../db_lock.h"

#define Btree2_maxkey		4096	// max key size
#define Btree2_maxskip		16		// max height of skip tower
#define Btree2_maxslots		65536	// max skip entries
#define Btree2_maxbits		29	// maximum page size in bits
#define Btree2_minbits		9	// minimum page size in bits
#define Btree2_minpage		(1 << Btree2_minbits)	// minimum page size
#define Btree2_maxpage		(1 << Btree2_maxbits)	// maximum page size

//	types of btree pages/allocations

typedef enum{
	Btree2_rootPage = 3,
	Btree2_interior,
	Btree2_leafPage,
	MAXBtree2Type
} Btree2PageType;

//	Btree2Index global data on disk

typedef struct {
	DbIndex base[1];
	DbAddr freePage[MAXBtree2Type];	//
	DbAddr pageNos[1];	// frames of recycled pageNo
	uint32_t pageSize;
	uint8_t pageBits;
	uint8_t leafXtra;
	uint8_t skipBits;	// unit size for skip list entries
	ObjId root;
	ObjId left;
	ObjId right;
} Btree2Index;

//	Btree2 page layout

typedef enum {
	Btree2_pageempty,
	Btree2_pageactive,	// page is live
	Btree2_pageclean,	// page being cleaned
	Btree2_pagesplit,	// page being split
} Btree2PageState;

//	This structure is immediately
//	followed by the key slots

typedef struct {
	union Btree2Alloc {
		struct {
			Btree2PageState state : 8;
			uint8_t filler;
			uint16_t nxt;	// next skip list storage unit
		};
		uint8_t bytes[4];
		uint32_t word[1];
	} alloc[1];
	uint16_t garbage[1];	// page garbage in skip units
	uint16_t size, cnt;		// page size in skip units, count of active keys
	uint8_t height;		// height of skip list
	uint8_t lvl;		// level of page
	uint8_t pageBits;
	uint8_t leafXtra;
	uint8_t skipBits;	// unit size for skip list allocations
	uint8_t pageType;	// allocation type
	DbAddr newPage;		// replacement page
	ObjId pageNo;		// page number
	ObjId right;		// page to right
	ObjId left;			// page to left
	uint16_t skipHead[Btree2_maxskip];
} Btree2Page;

//	Slot types

typedef enum {
	Btree2_slotunused,	// slot unused
	Btree2_slotactive,	// slot active
	Btree2_slotmoved,	// slot copied into new page version
	Btree2_slotdeleted,	// slot deleted
} Btree2SlotState;

//	tower values < (sizeof(Page) >> skipBits)
//	are not allocated and are special instead

typedef enum {
	Btree2_towereof,			// not set yet / end-of-list
	Btree2_towerbase			// last special value
} Btree2TowerSlot;

//	Page key slot definition.

typedef struct {
	union {
		Btree2SlotState slotState : 8;
		uint8_t state[1];
	};
	uint8_t lazyHeight[1];	// next tower idx to install
	uint8_t height;			// final tower height
	uint16_t tower[1];		// skip list tower
} Btree2Slot;

typedef struct {
	ObjId pageNo;		// current page Number
	ObjId parent;		// parent page Number
	DbAddr pageAddr;	// current page address
	Btree2Page *page;	// current page
	Btree2Slot *slot;	// height zero slot
	uint8_t lvl;		// level for btree page
	uint8_t height;		// tower height
	uint16_t off;		// offset of current slot
	uint16_t prevSlot[Btree2_maxskip];	// tower next slot offsets
} Btree2Set;

typedef struct {
	DbCursor base[1];	// base object
	Btree2Page *page;	// cursor position page buffer
	DbAddr pageAddr;	// cursor page buffer address
	uint16_t slotOff;	// cursor position offset6+
} Btree2Cursor;

#define btree2index(map) ((Btree2Index *)(map->arena + 1))

DbStatus btree2NewCursor(DbCursor *cursor, DbMap *map);
DbStatus btree2ReturnCursor(DbCursor *dbCursor, DbMap *map);

DbStatus btree2LeftKey(DbCursor *cursor, DbMap *map);
DbStatus btree2RightKey(DbCursor *cursor, DbMap *map);

DbStatus btree2FindKey(DbCursor *dbCursor, DbMap *map, uint8_t *key, uint32_t keylen, bool onlyOne);
DbStatus btree2NextKey (DbCursor *cursor, DbMap *map);
DbStatus btree2PrevKey (DbCursor *cursor, DbMap *map);

DbStatus btree2Init(Handle *hndl, Params *params);
DbStatus btree2InsertKey(Handle *hndl, uint8_t *key, uint32_t keyLen, uint8_t lvl, Btree2SlotState state);
DbStatus btree2DeleteKey(Handle *hndl, uint8_t *key, uint32_t keyLen);
DbStatus btree2LoadPage(Handle *index, Btree2Set *set, uint8_t *key, uint32_t keyLen, uint8_t lvl);

uint64_t btree2AllocPage(Handle *index, int type, uint32_t size);
uint64_t btree2NewPage (Handle *hndl, uint8_t lvl, Btree2PageType type);

DbStatus btree2CleanPage(Handle *hndl, Btree2Set *set);
DbStatus btree2SplitPage (Handle *hndl, Btree2Set *set);
DbStatus btree2FixKey (Handle *hndl, uint8_t *fenceKey, uint8_t lvl);

void btree2PutPageNo(uint8_t *key, uint32_t len, uint64_t bits);
uint64_t btree2GetPageNo(uint8_t *key, uint32_t len);
uint16_t btree2AllocSlot(Btree2Page *page, uint16_t size);
uint16_t btree2SizeSlot(Btree2Page *page, uint32_t totKeySize, uint8_t height);
uint16_t btree2InstallSlot(Handle *index, Btree2Page *page, Btree2Slot *slot, uint8_t height);
uint16_t btree2SlotSize(Btree2Slot *slot, uint8_t skipBits, uint8_t height);
uint8_t btree2GenHeight(Handle *index);

bool btree2SkipDead(Btree2Set *set);
bool btree2FindSlot(Btree2Set *set, uint8_t *key, uint32_t keyLen);
bool btree2FillTower(Btree2Set *, uint8_t idx);
bool btree2RecyclePage(Handle *index, int type, uint64_t bits);
bool btree2RecyclePageNo(Handle *index, uint64_t bits);
bool btree2InstallKey(Handle *index, Btree2Set *set, uint16_t off, uint8_t *key, uint32_t keyLen, uint8_t height);
