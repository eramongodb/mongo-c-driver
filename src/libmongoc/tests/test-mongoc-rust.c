#include <mongoc/mongoc-rust-private.h>

#include <TestSuite.h>

static void
test_sanity_check(void)
{
   ASSERT_CMPINT32(mongoc_rust_sanity_check(1), ==, 1);
   ASSERT_CMPINT32(mongoc_rust_sanity_check(2), ==, 2);
   ASSERT_CMPINT32(mongoc_rust_sanity_check(3), ==, 3);
}

void
test_mongoc_rust_install(TestSuite *suite)
{
   TestSuite_Add(suite, "/rust/sanity_check", test_sanity_check);
}
