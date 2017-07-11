/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
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

#include <cstdlib>
#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <fc/filesystem.hpp>
#include <fc/smart_ref_impl.hpp>   // required for gcc in release mode
#include <fc/string.hpp>
#include <fc/io/fstream.hpp>
#include <fc/io/json.hpp>
#include <muse/chain/genesis_state.hpp>
#include <muse/chain/protocol/types.hpp>

// we need to include the world in order to serialize fee_parameters

using namespace muse::chain;

static const char generated_file_banner[] =
"//                                   _           _    __ _ _        //\n"
"//                                  | |         | |  / _(_) |       //\n"
"//    __ _  ___ _ __   ___ _ __ __ _| |_ ___  __| | | |_ _| | ___   //\n"
"//   / _` |/ _ \\ '_ \\ / _ \\ '__/ _` | __/ _ \\/ _` | |  _| | |/ _ \\  //\n"
"//  | (_| |  __/ | | |  __/ | | (_| | ||  __/ (_| | | | | | |  __/  //\n"
"//   \\__, |\\___|_| |_|\\___|_|  \\__,_|\\__\\___|\\__,_| |_| |_|_|\\___|  //\n"
"//    __/ |                                                         //\n"
"//   |___/                                                          //\n"
"//                                                                  //\n"
"// Generated by:  libraries/chain_id/identify_chain.cpp             //\n"
"//                                                                  //\n"
"// Warning: This is a generated file, any changes made here will be //\n"
"// overwritten by the build process.  If you need to change what    //\n"
"// is generated here, you should use the CMake variable             //\n"
"// GRAPHENE_EGENESIS_JSON to specify an embedded genesis state.     //\n"
"//                                                                  //\n"
;

// hack:  import create_example_genesis() even though it's a way, way
// specific internal detail
namespace muse { namespace app { namespace detail {
genesis_state_type create_example_genesis();
} } } // graphene::app::detail

fc::path get_path(
   const boost::program_options::variables_map& options,
   const std::string& name )
{
   fc::path result = options[name].as<boost::filesystem::path>();
   if( result.is_relative() )
      result = fc::current_path() / result;
   return result;
}

void convert_to_c_array(
   const std::string& src,
   std::string& dest,
   int width = 40 )
{
   dest.reserve( src.length() * 6 / 5 );
   bool needs_comma = false;
   int row = 0;
   for( std::string::size_type i=0; i<src.length(); i+=width )
   {
      std::string::size_type j = std::min( i+width, src.length() );
      if( needs_comma )
         dest.append(",\n");
      dest.append("\"");
      for( std::string::size_type k=i; k<j; k++ )
      {
         char c = src[k];
         switch(c)
         {
            // use most short escape sequences
            case '\"': dest.append("\\\""); break;
            case '\?': dest.append("\\\?"); break;
            case '\\': dest.append("\\\\"); break;
            case '\a': dest.append( "\\a"); break;
            case '\b': dest.append( "\\b"); break;
            case '\f': dest.append( "\\f"); break;
            case '\n': dest.append( "\\n"); break;
            case '\r': dest.append( "\\r"); break;
            case '\t': dest.append( "\\t"); break;
            case '\v': dest.append( "\\v"); break;

            // single quote and misc. ASCII is OK
            case '\'':
            case ' ': case '!': case '#': case '$': case '%': case '&': case '(': case ')':
            case '*': case '+': case ',': case '-': case '.': case '/':
            case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            case ':': case ';': case '<': case '=': case '>': case '@':
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
            case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
            case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
            case '[': case ']': case '^': case '_': case '`':
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
            case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
            case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
            case '{': case '|': case '}': case '~':
               dest.append(&c, 1);
               break;

            // use shortest octal escape for everything else
            default:
               dest.append("\\");
               char dg[3];
               dg[0] = '0' + ((c >> 6) & 3);
               dg[1] = '0' + ((c >> 3) & 7);
               dg[2] = '0' + ((c     ) & 7);
               int start = (dg[0] == '0' ? (dg[1] == '0' ? 2 : 1) : 0);
               dest.append( dg+start, 3-start );
         }
      }
      dest.append("\"");
      needs_comma = true;
      row++;
   }
   std::cerr << "\n";
   return;
}

struct egenesis_info
{
   fc::optional< genesis_state_type > genesis;
   fc::optional< chain_id_type > chain_id;
   fc::optional< std::string > genesis_json;
   fc::optional< fc::sha256 > genesis_json_hash;
   fc::optional< std::string > genesis_json_array;
   int genesis_json_array_width,
       genesis_json_array_height;

   void fillin()
   {
      // must specify either genesis_json or genesis
      if( genesis.valid() )
      {
         if( !genesis_json.valid() )
            // If genesis_json not exist, generate from genesis
            genesis_json = fc::json::to_string( *genesis );
      }
      else if( genesis_json.valid() )
      {
         // If genesis not exist, generate from genesis_json
         genesis = fc::json::from_string( *genesis_json ).as< genesis_state_type >();
      }
      else
      {
         // Neither genesis nor genesis_json exists, crippled
         std::cerr << "embed_genesis:  Need genesis or genesis_json\n";
         exit(1);
      }
      // init genesis_json_hash from genesis_json
      if( !genesis_json_hash.valid() )
         genesis_json_hash = fc::sha256::hash( *genesis_json );
      // init chain_id from genesis_json_hash
      if( !chain_id.valid() )
         chain_id = genesis_json_hash;
      // init genesis_json_array from genesis_json
      if( !genesis_json_array.valid() )
      {
         genesis_json_array = std::string();
         // TODO: gzip
         int width = 40;
         convert_to_c_array( *genesis_json, *genesis_json_array, width );
         int height = (genesis_json->length() + width-1) / width;
         genesis_json_array_width = width;
         genesis_json_array_height = height;
      }
   }
};

void load_genesis(
   const boost::program_options::variables_map& options,
   egenesis_info& info
   )
{
   if( options.count("genesis-json") )
   {
      fc::path genesis_json_filename = get_path( options, "genesis-json" );
      std::cerr << "embed_genesis:  Reading genesis from file " << genesis_json_filename.preferred_string() << "\n";
      info.genesis_json = std::string();
      read_file_contents( genesis_json_filename, *info.genesis_json );
   }
   //else
     // *info.genesis = ""; //muse::app::detail::create_example_genesis();

   if( options.count("chain-id") )
   {
      std::string chain_id_str = options["chain-id"].as<std::string>();
      std::cerr << "embed_genesis:  Genesis ID from argument is " << chain_id_str << "\n";
      info.chain_id = chain_id_str;
   }
   return;
}

int main( int argc, char** argv )
{
   int main_return = 0;
   boost::program_options::options_description cli_options("Graphene Chain Identifier");
   cli_options.add_options()
      ("help,h", "Print this help message and exit.")
      ("genesis-json,g", boost::program_options::value<boost::filesystem::path>(), "File to read genesis state from")
      ("tmplsub,t", boost::program_options::value<std::vector< std::string > >()->composing(),
       "Given argument of form src.cpp.tmpl---dest.cpp, write dest.cpp expanding template invocations in src")
      ;

   boost::program_options::variables_map options;
   try
   {
      boost::program_options::store( boost::program_options::parse_command_line(argc, argv, cli_options), options );
   }
   catch (const boost::program_options::error& e)
   {
      std::cerr << "embed_genesis:  error parsing command line: " << e.what() << "\n";
      return 1;
   }

   if( options.count("help") )
   {
      std::cout << cli_options << "\n";
      return 0;
   }

   egenesis_info info;

   load_genesis( options, info );
   info.fillin();

   fc::mutable_variant_object template_context = fc::mutable_variant_object()
      ( "generated_file_banner", generated_file_banner )
      ( "chain_id", (*info.chain_id).str() )
      ;
   if( info.genesis_json.valid() )
   {
      template_context["genesis_json_length"] = info.genesis_json->length();
      template_context["genesis_json_array"] = (*info.genesis_json_array);
      template_context["genesis_json_hash"] = (*info.genesis_json_hash).str();
      template_context["genesis_json_array_width"] = info.genesis_json_array_width;
      template_context["genesis_json_array_height"] = info.genesis_json_array_height;
   }

   for( const std::string& src_dest : options["tmplsub"].as< std::vector< std::string > >() )
   {
      std::cerr << "embed_genesis:  parsing tmplsub parameter \"" << src_dest << "\"\n";
      size_t pos = src_dest.find( "---" );
      if( pos == std::string::npos )
      {
         std::cerr << "embed_genesis:  could not parse tmplsub parameter:  '---' not found\n";
         main_return = 1;
         continue;
      }
      std::string src = src_dest.substr( 0, pos );
      std::string dest = src_dest.substr( pos+3 );

      std::string tmpl;
      read_file_contents( fc::path( src ), tmpl );
      std::string out_str = fc::format_string( tmpl, template_context );
      fc::path dest_filename = fc::path( dest );
      fc::ofstream outfile( dest_filename );
      outfile.write( out_str.c_str(), out_str.size() );
      outfile.close();
   }

   return main_return;
}
