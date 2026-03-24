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

#include <mongoc/mongoc-config.h>


#ifdef MONGOC_ENABLE_CRYPTO_RUST

#include <bson/macros.h>

#include <stdint.h>

int
_mongoc_rand_bytes(uint8_t *buf, int num)
{
   (void)buf;
   (void)num;
   BSON_UNREACHABLE("not yet implemented!");
}

void
mongoc_rand_seed(const void *buf, int num)
{
   (void)buf;
   (void)num;
   BSON_UNREACHABLE("not yet implemented!");
}

void
mongoc_rand_add(const void *buf, int num, double entropy)
{
   (void)buf;
   (void)num;
   (void)entropy;
   BSON_UNREACHABLE("not yet implemented!");
}

int
mongoc_rand_status(void)
{
   BSON_UNREACHABLE("not yet implemented!");
}

#endif
