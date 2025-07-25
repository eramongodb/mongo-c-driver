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

#include <mongoc/mongoc-client-private.h>
#include <mongoc/mongoc-server-description-private.h>

#include <mongoc/mongoc.h>

#include <TestSuite.h>
#include <test-conveniences.h>

void
reset_basic_sd (mongoc_server_description_t *sd)
{
   bson_error_t error;
   bson_t *hello;

   hello = BCON_NEW ("minWireVersion", BCON_INT32 (WIRE_VERSION_MIN), "maxWireVersion", BCON_INT32 (WIRE_VERSION_MAX));

   mongoc_server_description_reset (sd);
   memset (&error, 0, sizeof (bson_error_t));
   mongoc_server_description_handle_hello (sd, hello, 0 /* rtt */, &error);
   bson_destroy (hello);
}

/* These checks will start failing and need to be updated once CDRIVER-3527 is
 * addressed. */
static void
_test_hostlist (mongoc_server_description_t *sd1,
                bson_t *sd1_hostlist,
                mongoc_server_description_t *sd2,
                bson_t *sd2_hostlist)
{
   bson_reinit (sd1_hostlist);
   bson_reinit (sd2_hostlist);
   BSON_ASSERT (_mongoc_server_description_equal (sd1, sd2));

   /* [ "h1" ] vs [] */
   BSON_APPEND_UTF8 (sd1_hostlist, "0", "h1");
   BSON_ASSERT (!_mongoc_server_description_equal (sd1, sd2));

   /* [ "h1" ] vs [ "h1" ] */
   BSON_APPEND_UTF8 (sd2_hostlist, "0", "h1");
   BSON_ASSERT (_mongoc_server_description_equal (sd1, sd2));

   /* [ "h1", "h2", "h3" ] vs [ "h1" ] */
   BSON_APPEND_UTF8 (sd1_hostlist, "1", "h2");
   BSON_APPEND_UTF8 (sd1_hostlist, "2", "h3");
   BSON_ASSERT (!_mongoc_server_description_equal (sd1, sd2));

   /* [ "h1", "h2", "h3" ] vs [ "h1", "h3", "h2" ]. Considered unequal since we
    * do not do a set comparison. */
   BSON_APPEND_UTF8 (sd2_hostlist, "1", "h3");
   BSON_APPEND_UTF8 (sd2_hostlist, "2", "h2");
   BSON_ASSERT (!_mongoc_server_description_equal (sd1, sd2));

   /* [ "h1", "h1" ] vs [ "h1" ]. Considered unequal since we do not do a set
    * comparison. */
   bson_reinit (sd1_hostlist);
   bson_reinit (sd2_hostlist);
   BSON_APPEND_UTF8 (sd1_hostlist, "0", "h1");
   BSON_APPEND_UTF8 (sd1_hostlist, "1", "h1");
   BSON_APPEND_UTF8 (sd2_hostlist, "0", "h1");
   BSON_ASSERT (!_mongoc_server_description_equal (sd1, sd2));

   /* Arbitrary non-array like equal docs. Considered equal since we don't do
    * any parsing. */
   bson_reinit (sd1_hostlist);
   bson_reinit (sd2_hostlist);
   BSON_APPEND_UTF8 (sd1_hostlist, "test", "h1");
   BSON_APPEND_UTF8 (sd2_hostlist, "test", "h1");
   BSON_ASSERT (_mongoc_server_description_equal (sd1, sd2));
}

/* Unit test of server description. */
/* Check that variations on all (=) fields result in inequal, and variations on
 * all non (=) fields do not */
void
test_server_description_equal (void)
{
   mongoc_server_description_t sd1;
   mongoc_server_description_t sd2;

   mongoc_server_description_init (&sd1, "host:1234", 1);
   mongoc_server_description_init (&sd2, "host:1234", 2);

   reset_basic_sd (&sd1);
   reset_basic_sd (&sd2);
   BSON_ASSERT (_mongoc_server_description_equal (&sd1, &sd2));

   /* "address" differs, still considered equal. */
   sd2.connection_address = "host2:5678";
   BSON_ASSERT (_mongoc_server_description_equal (&sd1, &sd2));

   /* "roundTripTime" differs, still considered equal. */
   sd1.round_trip_time_msec = 1234;
   BSON_ASSERT (_mongoc_server_description_equal (&sd1, &sd2));

   /* "lastWriteDate"/"opTime" are stored in last_hello_response and not parsed
    * out. Check that overwriting the stored reply does not factor into the
    * equality check. */
   bson_reinit (&sd1.last_hello_response);
   BSON_ASSERT (_mongoc_server_description_equal (&sd1, &sd2));

   /* "error" differs, considered unequal. */
   bson_set_error (&sd1.error, MONGOC_ERROR_SERVER, 123, "some error");
   BSON_ASSERT (!_mongoc_server_description_equal (&sd1, &sd2));

   /* "type" differs, considered unequal. */
   reset_basic_sd (&sd1);
   reset_basic_sd (&sd2);
   sd1.type = MONGOC_SERVER_RS_GHOST;
   BSON_ASSERT (!_mongoc_server_description_equal (&sd1, &sd2));

   /* "minWireVersion" differs, considered unequal. */
   reset_basic_sd (&sd1);
   reset_basic_sd (&sd2);
   sd1.min_wire_version = 2;
   BSON_ASSERT (!_mongoc_server_description_equal (&sd1, &sd2));

   /* "maxWireVersion" differs, considered unequal. */
   reset_basic_sd (&sd1);
   reset_basic_sd (&sd2);
   sd1.max_wire_version = 7;
   BSON_ASSERT (!_mongoc_server_description_equal (&sd1, &sd2));

   /* "me" differs, considered unequal. */
   reset_basic_sd (&sd1);
   reset_basic_sd (&sd2);
   sd1.me = "test:1234";
   sd2.me = "test:1235";
   BSON_ASSERT (!_mongoc_server_description_equal (&sd1, &sd2));

   /* But if "me" only differs only in case, considered equal. */
   reset_basic_sd (&sd1);
   reset_basic_sd (&sd2);
   sd1.me = "tesT:1234";
   sd2.me = "test:1234";
   BSON_ASSERT (_mongoc_server_description_equal (&sd1, &sd2));

   /* Test variations of "hosts", "passives", and "arbiters". */
   _test_hostlist (&sd1, &sd1.hosts, &sd2, &sd2.hosts);
   _test_hostlist (&sd1, &sd1.passives, &sd2, &sd2.passives);
   _test_hostlist (&sd1, &sd1.arbiters, &sd2, &sd2.arbiters);

   /* "tags" differs, considered unequal. */
   reset_basic_sd (&sd1);
   reset_basic_sd (&sd2);
   bson_reinit (&sd1.tags);
   BSON_APPEND_UTF8 (&sd1.tags, "tag", "nyc");
   BSON_ASSERT (!_mongoc_server_description_equal (&sd1, &sd2));

   /* "setName" differs, considered unequal. */
   reset_basic_sd (&sd1);
   reset_basic_sd (&sd2);
   sd1.set_name = "set";
   BSON_ASSERT (!_mongoc_server_description_equal (&sd1, &sd2));

   /* "setName" differs by case only, considered unequal. */
   reset_basic_sd (&sd1);
   reset_basic_sd (&sd2);
   sd1.set_name = "set";
   sd2.set_name = "SET";
   BSON_ASSERT (!_mongoc_server_description_equal (&sd1, &sd2));

   /* "setVersion" differs, considered unequal. */
   reset_basic_sd (&sd1);
   reset_basic_sd (&sd2);
   sd1.set_version = 1;
   BSON_ASSERT (!_mongoc_server_description_equal (&sd1, &sd2));

   /* "electionId" differs, considered unequal. */
   reset_basic_sd (&sd1);
   reset_basic_sd (&sd2);
   bson_oid_init_from_string (&sd1.election_id, "000000000000000000001234");
   BSON_ASSERT (!_mongoc_server_description_equal (&sd1, &sd2));

   /* "primary" differs, considered unequal. */
   reset_basic_sd (&sd1);
   reset_basic_sd (&sd2);
   sd1.current_primary = "host";
   BSON_ASSERT (!_mongoc_server_description_equal (&sd1, &sd2));

   /* "primary" differs in case only, considered equal. */
   reset_basic_sd (&sd1);
   reset_basic_sd (&sd2);
   sd1.current_primary = "host";
   sd2.current_primary = "HOST";
   BSON_ASSERT (_mongoc_server_description_equal (&sd1, &sd2));

   /* "logicalSessionTimeoutMinutes" differs, considered unequal. */
   reset_basic_sd (&sd1);
   reset_basic_sd (&sd2);
   sd1.session_timeout_minutes = 1;
   BSON_ASSERT (!_mongoc_server_description_equal (&sd1, &sd2));

   /* "compressors" differs, still considered equal since that is only
    * applicable for handshake. */
   reset_basic_sd (&sd1);
   reset_basic_sd (&sd2);
   bson_reinit (&sd1.compressors);
   BSON_APPEND_UTF8 (&sd1.compressors, "0", "zstd");
   BSON_ASSERT (_mongoc_server_description_equal (&sd1, &sd2));

   /* "topologyVersion" differs, considered unequal. */
   reset_basic_sd (&sd1);
   reset_basic_sd (&sd2);
   BCON_APPEND (&sd1.topology_version, "x", BCON_INT32 (1));
   BSON_ASSERT (!_mongoc_server_description_equal (&sd1, &sd2));

   mongoc_server_description_cleanup (&sd1);
   mongoc_server_description_cleanup (&sd2);
}

/* Test that msg set to anything else besides "isdbgrid" is not considered
 * server type mongos */
void
test_server_description_msg_without_isdbgrid (void)
{
   mongoc_server_description_t sd;
   bson_t *hello;
   bson_error_t error;

   mongoc_server_description_init (&sd, "host:1234", 1);
   hello = BCON_NEW ("minWireVersion",
                     BCON_INT32 (WIRE_VERSION_MIN),
                     "maxWireVersion",
                     BCON_INT32 (WIRE_VERSION_MAX),
                     "msg",
                     "isdbgrid");
   memset (&error, 0, sizeof (bson_error_t));
   mongoc_server_description_handle_hello (&sd, hello, 0 /* rtt */, &error);
   BSON_ASSERT (sd.type == MONGOC_SERVER_MONGOS);

   mongoc_server_description_reset (&sd);
   bson_destroy (hello);
   hello = BCON_NEW ("minWireVersion",
                     BCON_INT32 (WIRE_VERSION_MIN),
                     "maxWireVersion",
                     BCON_INT32 (WIRE_VERSION_MAX),
                     "msg",
                     "something_else");
   mongoc_server_description_handle_hello (&sd, hello, 0 /* rtt */, &error);
   BSON_ASSERT (sd.type == MONGOC_SERVER_STANDALONE);

   bson_destroy (hello);
   mongoc_server_description_cleanup (&sd);
}

static void
test_server_description_ignores_rtt (void)
{
   mongoc_server_description_t sd;
   bson_error_t error;
   bson_t hello;

   bson_init (&hello);
   BCON_APPEND (&hello, "isWritablePrimary", BCON_BOOL (true));

   memset (&error, 0, sizeof (bson_error_t));
   mongoc_server_description_init (&sd, "host:1234", 1);
   /* Initially, the RTT is MONGOC_RTT_UNSET. */
   ASSERT_CMPINT64 (sd.round_trip_time_msec, ==, MONGOC_RTT_UNSET);
   BSON_ASSERT (sd.type == MONGOC_SERVER_UNKNOWN);
   /* If MONGOC_RTT_UNSET is passed as the RTT, it remains MONGOC_RTT_UNSET. */
   mongoc_server_description_handle_hello (&sd, &hello, MONGOC_RTT_UNSET, &error);
   ASSERT_CMPINT64 (sd.round_trip_time_msec, ==, MONGOC_RTT_UNSET);
   BSON_ASSERT (sd.type == MONGOC_SERVER_STANDALONE);
   /* The first real RTT overwrites the stored RTT. */
   mongoc_server_description_handle_hello (&sd, &hello, 10, &error);
   ASSERT_CMPINT64 (sd.round_trip_time_msec, ==, 10);
   BSON_ASSERT (sd.type == MONGOC_SERVER_STANDALONE);
   /* But subsequent MONGOC_RTT_UNSET values do not effect it. */
   mongoc_server_description_handle_hello (&sd, &hello, MONGOC_RTT_UNSET, &error);
   ASSERT_CMPINT64 (sd.round_trip_time_msec, ==, 10);
   BSON_ASSERT (sd.type == MONGOC_SERVER_STANDALONE);

   mongoc_server_description_cleanup (&sd);
   bson_destroy (&hello);
}

static void
test_server_description_hello (void)
{
   mongoc_server_description_t sd;
   bson_error_t error;
   bson_t hello_response;

   bson_init (&hello_response);
   BCON_APPEND (&hello_response, "isWritablePrimary", BCON_BOOL (true));

   memset (&error, 0, sizeof (bson_error_t));
   mongoc_server_description_init (&sd, "host:1234", 1);
   BSON_ASSERT (sd.type == MONGOC_SERVER_UNKNOWN);
   mongoc_server_description_handle_hello (&sd, &hello_response, 0, &error);
   BSON_ASSERT (sd.type == MONGOC_SERVER_STANDALONE);

   mongoc_server_description_cleanup (&sd);
   bson_destroy (&hello_response);
}

static void
test_server_description_hello_cmd_not_found (void)
{
   mongoc_server_description_t sd;
   bson_error_t error;
   const char *response = "{"
                          "  'ok' : 0,"
                          "  'errmsg' : 'no such command: \\'hello\\'',"
                          "  'code' : 59,"
                          "  'codeName' : 'CommandNotFound'"
                          "}";

   memset (&error, 0, sizeof (bson_error_t));
   mongoc_server_description_init (&sd, "host:1234", 1);
   BSON_ASSERT (sd.type == MONGOC_SERVER_UNKNOWN);
   mongoc_server_description_handle_hello (&sd, tmp_bson (response), 0, &error);
   BSON_ASSERT (sd.type == MONGOC_SERVER_UNKNOWN);

   mongoc_server_description_cleanup (&sd);
}

static void
test_server_description_legacy_hello (void)
{
   mongoc_server_description_t sd;
   bson_error_t error;
   bson_t hello_response;

   bson_init (&hello_response);
   BCON_APPEND (&hello_response, HANDSHAKE_RESPONSE_LEGACY_HELLO, BCON_BOOL (true));

   memset (&error, 0, sizeof (bson_error_t));
   mongoc_server_description_init (&sd, "host:1234", 1);
   BSON_ASSERT (sd.type == MONGOC_SERVER_UNKNOWN);
   mongoc_server_description_handle_hello (&sd, &hello_response, 0, &error);
   BSON_ASSERT (sd.type == MONGOC_SERVER_STANDALONE);
   BSON_ASSERT (!sd.hello_ok);

   mongoc_server_description_cleanup (&sd);
   bson_destroy (&hello_response);
}

static void
test_server_description_legacy_hello_ok (void)
{
   mongoc_server_description_t sd;
   bson_error_t error;
   bson_t hello_response;

   bson_init (&hello_response);
   BCON_APPEND (&hello_response, HANDSHAKE_RESPONSE_LEGACY_HELLO, BCON_BOOL (true));
   BCON_APPEND (&hello_response, "helloOk", BCON_BOOL (true));

   memset (&error, 0, sizeof (bson_error_t));
   mongoc_server_description_init (&sd, "host:1234", 1);
   BSON_ASSERT (sd.type == MONGOC_SERVER_UNKNOWN);
   mongoc_server_description_handle_hello (&sd, &hello_response, 0, &error);
   BSON_ASSERT (sd.type == MONGOC_SERVER_STANDALONE);
   BSON_ASSERT (sd.hello_ok);

   mongoc_server_description_cleanup (&sd);
   bson_destroy (&hello_response);
}

static void
test_server_description_connection_id (void)
{
   mongoc_server_description_t sd;
   bson_t *hello;
   bson_error_t error;

   // Test an int32.
   {
      mongoc_server_description_init (&sd, "host:1234", 1);
      hello = BCON_NEW ("minWireVersion",
                        BCON_INT32 (WIRE_VERSION_MIN),
                        "maxWireVersion",
                        BCON_INT32 (WIRE_VERSION_MAX),
                        "connectionId",
                        BCON_INT32 (1));
      memset (&error, 0, sizeof (bson_error_t));
      mongoc_server_description_handle_hello (&sd, hello, 0 /* rtt */, &error);
      BSON_ASSERT (sd.type == MONGOC_SERVER_STANDALONE);
      ASSERT_CMPINT64 (sd.server_connection_id, ==, 1);
      mongoc_server_description_cleanup (&sd);
      bson_destroy (hello);
   }
   // Test an int64.
   {
      mongoc_server_description_init (&sd, "host:1234", 1);
      hello = BCON_NEW ("minWireVersion",
                        BCON_INT32 (WIRE_VERSION_MIN),
                        "maxWireVersion",
                        BCON_INT32 (WIRE_VERSION_MAX),
                        "connectionId",
                        BCON_INT64 (1));
      memset (&error, 0, sizeof (bson_error_t));
      mongoc_server_description_handle_hello (&sd, hello, 0 /* rtt */, &error);
      BSON_ASSERT (sd.type == MONGOC_SERVER_STANDALONE);
      ASSERT_CMPINT64 (sd.server_connection_id, ==, 1);
      bson_destroy (hello);
      mongoc_server_description_cleanup (&sd);
   }
   // Test a double.
   {
      mongoc_server_description_init (&sd, "host:1234", 1);
      hello = BCON_NEW ("minWireVersion",
                        BCON_INT32 (WIRE_VERSION_MIN),
                        "maxWireVersion",
                        BCON_INT32 (WIRE_VERSION_MAX),
                        "connectionId",
                        BCON_DOUBLE (1));
      memset (&error, 0, sizeof (bson_error_t));
      mongoc_server_description_handle_hello (&sd, hello, 0 /* rtt */, &error);
      BSON_ASSERT (sd.type == MONGOC_SERVER_STANDALONE);
      ASSERT_CMPINT64 (sd.server_connection_id, ==, 1);
      bson_destroy (hello);
      mongoc_server_description_cleanup (&sd);
   }
}

static void
test_server_description_hello_type_error (void)
{
   mongoc_server_description_t sd;
   bson_error_t error;
   const char *hello = "{"
                       "  'ok' : { '$numberInt' : '1' },"
                       " 'ismaster' : true,"
                       " 'maxBsonObjectSize' : { '$numberInt' : '16777216' },"
                       " 'maxMessageSizeBytes' : { '$numberInt' : '48000000'},"
                       " 'maxWriteBatchSize' : { '$numberLong' : '565160423'},"
                       " 'logicalSessionTimeoutMinutes' : { '$numberInt' : '30'},"
                       " 'connectionId' : { '$numberLong' : '565160423'},"
                       " 'minWireVersion' : { '$numberInt' : '0'},"
                       " 'maxWireVersion' : { '$numberInt' : '15'},"
                       " 'readOnly' : true"
                       "}";
   mongoc_server_description_init (&sd, "host:1234", 1);
   memset (&error, 0, sizeof (bson_error_t));
   mongoc_server_description_handle_hello (&sd, tmp_bson (hello), 0, &error);
   BSON_ASSERT (sd.type == MONGOC_SERVER_UNKNOWN);
   BSON_ASSERT (sd.error.code == MONGOC_ERROR_STREAM_INVALID_TYPE);
   ASSERT_ERROR_CONTAINS (sd.error, MONGOC_ERROR_STREAM, MONGOC_ERROR_STREAM_INVALID_TYPE, "unexpected type");

   mongoc_server_description_cleanup (&sd);
}

static void
test_copy (const char *hello_json)
{
   mongoc_server_description_t sd, *sd_copy;
   mongoc_server_description_init (&sd, "host:1234", 1);
   bson_error_t empty_error = {0};
   mongoc_server_description_handle_hello (&sd, tmp_bson (hello_json), 0, &empty_error);
   sd_copy = mongoc_server_description_new_copy (&sd);

   // Check server descriptions compare equal by "Server Description Equality" rules. Not all fields are considered.
   ASSERT (_mongoc_server_description_equal (&sd, sd_copy));

   // Check all fields:
   ASSERT_CMPUINT32 (sd.id, ==, sd_copy->id);
   ASSERT_CMPSTR (sd.host.host_and_port, sd_copy->host.host_and_port);
   ASSERT_CMPINT64 (sd.round_trip_time_msec, ==, sd_copy->round_trip_time_msec);
   ASSERT_CMPINT64 (sd.last_update_time_usec, ==, sd_copy->last_update_time_usec);
   ASSERT_EQUAL_BSON (&sd.last_hello_response, &sd_copy->last_hello_response);
   ASSERT_CMPINT ((int) sd.has_hello_response, ==, (int) sd_copy->has_hello_response);
   ASSERT_CMPINT ((int) sd.hello_ok, ==, (int) sd_copy->hello_ok);
   ASSERT_CMPSTR (sd.connection_address, sd_copy->connection_address);
   ASSERT_CMPSTR (sd.me, sd_copy->me);
   ASSERT_CMPINT ((int) sd.opened, ==, (int) sd_copy->opened);
   ASSERT_CMPSTR (sd.set_name, sd_copy->set_name);
   ASSERT_MEMCMP (&sd.error, &sd_copy->error, (int) sizeof (bson_error_t));
   ASSERT_CMPINT ((int) sd.type, ==, (int) sd_copy->type);
   ASSERT_CMPINT32 (sd.min_wire_version, ==, sd_copy->min_wire_version);
   ASSERT_CMPINT32 (sd.max_wire_version, ==, sd_copy->max_wire_version);
   ASSERT_CMPINT32 (sd.max_msg_size, ==, sd_copy->max_msg_size);
   ASSERT_CMPINT32 (sd.max_bson_obj_size, ==, sd_copy->max_bson_obj_size);
   ASSERT_CMPINT32 (sd.max_write_batch_size, ==, sd_copy->max_write_batch_size);
   ASSERT_CMPINT64 (sd.session_timeout_minutes, ==, sd_copy->session_timeout_minutes);
   ASSERT_EQUAL_BSON (&sd.hosts, &sd_copy->hosts);
   ASSERT_EQUAL_BSON (&sd.passives, &sd_copy->passives);
   ASSERT_EQUAL_BSON (&sd.arbiters, &sd_copy->arbiters);
   ASSERT_EQUAL_BSON (&sd.tags, &sd_copy->tags);
   ASSERT_CMPSTR (sd.current_primary, sd_copy->current_primary);
   ASSERT_CMPINT64 (sd.set_version, ==, sd_copy->set_version);
   ASSERT_MEMCMP (&sd.election_id, &sd_copy->election_id, (int) sizeof (bson_oid_t));
   ASSERT_CMPINT64 (sd.last_write_date_ms, ==, sd_copy->last_write_date_ms);
   ASSERT_EQUAL_BSON (&sd.compressors, &sd_copy->compressors);
   ASSERT_EQUAL_BSON (&sd.topology_version, &sd_copy->topology_version);
   ASSERT_CMPUINT32 (sd.generation, ==, sd_copy->generation);
   ASSERT (sd_copy->_generation_map_ != NULL); // Do not compare entries. Just ensure non-NULL.
   ASSERT_MEMCMP (&sd.service_id, &sd_copy->service_id, (int) sizeof (bson_oid_t));
   ASSERT_CMPINT64 (sd.server_connection_id, ==, sd_copy->server_connection_id);

   mongoc_server_description_cleanup (&sd);
   mongoc_server_description_destroy (sd_copy);
}

static void
test_server_description_copy (void)
{
   const char *hello_mongod = BSON_STR ({
      "topologyVersion" : {"processId" : {"$oid" : "6792ef87965dee8797402adb"}, "counter" : 6},
      "hosts" : ["localhost:27017"],
      "setName" : "rs0",
      "setVersion" : 1,
      "isWritablePrimary" : true,
      "secondary" : false,
      "primary" : "localhost:27017",
      "me" : "localhost:27017",
      "electionId" : {"$oid" : "7fffffff0000000000000016"},
      "lastWrite" : {
         "opTime" : {"ts" : {"$timestamp" : {"t" : 1737682844, "i" : 1}}, "t" : 22},
         "lastWriteDate" : {"$date" : "2025-01-24T01:40:44Z"},
         "majorityOpTime" : {"ts" : {"$timestamp" : {"t" : 1737682844, "i" : 1}}, "t" : 22},
         "majorityWriteDate" : {"$date" : "2025-01-24T01:40:44Z"}
      },
      "maxBsonObjectSize" : 16777216,
      "maxMessageSizeBytes" : 48000000,
      "maxWriteBatchSize" : 100000,
      "localTime" : {"$date" : "2025-01-24T01:40:51.968Z"},
      "logicalSessionTimeoutMinutes" : 30,
      "connectionId" : 13,
      "minWireVersion" : 0,
      "maxWireVersion" : 25,
      "readOnly" : false,
      "ok" : 1.0,
      "$clusterTime" : {
         "clusterTime" : {"$timestamp" : {"t" : 1737682844, "i" : 1}},
         "signature" :
            {"hash" : {"$binary" : {"base64" : "AAAAAAAAAAAAAAAAAAAAAAAAAAA=", "subType" : "00"}}, "keyId" : 0}
      },
      "operationTime" : {"$timestamp" : {"t" : 1737682844, "i" : 1}}
   });

   const char *hello_mongos = BSON_STR ({
      "isWritablePrimary" : true,
      "msg" : "isdbgrid",
      "topologyVersion" : {"processId" : {"$oid" : "6791af1181771f367602ec40"}, "counter" : 0},
      "maxBsonObjectSize" : 16777216,
      "maxMessageSizeBytes" : 48000000,
      "maxWriteBatchSize" : 100000,
      "localTime" : {"$date" : "2025-01-24T01:24:57.217Z"},
      "logicalSessionTimeoutMinutes" : 30,
      "connectionId" : 3310,
      "maxWireVersion" : 25,
      "minWireVersion" : 0,
      "ok" : 1.0,
      "$clusterTime" : {
         "clusterTime" : {"$timestamp" : {"t" : 1737681896, "i" : 1}},
         "signature" :
            {"hash" : {"$binary" : {"base64" : "AAAAAAAAAAAAAAAAAAAAAAAAAAA=", "subType" : "00"}}, "keyId" : 0}
      },
      "operationTime" : {"$timestamp" : {"t" : 1737681896, "i" : 1}}
   });

   test_copy (hello_mongod);
   test_copy (hello_mongos);
}

void
test_server_description_install (TestSuite *suite)
{
   TestSuite_Add (suite, "/server_description/equal", test_server_description_equal);
   TestSuite_Add (suite, "/server_description/msg_without_isdbgrid", test_server_description_msg_without_isdbgrid);
   TestSuite_Add (suite, "/server_description/ignores_unset_rtt", test_server_description_ignores_rtt);
   TestSuite_Add (suite, "/server_description/hello", test_server_description_hello);
   TestSuite_Add (suite, "/server_description/hello_cmd_not_found", test_server_description_hello_cmd_not_found);
   TestSuite_Add (suite, "/server_description/legacy_hello", test_server_description_legacy_hello);
   TestSuite_Add (suite, "/server_description/legacy_hello_ok", test_server_description_legacy_hello_ok);
   TestSuite_Add (suite, "/server_description/connection_id", test_server_description_connection_id);
   TestSuite_Add (suite, "/server_description/hello_type_error", test_server_description_hello_type_error);
   TestSuite_Add (suite, "/server_description/copy", test_server_description_copy);
}
