#pragma once

#include "wasm/types/types.hpp"
#include "wasm/types/uint128.hpp"
#include "wasm/types/varint.hpp"
#include "wasm/types/name.hpp"
#include "wasm/types/asset.hpp"
// #include "json/json_spirit.h"
#include "json/json_spirit_value.h"

namespace wasm { 
using namespace json_spirit;
typedef json_spirit::Config::Value_type::Config_type Config_type;

enum ABI_Enum
{
    ID_Comment = 0,
    ID_Version,
    ID_Structs,
    ID_Types,
    ID_Actions,
    ID_Tables,
    ID_Ricardian_clauses,
    ID_Variants,
    ID_Abi_extensions,
    ID_Name,
    ID_Base,
    ID_Fields,
    ID_Type,
    ID_Ricardian_contract,
    ID_ID,
    ID_Body,
    ID_Index_type,
    ID_Key_names,
    ID_Key_types,
    ID_New_type_name,
    ID_Symbolcode,
    ID_Precision,
    ID_Symbol,
    ID_Amount,
};

static std::map<std::string, ABI_Enum> mapStringValues = {
   {"____comment" , ID_Comment},
   {"version" , ID_Version},
   {"structs" , ID_Structs},
   {"types" , ID_Types},
   {"actions" , ID_Actions},
   {"tables" , ID_Tables},
   {"ricardian_clauses" , ID_Ricardian_clauses},
   {"variants" , ID_Variants},
   {"abi_extensions" , ID_Abi_extensions},
   {"name" , ID_Name},
   {"base" , ID_Base},
   {"fields" , ID_Fields},
   {"type" , ID_Type},
   {"ricardian_contract" , ID_Ricardian_contract},
   {"id" , ID_ID},
   {"body" , ID_Body},
   {"index_type" , ID_Index_type},
   {"key_names" , ID_Key_names},
   {"types" , ID_Key_types},
   {"new_type_name" , ID_New_type_name},
   {"symbol_code" , ID_Symbolcode},
   {"precision" , ID_Precision},
   {"sym" , ID_Symbol},
   {"symbol" , ID_Symbol},
   {"amount" , ID_Amount},
};

inline void from_variant( const json_spirit::Value& v, wasm::name& t ){
     if (v.type() == json_spirit::str_type) {              
         t = wasm::name(v.get_str());

         std::cout << "name:" << t.to_string() << std::endl ;
     } 
}


inline void from_variant( const json_spirit::Value& v, std::string& t ){
     if (v.type() == json_spirit::str_type) {              
         t = v.get_str();
     } 
}

inline void from_variant( const json_spirit::Value& v, wasm::bytes& t ){
     if (v.type() == json_spirit::str_type) {              
         string str = v.get_str();
         t.insert(t.begin(), str.begin(), str.end());
     } 
}

inline void from_variant( const json_spirit::Value& v, wasm::signed_int& t ){
     if (v.type() == json_spirit::int_type) {              
         t = static_cast< int32_t >(v.get_int64());
     } 
}

inline void from_variant( const json_spirit::Value& v, wasm::unsigned_int& t ){
     if (v.type() == json_spirit::int_type) {              
         t = static_cast< uint32_t >(v.get_int64());
     } 
}


inline void from_variant( const json_spirit::Value& v, bool& t ){
     if (v.type() == json_spirit::bool_type) {              
         t = v.get_bool();
     } 
}

//floating_point
// inline void from_variant( const json_spirit::Value& v, double& t ){
//      if (v.type() == json_spirit::real_type) {              
//          t = v.get_real();
//      } 
// }

// inline void from_variant( const json_spirit::Value& v, float& t ){
//      if (v.type() == json_spirit::real_type) {              
//          t = static_cast< float >((v.get_real()));
//      } 
// }
template<typename T, std::enable_if_t<std::is_floating_point<T>::value>* = nullptr>
inline void from_variant( const json_spirit::Value& v, T& t ){
     if (v.type() == json_spirit::int_type) {              
        t = v.get_real();
     } 
}

inline string VectorToHexString(std::vector<char> str, string separator = " ")
{

    const std::string hex = "0123456789abcdef";
    std::stringstream ss;
 
    for (std::string::size_type i = 0; i < str.size(); ++i)
        ss << hex[(unsigned uint8_t)str[i] >> 4] << hex[(unsigned uint8_t)str[i] & 0xf] << separator;

    return ss.str();

}


template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
inline void from_variant( const json_spirit::Value& v, T& t ){
     if (v.type() == json_spirit::int_type) {              
         t = v.get_int64();
     } 
}

//number
// inline void from_variant( const json_spirit::Value& v, uint8_t& t ){
//      //if (v.type() == json_spirit::int_type) {   
  
//          char* p = (char *)(v.get_int64());
//          std::vector<char> vec;
//          for(int i =0 ; i < 8; i ++ ){
//             vec.push_back(*p);
//             p++;
//          }
//          VectorToHexString(vec);
 
//          t = v.get_int64();
//          printf("-------1:%d-----------size:%d\n", t, vec.size());

//          int64_t t2 = t;

        
//         std::cout << std::hex <<"precision:" <<  t2 << std::endl ;
//      //} 
// }


// inline void from_variant( const json_spirit::Value& v, int8_t& t ){
//      if (v.type() == json_spirit::int_type) {              
//          t = static_cast< int8_t >(v.get_int64());
//      } 
// }

// inline void from_variant( const json_spirit::Value& v, uint16_t& t ){
//      if (v.type() == json_spirit::int_type) {              
//          t = static_cast< uint16_t >(v.get_int64());
//      } 
// }
// inline void from_variant( const json_spirit::Value& v, int16_t& t ){
//      if (v.type() == json_spirit::int_type) {              
//          t = static_cast< int16_t >(v.get_int64());
//      } 
// }

// inline void from_variant( const json_spirit::Value& v, uint32_t& t ){
//      if (v.type() == json_spirit::int_type) {              
//          t = static_cast< uint32_t >(v.get_int64());
//      } 
// }
// inline void from_variant( const json_spirit::Value& v, int32_t& t ){
//      if (v.type() == json_spirit::int_type) {              
//          t = static_cast< int32_t >(v.get_int64());
//      } 
// }

// inline void from_variant( const json_spirit::Value& v, uint64_t& t ){
//      if (v.type() == json_spirit::int_type) {              
//          t = static_cast< uint64_t >(v.get_int64());
//      } 
// }
// inline void from_variant( const json_spirit::Value& v, int64_t& t ){
//      if (v.type() == json_spirit::int_type) {              
//          t = v.get_int64();
//      } 
// }

inline void from_variant( const json_spirit::Value& v, wasm::symbol_code& t ){
     if (v.type() == json_spirit::str_type) {              
         t = wasm::symbol_code(v.get_str());
     } 
}

inline void from_variant( const json_spirit::Value& v, wasm::symbol& t ){
     if (v.type() == json_spirit::obj_type) {   
        wasm::symbol_code symbol_code;
        uint8_t precision;
        auto o = v.get_obj();
        for( json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i )
        {   
            string key = Config_type::get_name(*i);
            ABI_Enum abi_key = mapStringValues[key];
            switch (abi_key)
            {
               case ID_Symbolcode : 
               {
                    from_variant(Config_type::get_value(*i), symbol_code);
                    //std::cout << symbol_code <<std::endl ;
                    break;
               }
               case ID_Precision : 
               {
                   from_variant(Config_type::get_value(*i), precision);
                   //std::cout << "precision line203" <<std::endl ;
                   break;
                }
               default : break;
            }            
        }
        t = wasm::symbol(symbol_code, precision);

        std::cout << " symbol_code:" << symbol_code.to_string()
                  << " precision:" << precision
                  << std::endl ;

     } 
}

inline void from_variant( const json_spirit::Value& v, wasm::asset& t ){
     if (v.type() == json_spirit::obj_type) {              
        auto o = v.get_obj();
        for( json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i )
        {   
            string key = Config_type::get_name(*i);
            ABI_Enum abi_key = mapStringValues[key];
            switch (abi_key)
            {
               case ID_Amount : 
               {
                    from_variant(Config_type::get_value(*i), t.amount);
                    //std::cout << t.amount <<std::endl ;
                    break;
               }
               case ID_Symbol : 
               {
                   from_variant(Config_type::get_value(*i), t.sym);
                   //std::cout << t.sym.to_string() <<std::endl ;
                   break;
                }
               default : break;
            }            
        }

        std::cout << "asset:" << t.to_string() << std::endl ;


    } 
}


template<typename T>
inline void from_variant( const json_spirit::Value& v, vector<T>& ts ){
     if (v.type() == json_spirit::array_type) {  
    
        auto a = v.get_array();
        for( json_spirit::Array::const_iterator i = a.begin(); i != a.end(); ++i )
        {    
            T t;
            from_variant( *i, t );
            ts.push_back(t);
        }

     } 

}

template<typename T>
inline void from_variant( const json_spirit::Value& v, optional<T>& opt ){

     if(v.type() == json_spirit::null_type) {
         return;
      }

      T t;
      from_variant(v, t);
      opt = t;
}

} //wasm


