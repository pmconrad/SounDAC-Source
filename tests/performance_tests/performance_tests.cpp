/*
 * Cop-yright (c) 2019 Peertracks, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <boost/test/unit_test.hpp>

#include <muse/chain/database.hpp>
#include <muse/chain/protocol/muse_operations.hpp>

#include "../common/database_fixture.hpp"
#include <cstdlib>
#include <iostream>

using namespace muse::chain;

BOOST_FIXTURE_TEST_SUITE( performance_tests, clean_database_fixture )

BOOST_AUTO_TEST_CASE( reporting_benchmark )
{ try {
   ACTORS( (alice)(speedtracks) );
   fund( "alice", 1000000000 );
   vest( "alice",  100000000 );
   fund( "speedtracks", 1000000000 );
   vest( "speedtracks",  900000000 );

   trx.clear();

   const uint32_t NUM_BANDS = 100000;
   const uint32_t NUM_SONGS = 10 * NUM_BANDS + 1;
   const uint32_t NUM_LISTENERS = 2 * NUM_BANDS + 1;
   std::vector<std::string> bands;
   bands.reserve(NUM_BANDS);
   BOOST_TEST_MESSAGE( "Creating " + fc::to_string(NUM_BANDS) + " band accounts" );
   {
      account_create_operation aco;
      aco.fee = asset( 1 );
      aco.creator = "alice";
      aco.owner = authority( 1, alice_public_key, 1 );
      aco.active = authority( 1, alice_public_key, 1 );
      aco.basic = authority( 1, alice_public_key, 1 );
      aco.memo_key = alice_public_key;
      for( uint32_t i = 1; i <= NUM_BANDS; ++i )
      {
         aco.new_account_name = "vanom-" + fc::to_string(i);
         trx.operations.push_back( aco );
         bands.emplace_back( std::move(aco.new_account_name) );
         if( i%100 == 0 )
         {
            trx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
            sign( trx, alice_private_key );
            db.push_transaction( trx, 0 );
            generate_block();
            trx.clear();
         }
      }
      if( !trx.operations.empty() )
      {
         sign( trx, alice_private_key );
         db.push_transaction( trx, 0 );
         trx.clear();
      }
   }
   db.get_account( "vanom-" + fc::to_string(NUM_BANDS) );

   std::vector<std::string> fans;
   fans.reserve(NUM_LISTENERS);
   BOOST_TEST_MESSAGE( "Creating " + fc::to_string(NUM_LISTENERS) + " listener accounts" );
   {
      account_create_operation aco;
      aco.fee = asset( 1 );
      aco.creator = "alice";
      aco.owner = authority( 1, alice_public_key, 1 );
      aco.active = authority( 1, alice_public_key, 1 );
      aco.basic = authority( 1, alice_public_key, 1 );
      aco.memo_key = alice_public_key;
      for( uint32_t i = 1; i <= NUM_LISTENERS; ++i )
      {
         aco.new_account_name = "fan-" + fc::to_string(i);
         trx.operations.push_back( aco );
         fans.emplace_back( std::move(aco.new_account_name) );
         if( i%100 == 0 )
         {
            trx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
            sign( trx, alice_private_key );
            db.push_transaction( trx, 0 );
            generate_block();
            trx.clear();
         }
      }
      if( !trx.operations.empty() )
      {
         sign( trx, alice_private_key );
         db.push_transaction( trx, 0 );
         trx.clear();
      }
   }
   db.get_account( "fan-" + fc::to_string(NUM_LISTENERS) );

   std::vector<std::string> songs;
   songs.reserve(NUM_SONGS);
   BOOST_TEST_MESSAGE( "Creating " + fc::to_string(NUM_SONGS) + " songs" );
   {
      content_operation cco;
      cco.uploader = "alice";
      cco.album_meta.album_title = "Best of Metalbang";
      cco.comp_meta.third_party_publishers = false;
      management_vote mgmt;
      mgmt.voter = "alice";
      mgmt.percentage = 100;
      cco.management.push_back( mgmt );
      cco.management_threshold = 100;
      distribution dist;
      dist.bp = 6000;
      cco.distributions.push_back( dist );
      dist.bp = 4000;
      cco.distributions.push_back( dist );
      for( uint32_t i = 1; i <= NUM_SONGS; ++i )
      {
         cco.url = "ipfs://metalbang." + fc::to_string(i);
         cco.track_meta.track_title = "Metalbang Remix #" + fc::to_string(i);
         cco.distributions[0].payee = bands[i % NUM_BANDS];
         cco.distributions[1].payee = bands[(i+1) % NUM_BANDS];
         trx.operations.push_back( cco );
         songs.emplace_back( std::move(cco.url) );
         if( i%100 == 0 )
         {
            trx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
            sign( trx, alice_private_key );
            db.push_transaction( trx, 0 );
            generate_block();
            trx.clear();
         }
      }
      if( !trx.operations.empty() )
      {
         sign( trx, alice_private_key );
         db.push_transaction( trx, 0 );
         trx.clear();
      }
   }
   db.get_content( "ipfs://metalbang." + fc::to_string(NUM_SONGS) );

   {
      streaming_platform_update_operation spuo;
      spuo.fee = asset( MUSE_MIN_STREAMING_PLATFORM_CREATION_FEE, MUSE_SYMBOL );
      spuo.owner = "speedtracks";
      spuo.url = "http://www.speedtracks.com";
      trx.operations.push_back( spuo );
      sign( trx, speedtracks_private_key );
      db.push_transaction( trx, 0 );
      trx.clear();
   }

   uint32_t reports_per_second = 1;
   const uint32_t blocks_per_day = 28800;
   const uint32_t seconds_per_block = 86400 / blocks_per_day;
   const uint32_t skip_blocks = seconds_per_block / 3 - 1;
   static_assert( 3 * (skip_blocks + 1) * blocks_per_day == 86400 );
   streaming_platform_report_operation spro;
   spro.streaming_platform = "speedtracks";
   spro.play_time = 10;
   uint32_t counter = 0;
   const fc::ecc::private_key& witness_key = generate_private_key("init_key");
   while( reports_per_second < 257 ) // we can squeeze about 1000 ops into one block
   {
      for( uint32_t phase = 1; phase <= 2; ++phase ) // 1=ramp-up, 2=sustained
      {
         auto start = fc::time_point::now();
         for( uint32_t i = 0; i < blocks_per_day; ++i )
         {
            for( uint32_t j = 0; j < seconds_per_block * reports_per_second; ++j )
            {
               spro.consumer = fans[counter % NUM_LISTENERS];
               spro.content  = songs[counter % NUM_SONGS];
               trx.operations.push_back( spro );
               ++counter;
            }
            trx.set_expiration( db.head_block_time() + MUSE_MAX_TIME_UNTIL_EXPIRATION );
            //sign( trx, speedtracks_private_key );
	    try
	    {
               db.push_transaction( trx, ~0 );
	    }
	    catch( const fc::assert_exception& e )
	    {
               wlog( "TX size exceeded?! tx with ${ops} ops has ${b} bytes.",
		     ("ops",trx.operations.size())
		     ("b",fc::raw::pack_to_vector(trx,50).size()) );
	       throw;
	    }
            generate_block( ~0, witness_key, skip_blocks );
            trx.clear();
         }
         auto end = fc::time_point::now();
         auto elapsed = end - start;
         wlog( "1 day of streaming at ${rps} took ${t}ms in phase ${p}",
               ("rps",reports_per_second)("t",elapsed.count()/1000)("p",phase) );
      }
      reports_per_second *= 4;
   }
} FC_LOG_AND_RETHROW() }

BOOST_AUTO_TEST_SUITE_END()
