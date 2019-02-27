#include "btree2.h"
#include "btree2_slot.h"
#include <stdlib.h>

//	debug slot function

#ifdef DEBUG
Btree2Slot *btree2Slot(Btree2Page *page, uint32_t off)
{
	return slotptr(page, off);
}

uint8_t *btree2Key(Btree2Slot *slot)
{
	return slotkey(slot);
}

#undef slotkey
#undef slotptr
#define slotkey(s) btree2Key(s)
#define slotptr(p,x) btree2Slot(p,x)
#endif

//	calc size of slot

uint16_t btree2SlotSize(Btree2Slot *slot, uint8_t skipBits, uint8_t height)
{
uint8_t *key = slotkey(slot);
uint32_t size;

	size = sizeof(*slot) + keylen(key);
	size += (height ? height : slot->height) * sizeof(uint16_t);
	size += (1 << skipBits) - 1;
	size >>= skipBits;
	return size;
}

// generate slot tower height (1-16)
//	w/frequency 1/2 down to 1/65536

uint32_t btree2GenHeight(Handle *index) {
uint32_t nrand32 = mynrand48(index->nrandState);
unsigned long height = 0;

	nrand32 |= 0x10000;
#ifdef _WIN32
	_BitScanReverse((unsigned long *)&height, nrand32);
    return 32 - height;
#else
	height = __builtin_clz(nrand32);
	return height + 1;
#endif
}

//	calculate amount (in skipBits) of space needed to install slot in page

uint16_t btree2SizeSlot (uint8_t skipBits, uint32_t totKeySize, uint8_t height)
{
uint32_t amt = (uint16_t)(sizeof(Btree2Slot) + height * sizeof(uint16_t) + totKeySize);

	amt += (1 << skipBits) - 1;
	return amt >> skipBits;
}

// allocate space for new slot (in skipBits units)

uint16_t btree2AllocSlot(Btree2Page *page, uint16_t size) {
uint16_t base = (sizeof(*page) + (1ULL << page->skipBits) - 1) >> page->skipBits;
union Btree2Alloc alloc[1], before[1];

	do {
		*before->word = *page->alloc->word;
		*alloc->word = *before->word;

		if( alloc->nxt > base + size )
	  	  if( alloc->state == Btree2_pageactive )
			alloc->nxt -= size;
		  else
			return 0;
		else
			return 0;

	} while( !atomicCAS32(page->alloc->word, *alloc->word, *before->word) );

	return alloc->nxt;
}

//  find and load page at given level for given key

DbStatus btree2LoadPage(DbMap *map, Btree2Set *set, uint8_t *key, uint32_t keyLen, uint8_t lvl) {
Btree2Index *btree2 = btree2index(map);
ObjId *pageNoPtr;

  set->pageNo.bits = btree2->root.bits;

  //  start at the root level of the btree2 and drill down

  do {
	pageNoPtr = fetchIdSlot (map, set->pageNo);
	set->pageAddr.bits = pageNoPtr->bits;
	set->page = getObj(map, set->pageAddr);

	if( set->page->lvl > set->rootLvl )
		set->rootLvl = set->rootLvl;

	//  compare with fence key and
	//  slide right to next page,
	//	or select the stopper page
	//	on far right end

	if( set->page->fence ) {
		set->slot = slotptr(set->page, set->page->fence);

		if( btree2KeyCmp (slotkey(set->slot), key, keyLen) < 0 ) {
	  		if( (set->pageNo.bits = set->page->right.bits) )
	  			continue;

	  		if( (set->pageNo.bits = set->page->stopper.bits) )
				continue;

			return DB_BTREE_error;
		}
	}

	//  find first key on page greater or equal to target key
	//  and continue to next level

	btree2FindSlot (set, key, keyLen);

	if (set->page->lvl == lvl)
		return DB_OK;

	set->pageNo.bits = get64(key, keyLen, btree2->base->binaryFlds);
  } while( set->pageNo.bits );

  // return error on end of right chain

  return DB_BTREE_error;
}

