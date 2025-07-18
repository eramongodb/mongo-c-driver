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

#include <bson/bson.h>

#include <TestSuite.h>


static void
test_bson_utf8_validate (void)
{
   static const unsigned char test1[] = {0xe2, 0x82, 0xac, ' ', 0xe2, 0x82, 0xac, ' ', 0xe2, 0x82, 0xac, 0};
   static const unsigned char test2[] = {0xc0, 0x80, 0};
   static const unsigned char out_of_bound[] = {0xfd, 0x9f, 0xb0, 0x80, 0x80, 0x80, 0};
   static const unsigned char bad[] = {0xed, 0xa0, 0xa5, 0};

   BSON_ASSERT (bson_utf8_validate ("asdf", 4, false));
   BSON_ASSERT (bson_utf8_validate ("asdf", 4, true));
   BSON_ASSERT (bson_utf8_validate ("asdf", 5, true));
   BSON_ASSERT (!bson_utf8_validate ("asdf", 5, false));

   BSON_ASSERT (bson_utf8_validate ((const char *) test1, 11, true));
   BSON_ASSERT (bson_utf8_validate ((const char *) test1, 11, false));
   BSON_ASSERT (bson_utf8_validate ((const char *) test1, 12, true));
   BSON_ASSERT (!bson_utf8_validate ((const char *) test1, 12, false));

   BSON_ASSERT (bson_utf8_validate ((const char *) test2, 2, true));
   BSON_ASSERT (!bson_utf8_validate ((const char *) test2, 2, false));

   /*
    * This character is allowed in 6-byte-sequence UTF-8 standard but seen as
    * out of bound in 4-byte-sequence UTF-8 standard (RFC 3629).
    */
   BSON_ASSERT (!bson_utf8_validate ((const char *) out_of_bound, 1, true));
   BSON_ASSERT (!bson_utf8_validate ((const char *) out_of_bound, 1, false));

   /*
    * Some UTF-8 code points are prohibited to match the constraints of
    * the UTF-16 character encoding (RFC 3629).
    */
   BSON_ASSERT (!bson_utf8_validate ((const char *) bad, 1, true));
   BSON_ASSERT (!bson_utf8_validate ((const char *) bad, 1, false));
}


static void
test_bson_utf8_escape_for_json (void)
{
   char *str;
   char *unescaped = "\x0e";

   str = bson_utf8_escape_for_json ("my\0key", 6);
   ASSERT_CMPSTR (str, "my\\u0000key");
   bson_free (str);

   str = bson_utf8_escape_for_json ("my\xc0\x80key", 7);
   ASSERT_CMPSTR (str, "my\\u0000key");
   bson_free (str);

   str = bson_utf8_escape_for_json ("my\"key", 6);
   ASSERT_CMPSTR (str, "my\\\"key");
   bson_free (str);

   str = bson_utf8_escape_for_json ("my\\key", 6);
   ASSERT_CMPSTR (str, "my\\\\key");
   bson_free (str);

   str = bson_utf8_escape_for_json ("\\\"\\\"", -1);
   ASSERT_CMPSTR (str, "\\\\\\\"\\\\\\\"");
   bson_free (str);

   str = bson_utf8_escape_for_json (unescaped, -1);
   ASSERT_CMPSTR (str, "\\u000e");
   bson_free (str);

   // 2 bytes expected
   {
      str = bson_utf8_escape_for_json ("\xc2", -1);
      ASSERT (!str);

      str = bson_utf8_escape_for_json ("\xc3\xa9", -1);
      ASSERT_CMPSTR (str, "é");
      bson_free (str);
   }

   // 3 bytes expected
   {
      str = bson_utf8_escape_for_json ("\xed", -1);
      ASSERT (!str);

      str = bson_utf8_escape_for_json ("\xed\x90", -1);
      ASSERT (!str);

      str = bson_utf8_escape_for_json ("\xe4\xb8\xad", -1);
      ASSERT_CMPSTR (str, "中");
      bson_free (str);
   }

   // 4 bytes expected
   {
      str = bson_utf8_escape_for_json ("\xf0", -1);
      ASSERT (!str);

      str = bson_utf8_escape_for_json ("\xf0\x9f", -1);
      ASSERT (!str);

      str = bson_utf8_escape_for_json ("\xf0\x9f\x9f", -1);
      ASSERT (!str);

      str = bson_utf8_escape_for_json ("\xf0\xa0\x9c\x8f", -1);
      ASSERT_CMPSTR (str, "𠜏");
      bson_free (str);
   }

   // Normal strings
   {
      str = bson_utf8_escape_for_json ("this is a normal string", -1);
      ASSERT_CMPSTR (str, "this is a normal string");
      bson_free (str);

      str = bson_utf8_escape_for_json ("this is a normal string", 23);
      ASSERT_CMPSTR (str, "this is a normal string");
      bson_free (str);

      str = bson_utf8_escape_for_json ("this is a normal string", 10);
      ASSERT_CMPSTR (str, "this is a ");
      bson_free (str);
   }
}


static void
test_bson_utf8_invalid (void)
{
   /* no UTF-8 sequence can start with 0x80 */
   static const unsigned char bad[] = {0x80, 0};

   BSON_ASSERT (!bson_utf8_validate ((const char *) bad, 1, true));
   BSON_ASSERT (!bson_utf8_validate ((const char *) bad, 1, false));
   BSON_ASSERT (!bson_utf8_escape_for_json ((const char *) bad, 1));
}


static void
test_bson_utf8_nil (void)
{
   static const unsigned char test[] = {'a', 0, 'b', 0};
   char *str;

   BSON_ASSERT (bson_utf8_validate ((const char *) test, 3, true));
   BSON_ASSERT (!bson_utf8_validate ((const char *) test, 3, false));

   /* no length provided, stop at first nil */
   str = bson_utf8_escape_for_json ((const char *) test, -1);
   ASSERT_CMPSTR ("a", str);
   bson_free (str);

   str = bson_utf8_escape_for_json ((const char *) test, 3);
   ASSERT_CMPSTR ("a\\u0000b", str);
   bson_free (str);
}


static void
test_bson_utf8_get_char (void)
{
   static const char *test1 = "asdf";
   static const unsigned char test2[] = {0xe2, 0x82, 0xac, ' ', 0xe2, 0x82, 0xac, ' ', 0xe2, 0x82, 0xac, 0};
   const char *c;

   c = test1;
   BSON_ASSERT (bson_utf8_get_char (c) == 'a');
   c = bson_utf8_next_char (c);
   BSON_ASSERT (bson_utf8_get_char (c) == 's');
   c = bson_utf8_next_char (c);
   BSON_ASSERT (bson_utf8_get_char (c) == 'd');
   c = bson_utf8_next_char (c);
   BSON_ASSERT (bson_utf8_get_char (c) == 'f');
   c = bson_utf8_next_char (c);
   BSON_ASSERT (!*c);

   c = (const char *) test2;
   BSON_ASSERT (bson_utf8_get_char (c) == 0x20AC);
   c = bson_utf8_next_char (c);
   BSON_ASSERT (c == (const char *) test2 + 3);
   BSON_ASSERT (bson_utf8_get_char (c) == ' ');
   c = bson_utf8_next_char (c);
   BSON_ASSERT (bson_utf8_get_char (c) == 0x20AC);
   c = bson_utf8_next_char (c);
   BSON_ASSERT (bson_utf8_get_char (c) == ' ');
   c = bson_utf8_next_char (c);
   BSON_ASSERT (bson_utf8_get_char (c) == 0x20AC);
   c = bson_utf8_next_char (c);
   BSON_ASSERT (!*c);
}


static void
test_bson_utf8_from_unichar (void)
{
   static const char test1[] = {'a'};
   static const unsigned char test2[] = {0xc3, 0xbf};
   static const unsigned char test3[] = {0xe2, 0x82, 0xac};
   uint32_t len;
   char str[6];

   /*
    * First possible sequence of a certain length.
    */
   bson_utf8_from_unichar (0, str, &len);
   BSON_ASSERT (len == 1);
   bson_utf8_from_unichar (0x00000080, str, &len);
   BSON_ASSERT (len == 2);
   bson_utf8_from_unichar (0x00000800, str, &len);
   BSON_ASSERT (len == 3);
   bson_utf8_from_unichar (0x00010000, str, &len);
   BSON_ASSERT (len == 4);

   /*
    * Last possible sequence of a certain length.
    */
   bson_utf8_from_unichar (0x0000007F, str, &len);
   BSON_ASSERT (len == 1);
   bson_utf8_from_unichar (0x000007FF, str, &len);
   BSON_ASSERT (len == 2);
   bson_utf8_from_unichar (0x0000FFFF, str, &len);
   BSON_ASSERT (len == 3);
   bson_utf8_from_unichar (0x0010FFFF, str, &len);
   BSON_ASSERT (len == 4);

   /*
    * Other interesting values.
    */
   bson_utf8_from_unichar (0x0000D7FF, str, &len);
   BSON_ASSERT (len == 3);
   bson_utf8_from_unichar (0x0000E000, str, &len);
   BSON_ASSERT (len == 3);
   bson_utf8_from_unichar (0x0000FFFD, str, &len);
   BSON_ASSERT (len == 3);
   bson_utf8_from_unichar (0x0010EFFF, str, &len);
   BSON_ASSERT (len == 4);
   bson_utf8_from_unichar (0x00110000, str, &len);
   BSON_ASSERT (len == 4);

   bson_utf8_from_unichar ('a', str, &len);
   BSON_ASSERT (len == 1);
   BSON_ASSERT (!memcmp (test1, str, 1));

   bson_utf8_from_unichar (0xFF, str, &len);
   BSON_ASSERT (len == 2);
   BSON_ASSERT (!memcmp ((const char *) test2, str, 2));

   bson_utf8_from_unichar (0x20AC, str, &len);
   BSON_ASSERT (len == 3);
   BSON_ASSERT (!memcmp ((const char *) test3, str, 3));
}


static void
test_bson_utf8_non_shortest (void)
{
   static const char *tests[] = {"\xE0\x80\x80",     /* Non-shortest form representation of U+0000 */
                                 "\xF0\x80\x80\x80", /* Non-shortest form representation of U+0000 */

                                 "\xE0\x83\xBF",     /* Non-shortest form representation of U+00FF */
                                 "\xF0\x80\x83\xBF", /* Non-shortest form representation of U+00FF */

                                 "\xF0\x80\xA3\x80", /* Non-shortest form representation of U+08C0 */

                                 NULL};
   static const char *valid_tests[] = {"\xC0\x80", /* Non-shortest form representation of U+0000.
                                                    * This is how \0 should be encoded. Special casing
                                                    * to allow for use in things like strlen(). */

                                       NULL};
   int i;

   for (i = 0; tests[i]; i++) {
      if (bson_utf8_validate (tests[i], strlen (tests[i]), false)) {
         fprintf (stderr, "non-shortest form failure, test %d\n", i);
         BSON_ASSERT (false);
      }
   }

   for (i = 0; valid_tests[i]; i++) {
      if (!bson_utf8_validate (valid_tests[i], strlen (valid_tests[i]), true)) {
         fprintf (stderr, "non-shortest form failure, valid_tests %d\n", i);
         BSON_ASSERT (false);
      }
   }
}


void
test_utf8_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/bson/utf8/validate", test_bson_utf8_validate);
   TestSuite_Add (suite, "/bson/utf8/invalid", test_bson_utf8_invalid);
   TestSuite_Add (suite, "/bson/utf8/nil", test_bson_utf8_nil);
   TestSuite_Add (suite, "/bson/utf8/escape_for_json", test_bson_utf8_escape_for_json);
   TestSuite_Add (suite, "/bson/utf8/get_char_next_char", test_bson_utf8_get_char);
   TestSuite_Add (suite, "/bson/utf8/from_unichar", test_bson_utf8_from_unichar);
   TestSuite_Add (suite, "/bson/utf8/non_shortest", test_bson_utf8_non_shortest);
}
