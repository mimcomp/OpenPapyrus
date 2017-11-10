/*
 * Summary: Chained hash tables
 * Description: This module implements the hash table support used in various places in the library.
 *
 * Copy: See Copyright for the status of this software.
 *
 * Author: Bjorn Reese <bjorn.reese@systematic.dk>
 */
#ifndef __XML_HASH_H__
#define __XML_HASH_H__

#ifdef __cplusplus
extern "C" {
#endif
/*
 * The hash table.
 */
//typedef struct _xmlHashTable xmlHashTable;
//typedef xmlHashTable * xmlHashTablePtr;

#ifdef __cplusplus
}
#endif

#include <libxml/xmlversion.h>
#include <libxml/parser.h>
//#include <libxml/dict.h>

#ifdef __cplusplus
extern "C" {
#endif
/*
 * Recent version of gcc produce a warning when a function pointer is assigned
 * to an object pointer, or vice versa.  The following macro is a dirty hack
 * to allow suppression of the warning.  If your architecture has function
 * pointers which are a different size than a void pointer, there may be some
 * serious trouble within the library.
 */
/**
 * XML_CAST_FPTR:
 * @fptr:  pointer to a function
 *
 * Macro to do a casting from an object pointer to a
 * function pointer without encountering a warning from
 * gcc
 *
 * #define XML_CAST_FPTR(fptr) (*(void **)(&fptr))
 * This macro violated ISO C aliasing rules (gcc4 on s390 broke)
 * so it is disabled now
 */

#define XML_CAST_FPTR(fptr) fptr
/*
 * function types:
 */
/**
 * xmlHashDeallocator:
 * @payload:  the data in the hash
 * @name:  the name associated
 *
 * Callback to free data from a hash.
 */
typedef void (*xmlHashDeallocator)(void *payload, xmlChar *name);
/**
 * xmlHashCopier:
 * @payload:  the data in the hash
 * @name:  the name associated
 *
 * Callback to copy data from a hash.
 *
 * Returns a copy of the data or NULL in case of error.
 */
typedef void *(*xmlHashCopier)(void *payload, xmlChar *name);
/**
 * xmlHashScanner:
 * @payload:  the data in the hash
 * @data:  extra scannner data
 * @name:  the name associated
 *
 * Callback when scanning data in a hash with the simple scanner.
 */
typedef void (*xmlHashScanner)(void *payload, void *data, xmlChar *name);
/**
 * xmlHashScannerFull:
 * @payload:  the data in the hash
 * @data:  extra scannner data
 * @name:  the name associated
 * @name2:  the second name associated
 * @name3:  the third name associated
 *
 * Callback when scanning data in a hash with the full scanner.
 */
typedef void (*xmlHashScannerFull)(void *payload, void *data, const xmlChar *name, const xmlChar *name2, const xmlChar *name3);
/*
 * Constructor and destructor.
 */
XMLPUBFUN xmlHashTable * /*XMLCALL*/FASTCALL xmlHashCreate(int size);
XMLPUBFUN xmlHashTable * /*XMLCALL*/FASTCALL xmlHashCreateDict(int size, xmlDict * dict);
XMLPUBFUN void /*XMLCALL*/FASTCALL xmlHashFree(xmlHashTable * pTable, xmlHashDeallocator f);
/*
 * Add a new entry to the hash table.
 */
XMLPUBFUN int /*XMLCALL*/FASTCALL xmlHashAddEntry(xmlHashTable * table, const xmlChar *name, void *userdata);
XMLPUBFUN int XMLCALL xmlHashUpdateEntry(xmlHashTable * table, const xmlChar *name, void *userdata, xmlHashDeallocator f);
XMLPUBFUN int /*XMLCALL*/FASTCALL xmlHashAddEntry2(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, void *userdata);
XMLPUBFUN int XMLCALL xmlHashUpdateEntry2(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, void *userdata, xmlHashDeallocator f);
XMLPUBFUN int /*XMLCALL*/FASTCALL xmlHashAddEntry3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3, void *userdata);
XMLPUBFUN int XMLCALL xmlHashUpdateEntry3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3, void *userdata, xmlHashDeallocator f);

/*
 * Remove an entry from the hash table.
 */
XMLPUBFUN int XMLCALL xmlHashRemoveEntry(xmlHashTable * table, const xmlChar *name, xmlHashDeallocator f);
XMLPUBFUN int XMLCALL xmlHashRemoveEntry2(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, xmlHashDeallocator f);
XMLPUBFUN int  XMLCALL xmlHashRemoveEntry3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3,xmlHashDeallocator f);
/*
 * Retrieve the userdata.
 */
XMLPUBFUN void * /*XMLCALL*/FASTCALL xmlHashLookup(xmlHashTable * table, const xmlChar *name);
XMLPUBFUN void * /*XMLCALL*/FASTCALL xmlHashLookup2(xmlHashTable * table, const xmlChar *name, const xmlChar *name2);
XMLPUBFUN void * /*XMLCALL*/FASTCALL xmlHashLookup3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3);
XMLPUBFUN void * XMLCALL xmlHashQLookup(xmlHashTable * table, const xmlChar *name, const xmlChar *prefix);
XMLPUBFUN void * XMLCALL xmlHashQLookup2(xmlHashTable * table, const xmlChar *name, const xmlChar *prefix, const xmlChar *name2, const xmlChar *prefix2);
XMLPUBFUN void * XMLCALL xmlHashQLookup3(xmlHashTable * table, const xmlChar *name, const xmlChar *prefix, const xmlChar *name2, const xmlChar *prefix2, const xmlChar *name3, const xmlChar *prefix3);
/*
 * Helpers.
 */
XMLPUBFUN xmlHashTable * XMLCALL xmlHashCopy(xmlHashTable * table, xmlHashCopier f);
XMLPUBFUN int XMLCALL xmlHashSize(xmlHashTable * table);
XMLPUBFUN void XMLCALL xmlHashScan(xmlHashTable * table, xmlHashScanner f, void *data);
XMLPUBFUN void XMLCALL xmlHashScan3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3, xmlHashScanner f, void *data);
XMLPUBFUN void XMLCALL xmlHashScanFull(xmlHashTable * table, xmlHashScannerFull f, void *data);
XMLPUBFUN void XMLCALL xmlHashScanFull3(xmlHashTable * table, const xmlChar *name, const xmlChar *name2, const xmlChar *name3, xmlHashScannerFull f, void *data);
#ifdef __cplusplus
}
#endif
#endif /* ! __XML_HASH_H__ */
