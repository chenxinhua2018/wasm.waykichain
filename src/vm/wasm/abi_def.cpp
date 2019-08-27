#include <wasm/abi_def.hpp>
#include "json/json_spirit.h"
#include "json/json_spirit_value.h"



namespace wasm { 
using namespace json_spirit;
using namespace wasm;


 void from_variant( const json_spirit::Value& v, std::string& t ){
     if (v.type() == json_spirit::str_type) {              
         t = v.get_str();
     } 
}
 void from_variant( const json_spirit::Value& v, wasm::name& t ){
     if (v.type() == json_spirit::str_type) {              
         t = wasm::name(v.get_str());

         std::cout << "name:" << t.to_string() << std::endl ;
     } 
}

 void from_variant( const json_spirit::Value& v, wasm::bytes& t ){
     if (v.type() == json_spirit::str_type) {              
         string str = v.get_str();
         t.insert(t.begin(), str.begin(), str.end());
     } 
}
 void from_variant( const json_spirit::Value& v, wasm::signed_int& t ){
     if (v.type() == json_spirit::int_type) {              
         t = static_cast< int32_t >(v.get_int64());
     } 
}
 void from_variant( const json_spirit::Value& v, wasm::unsigned_int& t ){
     if (v.type() == json_spirit::int_type) {              
         t = static_cast< uint32_t >(v.get_int64());
     } 
}

 void from_variant( const json_spirit::Value& v, bool& t ){
     if (v.type() == json_spirit::bool_type) {              
         t = v.get_bool();
     } 
}

void from_variant( const json_spirit::Value& v, double& t ){
     if (v.type() == json_spirit::real_type) {              
         t = v.get_real();
     } 
}
 void from_variant( const json_spirit::Value& v, float& t ){
     if (v.type() == json_spirit::real_type) {              
         t = static_cast< float >((v.get_real()));
     } 
}
// template<typename T, std::enable_if_t<std::is_floating_point<T>::value>* = nullptr>
//  void from_variant( const json_spirit::Value& v, T& t ){
//      if (v.type() == json_spirit::int_type) {              
//         t = v.get_real();
//      } 
// }
string VToHexString(std::vector<char> str, string separator = " ")
{

    const std::string hex = "0123456789abcdef";
    std::stringstream ss;
 
    for (std::string::size_type i = 0; i < str.size(); ++i)
        ss << hex[(unsigned uint8_t)str[i] >> 4] << hex[(unsigned uint8_t)str[i] & 0xf] << separator;

    return ss.str();

}


// template<typename T, std::enable_if_t<std::is_integral<T>::value>* = nullptr>
//  void from_variant( const json_spirit::Value& v, T& t ){
//      if (v.type() == json_spirit::int_type) {              
//          t = v.get_int64();
//      } 
// }
 void from_variant( const json_spirit::Value& v, uint8_t& t ){
     if (v.type() == json_spirit::int_type) {   
  
         t = v.get_int64();
         printf("-------1:%d-----------size:%d\n", t);

         int64_t t2 = t;

        
         std::cout << std::hex <<"precision:" <<  t << std::endl ;
     } 
}

 void from_variant( const json_spirit::Value& v, int8_t& t ){
     if (v.type() == json_spirit::int_type) {              
         t = static_cast< int8_t >(v.get_int64());
     } 
}
 void from_variant( const json_spirit::Value& v, uint16_t& t ){
     if (v.type() == json_spirit::int_type) {              
         t = static_cast< uint16_t >(v.get_int64());
     } 
} void from_variant( const json_spirit::Value& v, int16_t& t ){
     if (v.type() == json_spirit::int_type) {              
         t = static_cast< int16_t >(v.get_int64());
     } 
}
 void from_variant( const json_spirit::Value& v, uint32_t& t ){
     if (v.type() == json_spirit::int_type) {              
         t = static_cast< uint32_t >(v.get_int64());
     } 
} void from_variant( const json_spirit::Value& v, int32_t& t ){
     if (v.type() == json_spirit::int_type) {              
         t = static_cast< int32_t >(v.get_int64());
     } 
}
 void from_variant( const json_spirit::Value& v, uint64_t& t ){
     if (v.type() == json_spirit::int_type) {              
         t = static_cast< uint64_t >(v.get_int64());
     } 
} void from_variant( const json_spirit::Value& v, int64_t& t ){
     if (v.type() == json_spirit::int_type) {              
         t = v.get_int64();
     } 
}

template<typename T> void from_variant( const json_spirit::Value& v, vector<T>& ts ){
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

template<typename T> void from_variant( const json_spirit::Value& v, optional<T>& opt ){

     if(v.type() == json_spirit::null_type) {
         return;
      }

      T t;
      from_variant(v, t);
      opt = t;
}

 void from_variant( const json_spirit::Value& v, wasm::symbol_code& t ){
     if (v.type() == json_spirit::str_type) {              
         t = wasm::symbol_code(v.get_str());
     } 
}
 void from_variant( const json_spirit::Value& v, wasm::symbol& t ){
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
 void from_variant( const json_spirit::Value& v, wasm::asset& t ){
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

 void from_variant( const json_spirit::Value& v, field_def& field ){
     if (v.type() == json_spirit::obj_type) { 
        auto o = v.get_obj();
        for( json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i )
        {
            string key = Config_type::get_name(*i);
            ABI_Enum abi_key = mapStringValues[key];
            switch (abi_key)
            {
               case ID_Name : 
               {
                    from_variant(Config_type::get_value(*i), field.name);
                    //std::cout << field.name <<std::endl ;
                    break;
               }
               case ID_Type : 
               {
                   from_variant(Config_type::get_value(*i), field.type);
                   //std::cout << field.type <<std::endl ;
                   break;
                }
               default : break;
            }     
           
        }
     } 
}
 void from_variant( const json_spirit::Value& v, struct_def& s ){
     if (v.type() == json_spirit::obj_type) {   

        auto o = v.get_obj();
        for( json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i )
        {   
            string key = Config_type::get_name(*i);
            ABI_Enum abi_key = mapStringValues[key];
            switch (abi_key)
            {
               case ID_Name : 
               {
                    from_variant(Config_type::get_value(*i), s.name);
                    //std::cout << s.name <<std::endl ;
                    break;
               }
               case ID_Base : 
               {
                   from_variant(Config_type::get_value(*i), s.base);
                   //std::cout << s.base <<std::endl ;
                   break;
                }
               case ID_Fields : 
               {
                   from_variant(Config_type::get_value(*i), s.fields);
                   //std::cout << s.base <<std::endl ;
                   break;
               }
               default : break;
            }            
        }

     } 

}

 void from_variant( const json_spirit::Value& v, action_def& a ){
     if (v.type() == json_spirit::obj_type) {   

        auto o = v.get_obj();
        for( json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i )
        {   
            string key = Config_type::get_name(*i);
            ABI_Enum abi_key = mapStringValues[key];
            switch (abi_key)
            {
               case ID_Name : 
               {
                    from_variant(Config_type::get_value(*i), a.name);
                    //std::cout << a.name <<std::endl ;
                    break;
               }
               case ID_Type : 
               {
                   from_variant(Config_type::get_value(*i), a.type);
                   //std::cout << a.type <<std::endl ;
                   break;
                }
               case ID_Ricardian_contract : 
               {
                   from_variant(Config_type::get_value(*i), a.ricardian_contract);
                   //std::cout << a.ricardian_contract <<std::endl ;
                   break;
               }
               default : break;
            }            
        }

     } 

}

 void from_variant( const json_spirit::Value& v, table_def& table ){
     if (v.type() == json_spirit::obj_type) {   

        auto o = v.get_obj();
        for( json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i )
        {   
            string key = Config_type::get_name(*i);
            ABI_Enum abi_key = mapStringValues[key];
            switch (abi_key)
            {
               case ID_Name : 
               {
                    from_variant(Config_type::get_value(*i), table.name);
                    //std::cout << table.name <<std::endl ;
                    break;
               }
               case ID_Type : 
               {
                   from_variant(Config_type::get_value(*i), table.type);
                   //std::cout << table.type <<std::endl ;
                   break;
                }
               case ID_Index_type : 
               {
                   from_variant(Config_type::get_value(*i), table.index_type);
                   //std::cout << table.index_type <<std::endl ;
                   break;
                }
               case ID_Key_types : 
               {
                   from_variant(Config_type::get_value(*i), table.key_types);
                   //std::cout << table.index_type <<std::endl ;
                   break;
                }
               case ID_Key_names : 
               {
                   from_variant(Config_type::get_value(*i), table.key_names);
                   //std::cout << table.index_type <<std::endl ;
                   break;
                }


               default : break;
            }            
        }

     } 

}

 void from_variant( const json_spirit::Value& v, clause_pair& c ){
     if (v.type() == json_spirit::obj_type) {   

        auto o = v.get_obj();
        for( json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i )
        {   
            string key = Config_type::get_name(*i);
            ABI_Enum abi_key = mapStringValues[key];
            switch (abi_key)
            {
               case ID_ID : 
               {
                    from_variant(Config_type::get_value(*i), c.id);
                    //std::cout << c.id <<std::endl ;
                    break;
               }
               case ID_Body : 
               {
                   from_variant(Config_type::get_value(*i), c.body);
                   //std::cout << c.body <<std::endl ;
                   break;
                }
               default : break;
            }            
        }

     } 

}

 void from_variant( const json_spirit::Value& v, type_def& type ){
     if (v.type() == json_spirit::obj_type) {   

        auto o = v.get_obj();
        for( json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i )
        {   
            string key = Config_type::get_name(*i);
            ABI_Enum abi_key = mapStringValues[key];
            switch (abi_key)
            {
               case ID_New_type_name : 
               {
                    from_variant(Config_type::get_value(*i), type.new_type_name);
                    //std::cout << type.new_type_name <<std::endl ;
                    break;
               }
               case ID_Type : 
               {
                   from_variant(Config_type::get_value(*i), type.type);
                   //std::cout << type.type <<std::endl ;
                   break;
                }
               default : break;
            }            
        }

     } 

}


 void from_variant( const json_spirit::Value& v, abi_def& abi ){

 try {
     if (v.type() == json_spirit::obj_type) {
        auto o = v.get_obj();

        for( json_spirit::Object::const_iterator i = o.begin(); i != o.end(); ++i )
        {
                //std::cout << Config_type::get_name(*i) <<std::endl ;              
                string key = Config_type::get_name(*i);
                ABI_Enum abi_key = mapStringValues[key];

                switch (abi_key)
                {
                  case ID_Comment : break;
                  case ID_Version : 
                  {
                      abi.version  = Config_type::get_value(*i).get_str();
                      //std::cout << abi.version  <<std::endl ;
                      break;
                   }
                  case ID_Structs : 
                  {
                      from_variant(Config_type::get_value(*i), abi.structs);
                      break;
                  }
                  case ID_Actions :
                  { 
                      from_variant(Config_type::get_value(*i), abi.actions);
                      break;
                  }
                  case ID_Types : 
                  { 
                      from_variant(Config_type::get_value(*i), abi.types);
                      break;
                  }
                  case ID_Tables : 
                  { 
                      from_variant(Config_type::get_value(*i), abi.tables);
                      break;
                  }
                  case ID_Ricardian_clauses : 
                  {
                     from_variant(Config_type::get_value(*i), abi.ricardian_clauses);
                     break;
                  }
                  case ID_Variants : break;
                  case ID_Abi_extensions : break;
                  default : break;
                }             
        }
      } 
   } catch(...) {
           // mvo("data", act.data);
   }

}


 } //wasm


