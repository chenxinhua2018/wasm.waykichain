#pragma once
#include <map>
#include <string>
#include <functional>
#include <utility>
#include <chrono>

#include "json/json_spirit.h"
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer.h"
#include "wasm/abi_def.hpp"
#include "wasm/datastream.hpp"
#include "wasm/types/types.hpp"

namespace wasm { 

using namespace json_spirit;
using namespace wasm;

using std::map;
using std::string;
using std::function;
using std::pair;

using std::chrono::microseconds;
using std::chrono::system_clock;

static auto max_serialization_time = microseconds(15*1000);

struct abi_traverse_context;

/**
 *  Describes the binary representation message and table contents so that it can
 *  be converted to and from JSON.
 */
struct abi_serializer {
   abi_serializer(){ configure_built_in_types(); }
   abi_serializer( const abi_def& abi, const microseconds& max_serialization_time );
   void set_abi( const abi_def& abi, const microseconds& max_serialization_time );

   type_name resolve_type(const type_name& t)const;
   bool      is_array(const type_name& type)const;
   bool      is_optional(const type_name& type)const;
   bool      is_type(const type_name& type, const microseconds& max_serialization_time )const;
   bool      is_builtin_type(const type_name& type)const;
   bool      is_integer(const type_name& type) const;
   int       get_integer_size(const type_name& type) const;
   bool      is_struct(const type_name& type)const;
   type_name fundamental_type(const type_name& type)const;

   const struct_def& get_struct(const type_name& type)const;

   type_name get_action_type(type_name action)const;
   type_name get_table_type(type_name action)const;

   bytes       variant_to_binary( const type_name& type, const json_spirit::Value& var, microseconds max_serialization_time ) const;
   void        variant_to_binary( const type_name& type, const json_spirit::Value&  var, wasm::datastream<char*>& ds, microseconds max_serialization_time)const;


   typedef std::function<json_spirit::Value(wasm::datastream<const char*>&, bool, bool)>  unpack_function;
   typedef std::function<void(const json_spirit::Value&, wasm::datastream<char*>&, bool, bool)>  pack_function;

   static const size_t max_recursion_depth = 32; // arbitrary depth to prevent infinite recursion

  static std::vector<char>  pack( const string& abi, const string& action, const string& params, microseconds max_serialization_time)
  {
   //try{
       json_spirit::Value abi_v;
       json_spirit::read_string(abi, abi_v);

       wasm::abi_def def;
       wasm::from_variant(abi_v, def);
       wasm::abi_serializer abis(def, max_serialization_time);

       json_spirit::Value data_v;
       json_spirit::read_string(params, data_v);
       std::cout << json_spirit::write(data_v) << std::endl ;
       vector<char> data = abis.variant_to_binary(action, data_v, max_serialization_time);   

       return data;
    //} WASM_CAPTURE_AND_RETHROW( "abi_serializer pack error in params %s", params.data() )  

  }

private:

   map<type_name, type_name>     typedefs;
   map<type_name, struct_def>    structs;
   map<type_name, type_name>      actions;
   map<type_name, type_name>      tables;
   map<uint64_t, string>         error_messages;
   //map<type_name, variant_def>   variants;

   map<type_name, pair<unpack_function, pack_function>> built_in_types;
   void configure_built_in_types();

   bytes       _variant_to_binary( const type_name& type, const json_spirit::Value&  var, wasm::abi_traverse_context& ctx ) const;
   void        _variant_to_binary( const type_name& type, const json_spirit::Value&  var, wasm::datastream<char *>& ds, wasm::abi_traverse_context& ctx ) const;

   static type_name _remove_bin_extension(const type_name& type);
   bool _is_type( const type_name& type, wasm::abi_traverse_context& ctx )const;

   void validate( wasm::abi_traverse_context& ctx )const;

};

struct abi_traverse_context {
   abi_traverse_context( std::chrono::microseconds max_serialization_time )
   : max_serialization_time( max_serialization_time ),
     deadline( system_clock::now() ), // init to now, updated below
     recursion_depth(0)
   {
      // if( max_serialization_time > microseconds::maximum() - deadline.time_since_epoch() ) {
      //    deadline = system_clock::maximum();
      // } else {
         deadline += max_serialization_time;
      //}
   }

   abi_traverse_context( std::chrono::microseconds max_serialization_time, system_clock::time_point deadline )
   : max_serialization_time( max_serialization_time ), deadline( deadline ), recursion_depth(0)
   {}

   void check_deadline()const;

   //fc::scoped_exit<std::function<void()>> enter_scope();
public:
   uint32_t       recursion_depth;
   std::chrono::microseconds max_serialization_time;
   std::chrono::system_clock::time_point   deadline;
   //size_t       recursion_depth;
};



}  // wasm

