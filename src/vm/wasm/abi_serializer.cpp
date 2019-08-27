#include <wasm/exceptions.hpp>
#include <wasm/abi_serializer.hpp>
#include <wasm/types/name.hpp>
#include <wasm/types/symbol.hpp>
#include <wasm/types/asset.hpp>
#include <wasm/types/varint.hpp>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>

#include "json/json_spirit_writer.h"

using namespace boost;
using namespace wasm;
using namespace std;

namespace wasm { 

   //const size_t abi_serializer::max_recursion_depth;

   using boost::algorithm::ends_with;
   using std::string;

   // template <typename T>
   // inline wasm::variant variant_from_stream(wasm::datastream<const char*>& stream) {
   //    T temp;
   //    wasm::unpack( stream, temp );
   //    return std::variant(temp);
   // }

   template <typename T>
   auto pack_unpack() {
      return std::make_pair<abi_serializer::unpack_function, abi_serializer::pack_function>(
         []( wasm::datastream<const char*>& stream, bool is_array, bool is_optional) -> json_spirit::Value  {
            // if( is_array )
            //    return variant_from_stream<vector<T>>(stream);
            // else if ( is_optional )
            //    return variant_from_stream<optional<T>>(stream);
            // return variant_from_stream<T>(stream);
            json_spirit::Value v;
            return v;

         },
         []( const json_spirit::Value& var, wasm::datastream<char*>& ds, bool is_array, bool is_optional ){
            if( is_array ){
               vector<T> ts;
               wasm::from_variant(var, ts);
               ds << ts;
            }else if ( is_optional ) {
               optional<T> opt;
               wasm::from_variant(var, opt);
               ds << opt;
            }else {
               //std::cout << "pack_unpack line 52" << std::endl ;

               T t;
               wasm::from_variant(var, t);
               ds << t;
            }
         }
      );
   }

   abi_serializer::abi_serializer( const abi_def& abi, const microseconds& max_serialization_time ) {
      configure_built_in_types();
      set_abi(abi, max_serialization_time);
   }

   // void abi_serializer::add_specialized_unpack_pack( const string& name,
   //                                                   std::pair<abi_serializer::unpack_function, abi_serializer::pack_function> unpack_pack ) {
   //    built_in_types[name] = std::move( unpack_pack );
   // }

   void abi_serializer::configure_built_in_types() {

      built_in_types.emplace("bool",                      pack_unpack<uint8_t>());
      built_in_types.emplace("int8",                      pack_unpack<int8_t>());
      built_in_types.emplace("uint8",                     pack_unpack<uint8_t>());
      built_in_types.emplace("int16",                     pack_unpack<int16_t>());
      built_in_types.emplace("uint16",                    pack_unpack<uint16_t>());
      built_in_types.emplace("int32",                     pack_unpack<int32_t>());
      built_in_types.emplace("uint32",                    pack_unpack<uint32_t>());
      built_in_types.emplace("int64",                     pack_unpack<int64_t>());
      built_in_types.emplace("uint64",                    pack_unpack<uint64_t>());
      // built_in_types.emplace("int128",                    pack_unpack<int128_t>());
      // built_in_types.emplace("uint128",                   pack_unpack<uint128_t>());
      built_in_types.emplace("varint32",                  pack_unpack<wasm::signed_int>());
      built_in_types.emplace("varuint32",                 pack_unpack<wasm::unsigned_int>());

      // TODO: Add proper support for floating point types. For now this is good enough.
      built_in_types.emplace("float32",                   pack_unpack<float>());
      built_in_types.emplace("float64",                   pack_unpack<double>());
      // built_in_types.emplace("float128",                  pack_unpack<uint128_t>());

      // built_in_types.emplace("time_point",                pack_unpack<std::time_point>());
      // built_in_types.emplace("time_point_sec",            pack_unpack<std::time_point_sec>());
      // built_in_types.emplace("block_timestamp_type",      pack_unpack<block_timestamp_type>());

      built_in_types.emplace("table_name",                      pack_unpack<name>());
      built_in_types.emplace("action_name",                      pack_unpack<name>());
      built_in_types.emplace("name",                      pack_unpack<name>());

      built_in_types.emplace("bytes",                     pack_unpack<bytes>());
      built_in_types.emplace("string",                    pack_unpack<string>());

      // built_in_types.emplace("checksum160",               pack_unpack<checksum160_type>());
      // built_in_types.emplace("checksum256",               pack_unpack<checksum256_type>());
      // built_in_types.emplace("checksum512",               pack_unpack<checksum512_type>());

      // built_in_types.emplace("public_key",                pack_unpack<public_key_type>());
      // built_in_types.emplace("signature",                 pack_unpack<signature_type>());

      built_in_types.emplace("symbol",                    pack_unpack<symbol>());
      built_in_types.emplace("symbol_code",               pack_unpack<symbol_code>());
      built_in_types.emplace("asset",                     pack_unpack<asset>());
      // built_in_types.emplace("extended_asset",            pack_unpack<extended_asset>());
   }

   void abi_serializer::set_abi( const abi_def& abi, const microseconds& max_serialization_time ) {
      wasm::abi_traverse_context ctx(max_serialization_time);

      WASM_ASSERT(boost::starts_with(abi.version, "wasm::abi/1."), UNSUPPORTED_ABI_VERSION_EXCEPTION, "UNSUPPORTED_ABI_VERSION_EXCEPTION", "ABI has an unsupported version");

      typedefs.clear();
      structs.clear();
      actions.clear();
      tables.clear();
      // error_messages.clear();
      // variants.clear();

      for( const auto& st : abi.structs )
         structs[st.name] = st;

      for( const auto& td : abi.types ) {
         WASM_ASSERT(_is_type(td.type, ctx), INVALID_TYPE_INSIDE_ABI, "INVALID_TYPE_INSIDE_ABI",  "invalid type %s", td.type.data());
         WASM_ASSERT(!_is_type(td.new_type_name, ctx), DUPLICATE_ABI_DEF_EXCEPTION,"DUPLICATE_ABI_DEF_EXCEPTION",  "type already exists %s", td.new_type_name.data());
         typedefs[td.new_type_name] = td.type;
      }

      for( const auto& a : abi.actions )
         actions[a.name] = a.type;

      for( const auto& t : abi.tables )
         tables[t.name] = t.type;

      for( const auto& e : abi.error_messages )
         error_messages[e.error_code] = e.error_msg;

      // for( const auto& v : abi.variants.value )
      //    variants[v.name] = v;

      /**
       *  The ABI vector may contain duplicates which would make it
       *  an invalid ABI
       */
      WASM_ASSERT( typedefs.size() == abi.types.size(), DUPLICATE_ABI_DEF_EXCEPTION,"DUPLICATE_ABI_DEF_EXCEPTION", "duplicate type definition detected" );
      WASM_ASSERT( structs.size() == abi.structs.size(), DUPLICATE_ABI_DEF_EXCEPTION, "DUPLICATE_ABI_DEF_EXCEPTION", "duplicate struct definition detected" );
      WASM_ASSERT( actions.size() == abi.actions.size(), DUPLICATE_ABI_DEF_EXCEPTION, "DUPLICATE_ABI_DEF_EXCEPTION", "duplicate action definition detected" );
      WASM_ASSERT( tables.size() == abi.tables.size(), DUPLICATE_ABI_DEF_EXCEPTION, "DUPLICATE_ABI_DEF_EXCEPTION", "duplicate table definition detected" );
      //WASM_ASSERT( error_messages.size() == abi.error_messages.size(), duplicate_abi_err_msg_def_exception, "duplicate error message definition detected" );
      //WASM_ASSERT( variants.size() == abi.variants.value.size(), duplicate_abi_variant_def_exception, "duplicate variant definition detected" );

      validate(ctx);
   }

   bool abi_serializer::is_builtin_type(const type_name& type)const {
      return built_in_types.find(type) != built_in_types.end();
   }

   bool abi_serializer::is_integer(const type_name& type) const {
      string stype = type;
      return boost::starts_with(stype, "uint") || boost::starts_with(stype, "int");
   }

   int abi_serializer::get_integer_size(const type_name& type) const {
      string stype = type;
      WASM_ASSERT( is_integer(type), INVALID_TYPE_INSIDE_ABI,"INVALID_TYPE_INSIDE_ABI", "%s is not an integer type", stype.data());
      if( boost::starts_with(stype, "uint") ) {
         return boost::lexical_cast<int>(stype.substr(4));
      } else {
         return boost::lexical_cast<int>(stype.substr(3));
      }
   }

   bool abi_serializer::is_struct(const type_name& type)const {
      return structs.find(resolve_type(type)) != structs.end();
   }

   bool abi_serializer::is_array(const type_name& type)const {
      return boost::ends_with(string(type), "[]");
   }

   bool abi_serializer::is_optional(const type_name& type)const {
      return boost::ends_with(string(type), "?");
   }

   bool abi_serializer::is_type(const type_name& type, const microseconds& max_serialization_time)const {
      wasm::abi_traverse_context ctx(max_serialization_time);
      return _is_type(type, ctx);
   }

   type_name abi_serializer::fundamental_type(const type_name& type)const {
      if( is_array(type) ) {
         return type_name(string(type).substr(0, type.size()-2));
      } else if ( is_optional(type) ) {
         return type_name(string(type).substr(0, type.size()-1));
      } else {
       return type;
      }
   }

   type_name abi_serializer::_remove_bin_extension(const type_name& type) {
      if( boost::ends_with(type, "$") )
         return type.substr(0, type.size()-1);
      else
         return type;
   }

   bool abi_serializer::_is_type(const type_name& rtype, wasm::abi_traverse_context& ctx )const {
      ctx.check_deadline();
      auto type = fundamental_type(rtype);
      if( built_in_types.find(type) != built_in_types.end() ) return true;
      if( typedefs.find(type) != typedefs.end() ) return _is_type(typedefs.find(type)->second, ctx);
      if( structs.find(type) != structs.end() ) return true;
      //if( variants.find(type) != variants.end() ) return true;
      return false;
   }

   const struct_def& abi_serializer::get_struct(const type_name& type)const {
      auto itr = structs.find(resolve_type(type) );
      WASM_ASSERT( itr != structs.end(), INVALID_TYPE_INSIDE_ABI, "INVALID_TYPE_INSIDE_ABI","Unknown struct %s", type.data()) ;
      return itr->second;
   }

   type_name abi_serializer::resolve_type(const type_name& type)const {
      auto itr = typedefs.find(type);
      if( itr != typedefs.end() ) {
         for( auto i = typedefs.size(); i > 0; --i ) { // avoid infinite recursion
            const type_name& t = itr->second;
            itr = typedefs.find( t );
            if( itr == typedefs.end() ) return t;
         }
      }
      return type;
   }


 inline auto GetFieldVariant(const json_spirit::Value& v, field_name field, uint32_t index ){
      if (v.type() == json_spirit::obj_type) { 
         //std::cout << "GetFieldValue: field " << field << std::endl ;
         auto o = v.get_obj();
         for( json_spirit::Object::const_iterator iter = o.begin(); iter != o.end(); ++iter )
         { 
            string name = Config_type::get_name(*iter);
            if(name == field){
              return Config_type::get_value(*iter);
            }
         } 
      }

      WASM_THROW( INVALID_TYPE_INSIDE_ABI, "INVALID_TYPE_INSIDE_ABI", "Unknown %s", field.data()) ;

      json_spirit::Value var;
      return var;

  }

   void abi_serializer::_variant_to_binary( const type_name& type, const json_spirit::Value& var, wasm::datastream<char *>& ds, wasm::abi_traverse_context& ctx )const
   { 
      ctx.check_deadline();
      //std::cout << "[recursion_depth:" << ctx.recursion_depth <<" type:" << type << "]"<< std::endl ;
      ctx.recursion_depth ++;
      try {
            type_name rtype = resolve_type(type);

            auto s_itr = structs.end();
            auto btype = built_in_types.find(fundamental_type(rtype));
            if( btype != built_in_types.end() ) {
                // std::cout  << "[built_in_types]" << btype->first << std::endl;
                btype->second.second(var, ds, is_array(rtype), is_optional(rtype));
            } else if ( is_array(rtype) ) {
               auto t = var.get_array();
               ds << (wasm::unsigned_int) t.size();
               for( json_spirit::Array::const_iterator iter = t.begin(); iter!= t.end(); ++iter )
               { 
                  _variant_to_binary(fundamental_type(rtype), *iter, ds, ctx );
                }              
            } else if( (s_itr = structs.find(rtype)) != structs.end() ) {
                const auto& st = s_itr->second;
                if (var.type() == json_spirit::obj_type ) {
                   
                  if( st.base != type_name() ) {
                      _variant_to_binary(resolve_type(st.base), var, ds, ctx);
                  }
                   auto& vo = var.get_obj();
                   for( uint32_t i = 0; i < st.fields.size(); ++i ) {
                       // std::cout << "---------------------------------------------- " << std::endl ;
                       const auto& field = st.fields[i];
                       auto v = GetFieldVariant(vo, field.name, i);
                       //std::cout << "[GetFieldVariant]" << field.name << std::endl;
                       _variant_to_binary(_remove_bin_extension(field.type), v, ds, ctx);
                    }
                // } else if (var.type() == json_spirit::array_type){
                //     std::cout << "struct in array ===============================================" << rtype.data() << std::endl;
                //     const auto& va = var.get_array();
                //     for( uint32_t i = 0; i < st.fields.size(); ++i ) {
                //         const auto& field = st.fields[i];
                //         if( va.size() > i ) {
                //            _variant_to_binary(_remove_bin_extension(field.type), va[i], ds, ctx);
                //         }else
                //         {
                //            WASM_THROW( INVALID_TYPE_INSIDE_ABI, "Early end to input array specifying the fields of struct '%s'; require input for field '%s'", type.data(), field.name.data()) ;
                //         }
                //      }                 
                // }
                } else {
                  WASM_THROW( INVALID_TYPE_INSIDE_ABI, "INVALID_TYPE_INSIDE_ABI", "Unknown type %s", type.data()) ;
                }

            } else {
              WASM_THROW( INVALID_TYPE_INSIDE_ABI, "INVALID_TYPE_INSIDE_ABI", "Unknown type %s", type.data()) ;
            }
      } WASM_CAPTURE_AND_RETHROW ("can not convert %s to %s", type.data(), json_spirit::write(var).data())

   }

   bytes abi_serializer::_variant_to_binary( const type_name& type, const json_spirit::Value& var, wasm::abi_traverse_context& ctx )const
   {
      ctx.check_deadline();

      if( !_is_type(type, ctx) ) {
         bytes b;
         return b;
      }

      bytes temp( 1024*1024 );
      wasm::datastream<char*> ds(temp.data(), temp.size() );
      _variant_to_binary(type, var, ds, ctx);
      temp.resize(ds.tellp());
      return temp;
  }

   bytes abi_serializer::variant_to_binary( const type_name& type, const json_spirit::Value&  var, microseconds max_serialization_time )const {
      wasm::abi_traverse_context ctx(max_serialization_time);
      return _variant_to_binary(type, var, ctx);
   }

   void  abi_serializer::variant_to_binary( const type_name& type, const json_spirit::Value&  var, wasm::datastream<char*>& ds, microseconds max_serialization_time )const {
      wasm::abi_traverse_context ctx(max_serialization_time);
      _variant_to_binary(type, var, ds, ctx );
   }



   type_name abi_serializer::get_action_type(type_name action)const {
      auto itr = actions.find(action);
      if( itr != actions.end() ) return itr->second;
      return type_name();
   }
   type_name abi_serializer::get_table_type(type_name action)const {
      auto itr = tables.find(action);
      if( itr != tables.end() ) return itr->second;
      return type_name();
   }

   // optional<string> abi_serializer::get_error_message( uint64_t error_code )const {
   //    auto itr = error_messages.find( error_code );
   //    if( itr == error_messages.end() )
   //       return optional<string>();

   //    return itr->second;
   // }

    void abi_serializer::validate( wasm::abi_traverse_context& ctx )const {
      for( const auto& t : typedefs ) { try {
         vector<type_name> types_seen{t.first, t.second};
         auto itr = typedefs.find(t.second);
         while( itr != typedefs.end() ) {
            ctx.check_deadline();
            WASM_ASSERT( find(types_seen.begin(), types_seen.end(), itr->second) == types_seen.end(), ABI_CIRCULAR_DEF_EXCEPTION, "INVALID_TYPE_INSIDE_ABI", "Circular reference in type %s", itr->second.data() );
            types_seen.emplace_back(itr->second);
            itr = typedefs.find(itr->second);
         }
      } WASM_CAPTURE_AND_RETHROW( "Unknown new type %s", t.first.data() ) }

      for( const auto& t : typedefs ) { try {
         WASM_ASSERT(_is_type(t.second, ctx), INVALID_TYPE_INSIDE_ABI,"INVALID_TYPE_INSIDE_ABI", "Unknown type %s", t.second.data() );
      } WASM_CAPTURE_AND_RETHROW( "Unknown type %s", t.second.data() ) }

      for( const auto& s : structs ) { try {
         if( s.second.base != type_name() ) {
            struct_def current = s.second;
            vector<type_name> types_seen{current.name};
            while( current.base != type_name() ) {
               ctx.check_deadline();
               const auto& base = get_struct(current.base); //<-- force struct to inherit from another struct
               WASM_ASSERT( find(types_seen.begin(), types_seen.end(), base.name) == types_seen.end(), ABI_CIRCULAR_DEF_EXCEPTION, "INVALID_TYPE_INSIDE_ABI", "Circular reference in struct %s", s.second.name.data() );
               types_seen.emplace_back(base.name);
               current = base;
            }
         }
         for( const auto& field : s.second.fields ) { try {
            ctx.check_deadline();
            WASM_ASSERT(_is_type(_remove_bin_extension(field.type), ctx), INVALID_TYPE_INSIDE_ABI, "INVALID_TYPE_INSIDE_ABI", "invalid type inside abi in type %s", field.type.data() );
         } WASM_CAPTURE_AND_RETHROW( "parse error in struct %s field %s", s.first.data(), field.type.data() ) }  
      } WASM_CAPTURE_AND_RETHROW( "parse error in struct %s", s.first.data() ) }

      for( const auto& a : actions ) { try {
        ctx.check_deadline();
        WASM_ASSERT(_is_type(a.second, ctx), INVALID_TYPE_INSIDE_ABI, "INVALID_TYPE_INSIDE_ABI", "invalid type inside abi in action %s", a.second.data() );
      } WASM_CAPTURE_AND_RETHROW( "action %s error", a.first.data() ) }

      for( const auto& t : tables ) { try {
        ctx.check_deadline();
        WASM_ASSERT(_is_type(t.second, ctx), INVALID_TYPE_INSIDE_ABI, "INVALID_TYPE_INSIDE_ABI", "invalid type inside abi in table %s", t.second.data() );
      } WASM_CAPTURE_AND_RETHROW( "table %s error", t.first.data() ) }
    }


   void abi_traverse_context::check_deadline()const {
         WASM_ASSERT( system_clock::now() < deadline, ABI_SERIALIZATION_DEADLINE_EXCEPTION," ABI_SERIALIZATION_DEADLINE_EXCEPTION",
                     "serialization time limit %ldus exceeded", max_serialization_time.count() );
                     //"serialization time limit %sus exceeded", "1000" );
   }


   


}
