/*
 * Copyright 2021 MongoDB, Inc.
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

#include "./mongoc-shared-private.h"

#include "common-thread-private.h"
#include <bson/bson.h>

typedef struct _mongoc_shared_ptr_aux {
   int refcount;
   void (*deleter) (void *);
   void *managed;
} _mongoc_shared_ptr_aux;

static void
_release_aux (_mongoc_shared_ptr_aux *aux)
{
   aux->deleter (aux->managed);
   bson_free (aux);
}

static bson_mutex_t g_shared_ptr_mtx;
static bson_once_t g_shared_ptr_mtx_init_once = BSON_ONCE_INIT;

static BSON_ONCE_FUN (_init_mtx)
{
   bson_mutex_init (&g_shared_ptr_mtx);
   BSON_ONCE_RETURN;
}

static void
_shared_ptr_lock ()
{
   bson_mutex_lock (&g_shared_ptr_mtx);
}

static void
_shared_ptr_unlock ()
{
   bson_mutex_unlock (&g_shared_ptr_mtx);
}

void
mongoc_shared_ptr_reset (mongoc_shared_ptr *const ptr,
                         void *const pointee,
                         void (*const deleter) (void *))
{
   BSON_ASSERT_PARAM (ptr);
   if (!mongoc_shared_ptr_is_null (*ptr)) {
      /* Release the old value of the pointer, possibly destroying it */
      mongoc_shared_ptr_reset_null (ptr);
   }
   ptr->ptr = pointee;
   ptr->_aux = NULL;
   /* Take the new value */
   if (pointee != NULL) {
      BSON_ASSERT (deleter != NULL);
      ptr->_aux = bson_malloc0 (sizeof (_mongoc_shared_ptr_aux));
      ptr->_aux->deleter = deleter;
      ptr->_aux->refcount = 1;
      ptr->_aux->managed = pointee;
   }
   bson_once (&g_shared_ptr_mtx_init_once, _init_mtx);
}

void
mongoc_shared_ptr_assign (mongoc_shared_ptr *const out,
                          mongoc_shared_ptr const from)
{
   /* Copy from 'from' *first*, since this might be a self-assignment. */
   mongoc_shared_ptr copied = mongoc_shared_ptr_copy (from);
   BSON_ASSERT_PARAM (out);
   mongoc_shared_ptr_reset_null (out);
   *out = copied;
}

mongoc_shared_ptr
mongoc_shared_ptr_create (void *pointee, void (*deleter) (void *))
{
   mongoc_shared_ptr ret = MONGOC_SHARED_PTR_NULL;
   mongoc_shared_ptr_reset (&ret, pointee, deleter);
   return ret;
}

void
mongoc_atomic_shared_ptr_store (mongoc_shared_ptr *const out,
                                mongoc_shared_ptr const from)
{
   mongoc_shared_ptr prev = MONGOC_SHARED_PTR_NULL;
   BSON_ASSERT_PARAM (out);

   /* We are effectively "copying" the 'from' */
   (void) mongoc_shared_ptr_copy (from);

   _shared_ptr_lock ();
   /* Do the exchange. Quick! */
   prev = *out;
   *out = from;
   _shared_ptr_unlock ();

   /* Free the pointer that we just overwrote */
   mongoc_shared_ptr_reset_null (&prev);
}

mongoc_shared_ptr
mongoc_atomic_shared_ptr_load (mongoc_shared_ptr const *ptr)
{
   mongoc_shared_ptr r;
   BSON_ASSERT_PARAM (ptr);
   _shared_ptr_lock ();
   r = mongoc_shared_ptr_copy (*ptr);
   _shared_ptr_unlock ();
   return r;
}

mongoc_shared_ptr
mongoc_shared_ptr_copy (mongoc_shared_ptr const ptr)
{
   mongoc_shared_ptr ret = ptr;
   if (!mongoc_shared_ptr_is_null (ptr)) {
      bson_atomic_int_fetch_add (
         &ret._aux->refcount, 1, bson_memory_order_acquire);
   }
   return ret;
}

void
mongoc_shared_ptr_reset_null (mongoc_shared_ptr *const ptr)
{
   int prevcount = 0;
   BSON_ASSERT_PARAM (ptr);
   if (mongoc_shared_ptr_is_null (*ptr)) {
      /* Already null. Okay. */
      return;
   }
   /* Decrement the reference count by one */
   prevcount = bson_atomic_int_fetch_sub (
      &ptr->_aux->refcount, 1, bson_memory_order_acq_rel);
   if (prevcount == 1) {
      /* We just decremented from one to zero, so this is the last instance.
       * Release the managed data. */
      _release_aux (ptr->_aux);
   }
   ptr->_aux = NULL;
   ptr->ptr = NULL;
}

int
mongoc_shared_ptr_use_count (mongoc_shared_ptr const ptr)
{
   BSON_ASSERT (
      !mongoc_shared_ptr_is_null (ptr) &&
      "Unbound mongoc_shared_ptr given to mongoc_shared_ptr_use_count");
   return bson_atomic_int_fetch (&ptr._aux->refcount,
                                 bson_memory_order_relaxed);
}