/*
 * Copyright 2009-present MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <bson/bson-prelude.h>


#ifndef BSON_TYPES_H
#define BSON_TYPES_H

#include <bson/bson-endian.h>
#include <bson/bson_t.h>
#include <bson/compat.h>
#include <bson/config.h>
#include <bson/error.h>
#include <bson/macros.h>

#include <sys/types.h>

#include <stdlib.h>

BSON_BEGIN_DECLS


/*
 *--------------------------------------------------------------------------
 *
 * bson_unichar_t --
 *
 *       bson_unichar_t provides an unsigned 32-bit type for containing
 *       unicode characters. When iterating UTF-8 sequences, this should
 *       be used to avoid losing the high-bits of non-ascii characters.
 *
 *--------------------------------------------------------------------------
 */

typedef uint32_t bson_unichar_t;


/**
 * @brief Flags configuring the creation of a bson_context_t
 */
typedef enum {
   /** Use default options */
   BSON_CONTEXT_NONE = 0,
   /* Deprecated: Generating new OIDs from a bson_context_t is always
      thread-safe */
   BSON_CONTEXT_THREAD_SAFE = (1 << 0),
   /* Deprecated: Does nothing and is ignored */
   BSON_CONTEXT_DISABLE_HOST_CACHE = (1 << 1),
   /* Call getpid() instead of remembering the result of getpid() when using the
      context */
   BSON_CONTEXT_DISABLE_PID_CACHE = (1 << 2),
   /* Deprecated: Does nothing */
   BSON_CONTEXT_USE_TASK_ID = (1 << 3),
} bson_context_flags_t;


/**
 * bson_context_t:
 *
 * This structure manages context for the bson library. It handles
 * configuration for thread-safety and other performance related requirements.
 * Consumers will create a context and may use multiple under a variety of
 * situations.
 *
 * If your program calls fork(), you should initialize a new bson_context_t
 * using bson_context_init().
 *
 * If you are using threading, it is suggested that you use a bson_context_t
 * per thread for best performance. Alternatively, you can initialize the
 * bson_context_t with BSON_CONTEXT_THREAD_SAFE, although a performance penalty
 * will be incurred.
 *
 * Many functions will require that you provide a bson_context_t such as OID
 * generation.
 *
 * This structure is opaque in that you cannot see the contents of the
 * structure. However, it is stack allocatable in that enough padding is
 * provided in _bson_context_t to hold the structure.
 */
typedef struct _bson_context_t bson_context_t;

/**
 * bson_json_opts_t:
 *
 * This structure is used to pass options for serializing BSON into extended
 * JSON to the respective serialization methods.
 *
 * max_len can be either a non-negative integer, or BSON_MAX_LEN_UNLIMITED to
 * set no limit for serialization length.
 */
typedef struct _bson_json_opts_t bson_json_opts_t;


/**
 * bson_oid_t:
 *
 * This structure contains the binary form of a BSON Object Id as specified
 * on http://bsonspec.org. If you would like the bson_oid_t in string form
 * see bson_oid_to_string() or bson_oid_to_string_r().
 */
typedef struct {
   uint8_t bytes[12];
} bson_oid_t;

BSON_STATIC_ASSERT2 (oid_t, sizeof (bson_oid_t) == 12);

/**
 * bson_decimal128_t:
 *
 * @high The high-order bytes of the decimal128.  This field contains sign,
 *       combination bits, exponent, and part of the coefficient continuation.
 * @low  The low-order bytes of the decimal128.  This field contains the second
 *       part of the coefficient continuation.
 *
 * This structure is a boxed type containing the value for the BSON decimal128
 * type.  The structure stores the 128 bits such that they correspond to the
 * native format for the IEEE decimal128 type, if it is implemented.
 **/
typedef struct {
#if BSON_BYTE_ORDER == BSON_LITTLE_ENDIAN
   uint64_t low;
   uint64_t high;
#elif BSON_BYTE_ORDER == BSON_BIG_ENDIAN
   uint64_t high;
   uint64_t low;
#endif
} bson_decimal128_t;


/**
 * @brief Flags and error codes for BSON validation functions.
 *
 * Pass these flags bits to control the behavior of the `bson_validate` family
 * of functions.
 *
 * Additionally, if validation fails, then the error code set on a `bson_error_t`
 * will have the value corresponding to the reason that validation failed.
 */
typedef enum {
   /**
    * @brief No special validation behavior specified.
    */
   BSON_VALIDATE_NONE = 0,
   /**
    * @brief Check that all text components of the BSON data are valid UTF-8.
    *
    * Note that this will also cause validation to reject valid text that contains
    * a null character. This can be changed by also passing
    * `BSON_VALIDATE_UTF8_ALLOW_NULL`
    */
   BSON_VALIDATE_UTF8 = (1 << 0),
   /**
    * @brief Check that element keys do not begin with an ASCII dollar `$`
    */
   BSON_VALIDATE_DOLLAR_KEYS = (1 << 1),
   /**
    * @brief Check that element keys do not contain an ASCII period `.`
    */
   BSON_VALIDATE_DOT_KEYS = (1 << 2),
   /**
    * @brief If set then it is *not* an error for a UTF-8 string to contain
    * embedded null characters.
    *
    * This has no effect unless `BSON_VALIDATE_UTF8` is also passed.
    */
   BSON_VALIDATE_UTF8_ALLOW_NULL = (1 << 3),
   /**
    * @brief Check that no element key is a zero-length empty string.
    */
   BSON_VALIDATE_EMPTY_KEYS = (1 << 4),
   /**
    * @brief This is not a flag that controls behavior, but is instead used to indicate
    * that a BSON document is corrupted in some way. This is the value that will
    * appear as an error code.
    *
    * Passing this as a flag has no effect.
    */
   BSON_VALIDATE_CORRUPT = (1 << 5),
} bson_validate_flags_t;


/**
 * bson_type_t:
 *
 * This enumeration contains all of the possible types within a BSON document.
 * Use bson_iter_type() to fetch the type of a field while iterating over it.
 */
typedef enum {
   BSON_TYPE_EOD = 0x00,
   BSON_TYPE_DOUBLE = 0x01,
   BSON_TYPE_UTF8 = 0x02,
   BSON_TYPE_DOCUMENT = 0x03,
   BSON_TYPE_ARRAY = 0x04,
   BSON_TYPE_BINARY = 0x05,
   BSON_TYPE_UNDEFINED = 0x06,
   BSON_TYPE_OID = 0x07,
   BSON_TYPE_BOOL = 0x08,
   BSON_TYPE_DATE_TIME = 0x09,
   BSON_TYPE_NULL = 0x0A,
   BSON_TYPE_REGEX = 0x0B,
   BSON_TYPE_DBPOINTER = 0x0C,
   BSON_TYPE_CODE = 0x0D,
   BSON_TYPE_SYMBOL = 0x0E,
   BSON_TYPE_CODEWSCOPE = 0x0F,
   BSON_TYPE_INT32 = 0x10,
   BSON_TYPE_TIMESTAMP = 0x11,
   BSON_TYPE_INT64 = 0x12,
   BSON_TYPE_DECIMAL128 = 0x13,
   BSON_TYPE_MAXKEY = 0x7F,
   BSON_TYPE_MINKEY = 0xFF,
} bson_type_t;


/**
 * bson_subtype_t:
 *
 * This enumeration contains the various subtypes that may be used in a binary
 * field. See http://bsonspec.org for more information.
 */
typedef enum {
   BSON_SUBTYPE_BINARY = 0x00,
   BSON_SUBTYPE_FUNCTION = 0x01,
   BSON_SUBTYPE_BINARY_DEPRECATED = 0x02,
   BSON_SUBTYPE_UUID_DEPRECATED = 0x03,
   BSON_SUBTYPE_UUID = 0x04,
   BSON_SUBTYPE_MD5 = 0x05,
   BSON_SUBTYPE_ENCRYPTED = 0x06,
   BSON_SUBTYPE_COLUMN = 0x07,
   BSON_SUBTYPE_SENSITIVE = 0x08,
   BSON_SUBTYPE_VECTOR = 0x09,
   BSON_SUBTYPE_USER = 0x80,
} bson_subtype_t;


/*
 *--------------------------------------------------------------------------
 *
 * bson_value_t --
 *
 *       A boxed type to contain various bson_type_t types.
 *
 * See also:
 *       bson_value_copy()
 *       bson_value_destroy()
 *
 *--------------------------------------------------------------------------
 */

typedef struct _bson_value_t {
   bson_type_t value_type;
   int32_t padding;
   union {
      bson_oid_t v_oid;
      int64_t v_int64;
      int32_t v_int32;
      int8_t v_int8;
      double v_double;
      bool v_bool;
      int64_t v_datetime;
      struct {
         uint32_t timestamp;
         uint32_t increment;
      } v_timestamp;
      struct {
         char *str;
         uint32_t len;
      } v_utf8;
      struct {
         uint8_t *data;
         uint32_t data_len;
      } v_doc;
      struct {
         uint8_t *data;
         uint32_t data_len;
         bson_subtype_t subtype;
      } v_binary;
      struct {
         char *regex;
         char *options;
      } v_regex;
      struct {
         char *collection;
         uint32_t collection_len;
         bson_oid_t oid;
      } v_dbpointer;
      struct {
         char *code;
         uint32_t code_len;
      } v_code;
      struct {
         char *code;
         uint8_t *scope_data;
         uint32_t code_len;
         uint32_t scope_len;
      } v_codewscope;
      struct {
         char *symbol;
         uint32_t len;
      } v_symbol;
      bson_decimal128_t v_decimal128;
   } value;
} bson_value_t;


/**
 * bson_iter_t:
 *
 * This structure manages iteration over a bson_t structure. It keeps track
 * of the location of the current key and value within the buffer. Using the
 * various functions to get the value of the iter will read from these
 * locations.
 *
 * This structure is safe to discard on the stack. No cleanup is necessary
 * after using it.
 */
typedef struct {
   const uint8_t *raw; /* The raw buffer being iterated. */
   uint32_t len;       /* The length of raw. */
   uint32_t off;       /* The offset within the buffer. */
   uint32_t type;      /* The offset of the type byte. */
   uint32_t key;       /* The offset of the key byte. */
   uint32_t d1;        /* The offset of the first data byte. */
   uint32_t d2;        /* The offset of the second data byte. */
   uint32_t d3;        /* The offset of the third data byte. */
   uint32_t d4;        /* The offset of the fourth data byte. */
   uint32_t next_off;  /* The offset of the next field. */
   uint32_t err_off;   /* The offset of the error. */
   bson_value_t value; /* Internal value for various state. */
} bson_iter_t;


/**
 * bson_reader_t:
 *
 * This structure is used to iterate over a sequence of BSON documents. It
 * allows for them to be iterated with the possibility of no additional
 * memory allocations under certain circumstances such as reading from an
 * incoming mongo packet.
 */
BSON_ALIGNED_BEGIN (BSON_ALIGN_OF_PTR)
typedef struct {
   uint32_t type;
   /**< private >**/
} bson_reader_t BSON_ALIGNED_END (BSON_ALIGN_OF_PTR);


/**
 * bson_visitor_t:
 *
 * This structure contains a series of pointers that can be executed for
 * each field of a BSON document based on the field type.
 *
 * For example, if an int32 field is found, visit_int32 will be called.
 *
 * When visiting each field using bson_iter_visit_all(), you may provide a
 * data pointer that will be provided with each callback. This might be useful
 * if you are marshaling to another language.
 *
 * You may pre-maturely stop the visitation of fields by returning true in your
 * visitor. Returning false will continue visitation to further fields.
 */
typedef struct {
   /* run before / after descending into a document */
   bool (BSON_CALL *visit_before) (const bson_iter_t *iter, const char *key, void *data);
   bool (BSON_CALL *visit_after) (const bson_iter_t *iter, const char *key, void *data);
   /* corrupt BSON, or unsupported type and visit_unsupported_type not set */
   void (BSON_CALL *visit_corrupt) (const bson_iter_t *iter, void *data);
   /* normal bson field callbacks */
   bool (BSON_CALL *visit_double) (const bson_iter_t *iter, const char *key, double v_double, void *data);
   bool (BSON_CALL *visit_utf8) (
      const bson_iter_t *iter, const char *key, size_t v_utf8_len, const char *v_utf8, void *data);
   bool (BSON_CALL *visit_document) (const bson_iter_t *iter, const char *key, const bson_t *v_document, void *data);
   bool (BSON_CALL *visit_array) (const bson_iter_t *iter, const char *key, const bson_t *v_array, void *data);
   bool (BSON_CALL *visit_binary) (const bson_iter_t *iter,
                                   const char *key,
                                   bson_subtype_t v_subtype,
                                   size_t v_binary_len,
                                   const uint8_t *v_binary,
                                   void *data);
   /* normal field with deprecated "Undefined" BSON type */
   bool (BSON_CALL *visit_undefined) (const bson_iter_t *iter, const char *key, void *data);
   bool (BSON_CALL *visit_oid) (const bson_iter_t *iter, const char *key, const bson_oid_t *v_oid, void *data);
   bool (BSON_CALL *visit_bool) (const bson_iter_t *iter, const char *key, bool v_bool, void *data);
   bool (BSON_CALL *visit_date_time) (const bson_iter_t *iter, const char *key, int64_t msec_since_epoch, void *data);
   bool (BSON_CALL *visit_null) (const bson_iter_t *iter, const char *key, void *data);
   bool (BSON_CALL *visit_regex) (
      const bson_iter_t *iter, const char *key, const char *v_regex, const char *v_options, void *data);
   bool (BSON_CALL *visit_dbpointer) (const bson_iter_t *iter,
                                      const char *key,
                                      size_t v_collection_len,
                                      const char *v_collection,
                                      const bson_oid_t *v_oid,
                                      void *data);
   bool (BSON_CALL *visit_code) (
      const bson_iter_t *iter, const char *key, size_t v_code_len, const char *v_code, void *data);
   bool (BSON_CALL *visit_symbol) (
      const bson_iter_t *iter, const char *key, size_t v_symbol_len, const char *v_symbol, void *data);
   bool (BSON_CALL *visit_codewscope) (const bson_iter_t *iter,
                                       const char *key,
                                       size_t v_code_len,
                                       const char *v_code,
                                       const bson_t *v_scope,
                                       void *data);
   bool (BSON_CALL *visit_int32) (const bson_iter_t *iter, const char *key, int32_t v_int32, void *data);
   bool (BSON_CALL *visit_timestamp) (
      const bson_iter_t *iter, const char *key, uint32_t v_timestamp, uint32_t v_increment, void *data);
   bool (BSON_CALL *visit_int64) (const bson_iter_t *iter, const char *key, int64_t v_int64, void *data);
   bool (BSON_CALL *visit_maxkey) (const bson_iter_t *iter, const char *key, void *data);
   bool (BSON_CALL *visit_minkey) (const bson_iter_t *iter, const char *key, void *data);
   /* if set, called instead of visit_corrupt when an apparently valid BSON
    * includes an unrecognized field type (reading future version of BSON) */
   void (BSON_CALL *visit_unsupported_type) (const bson_iter_t *iter, const char *key, uint32_t type_code, void *data);
   bool (BSON_CALL *visit_decimal128) (const bson_iter_t *iter,
                                       const char *key,
                                       const bson_decimal128_t *v_decimal128,
                                       void *data);

   void *padding[7];
} bson_visitor_t;


/**
 * bson_next_power_of_two:
 * @v: A 32-bit unsigned integer of required bytes.
 *
 * Determines the next larger power of two for the value of @v
 * in a constant number of operations.
 *
 * It is up to the caller to guarantee this will not overflow.
 *
 * Returns: The next power of 2 from @v.
 */
static BSON_INLINE size_t
bson_next_power_of_two (size_t v)
{
   v--;
   v |= v >> 1;
   v |= v >> 2;
   v |= v >> 4;
   v |= v >> 8;
   v |= v >> 16;
#if BSON_WORD_SIZE == 64
   v |= v >> 32;
#endif
   v++;

   return v;
}


static BSON_INLINE bool
bson_is_power_of_two (uint32_t v)
{
   return ((v != 0) && ((v & (v - 1)) == 0));
}


BSON_END_DECLS


#endif /* BSON_TYPES_H */
