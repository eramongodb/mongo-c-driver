#include <mongoc/mongoc-rust-private.h>

#include <bson/bson.h>

#include <TestSuite.h>
#include <test-libmongoc.h>

#include <stdint.h>

static void
test_sanity_check(void)
{
   ASSERT_CMPINT32(mongoc_rust_sanity_check(1), ==, 1);
   ASSERT_CMPINT32(mongoc_rust_sanity_check(2), ==, 2);
   ASSERT_CMPINT32(mongoc_rust_sanity_check(3), ==, 3);
}

static void
test_client_new_valid(void)
{
   capture_logs(true);

   mongoc_rust_client_t *const client = mongoc_rust_client_new("mongodb://localhost:27017");

   ASSERT(client);
   ASSERT_NO_CAPTURED_LOGS("client");

   mongoc_rust_client_destroy(client);

   capture_logs(false);
}

static void
test_client_new_invalid(void)
{
   capture_logs(true);

   mongoc_rust_client_t *const client = mongoc_rust_client_new("invalid");

   ASSERT(!client);
   ASSERT_CAPTURED_LOG("client", MONGOC_LOG_LEVEL_ERROR, "connection string contains no scheme");

   mongoc_rust_client_destroy(client);

   capture_logs(false);
}

static void
test_client_get_database(void)
{
   mongoc_rust_client_t *const client = mongoc_rust_client_new("mongodb://localhost:27017");
   ASSERT(client);

   {
      mongoc_rust_database_t *const db = mongoc_rust_client_get_database(client, "admin");
      ASSERT(db);
      ASSERT_CMPSTR(mongoc_rust_database_get_name(db), "admin");
      mongoc_rust_database_destroy(db);
   }

   mongoc_rust_client_destroy(client);
}

static void
test_database_get_name(void)
{
   mongoc_rust_client_t *const client = mongoc_rust_client_new("mongodb://localhost:27017");
   ASSERT(client);

   char const *names[] = {"", "db", "test", NULL};

   for (char const **iter = names; *iter; ++iter) {
      char const *const name = *iter;

      mongoc_rust_database_t *const db = mongoc_rust_client_get_database(client, name);

      ASSERT(db);
      ASSERT_CMPSTR(mongoc_rust_database_get_name(db), name);

      mongoc_rust_database_destroy(db);
   }

   mongoc_rust_client_destroy(client);
}

static void
test_database_drop(void)
{
   mongoc_rust_client_t *const client = mongoc_rust_client_new("mongodb://localhost:27017");
   ASSERT(client);
   mongoc_rust_database_t *const db = mongoc_rust_client_get_database(client, "test");
   ASSERT(db);

   capture_logs(true);
   ASSERT(mongoc_rust_database_drop(db));
   ASSERT(mongoc_rust_database_drop(db));
   ASSERT(mongoc_rust_database_drop(db));
   ASSERT_NO_CAPTURED_LOGS("database");
   capture_logs(false);

   mongoc_rust_database_destroy(db);
   mongoc_rust_client_destroy(client);
}

static void
test_database_get_collection(void)
{
   mongoc_rust_client_t *const client = mongoc_rust_client_new("mongodb://localhost:27017");
   ASSERT(client);

   mongoc_rust_database_t *const db = mongoc_rust_client_get_database(client, "db");
   ASSERT(db);

   {
      mongoc_rust_collection_t *const coll = mongoc_rust_database_get_collection(db, "collection");
      ASSERT(coll);
      mongoc_rust_collection_destroy(coll);
   }

   mongoc_rust_database_destroy(db);
   mongoc_rust_client_destroy(client);
}

static void
test_database_create_collection(void)
{
   mongoc_rust_client_t *const client = mongoc_rust_client_new("mongodb://localhost:27017");
   ASSERT(client);

   mongoc_rust_database_t *const db = mongoc_rust_client_get_database(client, "db");
   ASSERT(db);

   mongoc_rust_database_drop(db);

   {
      capture_logs(true);

      char **const names = mongoc_rust_database_get_collection_names_with_opts(db);
      ASSERT(names);
      ASSERT_CMPSTR(names[0], NULL);

      bson_strfreev(names);

      ASSERT_NO_CAPTURED_LOGS("database");

      capture_logs(false);
   }

   {
      mongoc_rust_collection_t *const coll = mongoc_rust_database_create_collection(db, "coll");
      ASSERT(coll);
      mongoc_rust_collection_destroy(coll);
   }

   {
      capture_logs(true);

      char **const names = mongoc_rust_database_get_collection_names_with_opts(db);
      ASSERT(names);
      ASSERT_CMPSTR(names[0], "coll");
      ASSERT_CMPSTR(names[1], NULL);

      bson_strfreev(names);

      ASSERT_NO_CAPTURED_LOGS("database");

      capture_logs(false);
   }

   {
      mongoc_rust_collection_t *const coll = mongoc_rust_database_create_collection(db, "test");
      ASSERT(coll);
      mongoc_rust_collection_destroy(coll);
   }

   {
      capture_logs(true);

      char **const names = mongoc_rust_database_get_collection_names_with_opts(db);
      ASSERT(names);
      ASSERT_CMPSTR(names[0], "coll");
      ASSERT_CMPSTR(names[1], "test");
      ASSERT_CMPSTR(names[2], NULL);

      bson_strfreev(names);

      ASSERT_NO_CAPTURED_LOGS("database");

      capture_logs(false);
   }

   mongoc_rust_database_destroy(db);
   mongoc_rust_client_destroy(client);
}

static void
test_collection_get_name(void)
{
   mongoc_rust_client_t *const client = mongoc_rust_client_new("mongodb://localhost:27017");
   ASSERT(client);

   mongoc_rust_database_t *const db = mongoc_rust_client_get_database(client, "db");
   ASSERT(db);

   char const *names[] = {"", "coll", "test", NULL};

   for (char const **iter = names; *iter; ++iter) {
      char const *const name = *iter;

      mongoc_rust_collection_t *const coll = mongoc_rust_database_get_collection(db, name);
      ASSERT(coll);
      ASSERT_CMPSTR(mongoc_rust_collection_get_name(coll), name);

      mongoc_rust_collection_destroy(coll);
   }

   mongoc_rust_database_destroy(db);
   mongoc_rust_client_destroy(client);
}

static void
test_collection_drop(void)
{
   mongoc_rust_client_t *const client = mongoc_rust_client_new("mongodb://localhost:27017");
   ASSERT(client);
   mongoc_rust_database_t *const db = mongoc_rust_client_get_database(client, "test");
   ASSERT(db);
   mongoc_rust_collection_t *const coll = mongoc_rust_database_get_collection(db, "test");
   ASSERT(coll);

   capture_logs(true);
   ASSERT(mongoc_rust_collection_drop(coll));
   ASSERT(mongoc_rust_collection_drop(coll));
   ASSERT(mongoc_rust_collection_drop(coll));
   ASSERT_NO_CAPTURED_LOGS("collection");
   capture_logs(false);

   mongoc_rust_collection_destroy(coll);
   mongoc_rust_database_destroy(db);
   mongoc_rust_client_destroy(client);
}

static void
test_collection_count_documents(void)
{
   mongoc_rust_client_t *const client = mongoc_rust_client_new("mongodb://localhost:27017");
   ASSERT(client);
   mongoc_rust_database_t *const db = mongoc_rust_client_get_database(client, "test");
   ASSERT(db);
   mongoc_rust_collection_t *const coll = mongoc_rust_database_get_collection(db, "test");
   ASSERT(coll);

   capture_logs(true);
   int64_t const count = mongoc_rust_collection_count_documents(coll);
   ASSERT(count >= 0);
   ASSERT_NO_CAPTURED_LOGS("collection");
   capture_logs(false);

   mongoc_rust_collection_destroy(coll);
   mongoc_rust_database_destroy(db);
   mongoc_rust_client_destroy(client);
}

static void
test_database_drop_async(void)
{
   mongoc_rust_client_t *const client = mongoc_rust_client_new("mongodb://localhost:27017");
   ASSERT(client);

   mongoc_rust_database_t *const db = mongoc_rust_client_get_database(client, "db");
   ASSERT(db);

   capture_logs(true);
   mongoc_rust_collection_destroy(mongoc_rust_database_create_collection(db, "coll1"));
   mongoc_rust_collection_destroy(mongoc_rust_database_create_collection(db, "coll2"));
   mongoc_rust_collection_destroy(mongoc_rust_database_create_collection(db, "coll3"));
   ASSERT_NO_CAPTURED_LOGS("mongoc_rust_database_create_collection");
   capture_logs(false);

   {
      mongoc_rust_future_t *const future = mongoc_rust_database_drop_async(db);
      ASSERT(future);

      ASSERT(!mongoc_rust_future_get_void(future));

      // `mongoc_rust_future_wait(future)`
      while (!mongoc_rust_future_poll(future)) {
         ASSERT(!mongoc_rust_future_get_void(future));

         mongoc_rust_future_make_progress(future);
      }

      ASSERT(mongoc_rust_future_get_void(future));

      mongoc_rust_future_destroy(future);
   }

   mongoc_rust_database_destroy(db);
   mongoc_rust_client_destroy(client);
}

static void
test_collection_count_documents_async(void)
{
   mongoc_rust_client_t *const client = mongoc_rust_client_new("mongodb://localhost:27017");
   ASSERT(client);
   mongoc_rust_database_t *const db = mongoc_rust_client_get_database(client, "test");
   ASSERT(db);
   mongoc_rust_collection_t *const coll = mongoc_rust_database_get_collection(db, "test");
   ASSERT(coll);

   {
      mongoc_rust_future_t *const future = mongoc_rust_collection_count_documents_async(coll);
      ASSERT(future);

      ASSERT(!mongoc_rust_future_get_void(future));

      uint64_t count;

      // `mongoc_rust_future_wait(future)`
      while (!mongoc_rust_future_poll(future)) {
         ASSERT(!mongoc_rust_future_get_uint64(future, &count));

         mongoc_rust_future_make_progress(future);
      }

      ASSERT(mongoc_rust_future_get_uint64(future, &count));
      ASSERT_CMPUINT64(count, >=, 0);

      mongoc_rust_future_destroy(future);
   }

   mongoc_rust_collection_destroy(coll);
   mongoc_rust_database_destroy(db);
   mongoc_rust_client_destroy(client);
}

void
test_mongoc_rust_install(TestSuite *suite)
{
   TestSuite_Add(suite, "/rust/sanity_check", test_sanity_check);

   TestSuite_AddLive(suite, "/rust/client/new/valid", test_client_new_valid);
   TestSuite_AddLive(suite, "/rust/client/new/invalid", test_client_new_invalid);
   TestSuite_AddLive(suite, "/rust/client/get_database", test_client_get_database);

   TestSuite_AddLive(suite, "/rust/database/get_name", test_database_get_name);
   TestSuite_AddLive(suite, "/rust/database/drop", test_database_drop);
   TestSuite_AddLive(suite, "/rust/database/get_collection", test_database_get_collection);
   TestSuite_AddLive(suite, "/rust/database/create_collection", test_database_create_collection);

   TestSuite_AddLive(suite, "/rust/collection/get_name", test_collection_get_name);
   TestSuite_AddLive(suite, "/rust/collection/drop", test_collection_drop);
   TestSuite_AddLive(suite, "/rust/collection/count_documents", test_collection_count_documents);

   TestSuite_AddLive(suite, "/rust/database/drop_async", test_database_drop_async);
   TestSuite_AddLive(suite, "/rust/collection/count_documents_async", test_collection_count_documents_async);
}
