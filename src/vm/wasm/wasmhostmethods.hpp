
#pragma once
#include <iostream>
#include <ostream>
#include <string>
#include <cstring>
#include <sstream>
#include <map> 

#include "wasmcontextinterface.hpp"
#include "datastream.hpp"
#include "softfloat.hpp"
#include "compiler_builtins.hpp"
#include "types/uint128.hpp"
#include "exceptions.hpp"

extern map<string, string> database;
using namespace wasm;
namespace wasm {

class WasmHostMethods {

  public:
  WasmHostMethods( CWasmContextInterface *pCtx ){pWasmContext = pCtx;};
  ~WasmHostMethods(){};


  CWasmContextInterface* pWasmContext;

  static uint64_t char_to_symbol( char c ) {
      if( c >= 'a' && c <= 'z' )
         return (c - 'a') + 6;
      if( c >= '1' && c <= '5' )
         return (c - '1') + 1;
      return 0;
  }

  // Each char of the string is encoded into 5-bit chunk and left-shifted
  // to its 5-bit slot starting with the highest slot for the first char.
  // The 13th char, if str is long enough, is encoded into 4-bit chunk
  // and placed in the lowest 4 bits. 64 = 12 * 5 + 4
  static uint64_t string_to_name( const char* str )
  {
      uint64_t name = 0;
      int i = 0;
      for ( ; str[i] && i < 12; ++i) {
          // NOTE: char_to_symbol() returns char type, and without this explicit
          // expansion to uint64 type, the compilation fails at the point of usage
          // of string_to_name(), where the usage requires constant (compile time) expression.
           name |= (char_to_symbol(str[i]) & 0x1f) << (64 - 5 * (i + 1));
       }

      // The for-loop encoded up to 60 high bits into uint64 'name' variable,
      // if (strlen(str) > 12) then encode str[12] into the low (remaining)
      // 4 bits of 'name'
      if (i == 12)
          name |= char_to_symbol(str[12]) & 0x0F;
      return name;
  }

  static std::string name_to_string( uint64_t value){
      string charmap(".12345abcdefghijklmnopqrstuvwxyz");
      string str(".............");//(13,'.');

      uint64_t tmp = value;
      for( uint32_t i = 0; i <= 12; ++i ) {
         char c = charmap[tmp & (i == 0 ? 0x0f : 0x1f)];
         str[12-i] = c;
         tmp >>= (i == 0 ? 4 : 5);
      }

      //str.trim_right([]( char c ){ return c == '.'; }); 
      uint32_t length = str.size();
      for (uint32_t i = length - 1; i >= 0; --i){
      if (str[i] != '.'){
        return str.substr(0, i + 1);//string(str, str + i + 1);
      }
      } 

      return "";
  }

  static string StringToHex16String( string str, string separator = "" ){

    const std::string hex = "0123456789abcdef";
    std::stringstream ss;
 
    for (std::string::size_type i = 0; i < str.size(); ++i)
        ss << hex[(unsigned char)str[i] >> 4] << hex[(unsigned char)str[i] & 0xf] << separator;

    return ss.str();

 }

 string StringPrint( string str1, string str2 ){

      std::ostringstream   ostr;
      ostr << str1 << str2 << std::endl;
      string msg = ostr.str();
      return msg;
 }

  //system
  void abort() {
      wasm_assert( false,  "abort() called");
   }

  void wasm_assert( uint32_t test, const char* msg ) {
      //std::cout << "wasm_assert:" << msg << std::endl;
      if (!test ) {
          //std::cout << msg << std::endl;
          WASM_ASSERT(false, WASM_ASSERT_FAIL, "wasm-assert-fail", "wasm-assert-fail:%s", msg)
          //throw 0;
      }
  }

  void  wasm_assert_code( uint32_t test, uint64_t code ){
      //std::cout << "eosio_assert_code:" << code << std::endl;
      if (!test ) {
          //std::cout << code << std::endl;
          std::ostringstream o;
          o << code;
          WASM_ASSERT(false, WASM_ASSERT_FAIL, "wasm-assert-code", o.str().data())
          //throw 0;
      }
  }

  // void eosio_assert( uint32_t test, const char* msg ) {
  //     //std::cout << "eosio_assert:" << msg << std::endl;
  //     //std::cout << msg << std::endl;
  //     if (!test ) {
  //         std::cout << msg << std::endl;
  //         //LogPrint(msg);
  //         throw 0;
  //     }
  // }

  // void  eosio_assert_code( uint32_t test, uint64_t code ){
  //     //std::cout << "eosio_assert_code:" << code << std::endl;
  //     if (!test ) {
  //         std::cout << code << std::endl;
  //         throw 0;
  //     }
  // }

  uint64_t current_time() {
      //return static_cast<uint64_t>( context.control.pending_block_time().time_since_epoch().count() );
      //std::cout << "current_time" << std::endl;
      return 0;
  }


  //action
  //static void* memset(void* ptr, int x, size_t n) { return ::memset(ptr, x, n); }
  uint32_t read_action_data( void* memory, uint32_t buffer_size ) {

      //std::cout << "read_action_data" << std::endl;
      //uint32_t s = (uint32_t)wasmContext.trx.data.size();
      uint32_t s = pWasmContext->GetActionDataSize();
      if( buffer_size == 0 ) return s;

      uint32_t copy_size = std::min( buffer_size, s);
      //std::memcpy(memory, wasmContext.trx.data.data(), copy_size );
      std::memcpy(memory, pWasmContext->GetActionData(), copy_size );

      //std::string data(wasmContext.trx.data.data(),copy_size);
      std::string data(pWasmContext->GetActionData(),copy_size);

      //std::cout << "read_action_data:" << StringToHex16String(data) << " copy_size:"<< copy_size <<std::endl;
      return copy_size;
  }

  uint32_t action_data_size() {

      //auto size = wasmContext.trx.data.size();
      auto size = pWasmContext->GetActionDataSize();
      //std::cout << "action_data_size size:"<< size << std::endl;
      //return wasmContext.trx.data.size();
      return size;
  }

  uint64_t current_receiver() {
         //return wasmContext.receiver;
      return pWasmContext->Receiver();
  }

  //database
  int32_t db_store( const void* key, uint32_t key_len, const void* value, uint32_t value_len ) {  

      //std::cout << "db_store" << std::endl;

      //const uint64_t payer = wasmContext.receiver;
     const uint64_t payer = pWasmContext->Receiver();
      
      string k = string((const char*)key, key_len);
      string v = string((const char*)value, value_len);

      //std::cout << "db_store key:"<<StringToHex16String(k,"")<<" key_len:"<<key_len 
      //          << " value:"<<StringToHex16String(v,"")<<" value_len:"<<value_len << std::endl;

      //string oldValue;
      //const uint64_t contract = wasmContext.receiver;
      const uint64_t contract = pWasmContext->Receiver();

      //wasm_assert(wasmContext.cache.SetContractData(contract, k, v), StringPrint("wasm db_store SetContractData failed, key:", StringToHex16String(k," ")).data());
      wasm_assert(pWasmContext->SetData(contract, k, v), StringPrint("wasm db_store SetContractData failed, key:", StringToHex16String(k," ")).data());      //wasmContext.AppendUndo(contract, k, oldValue);

      return 1;
  }

  int32_t db_remove( const void* key, uint32_t key_len ) {  
      //std::cout << "db_remove" << std::endl;
      //const uint64_t payer = wasmContext.receiver;
     const uint64_t payer = pWasmContext->Receiver();

      string k = string((const char*)key, key_len);
      //std::cout << "db_remove key: "<< StringToHex16String(k)<<" key_len: "<<key_len << std::endl;

      //string oldValue;
      //const uint64_t contract = wasmContext.receiver;
      const uint64_t contract = pWasmContext->Receiver();

      // wasm_assert(wasmContext.cache.EraseContractData(contract, k),
      //             StringPrint("wasm db_remove EraseContractData failed, key:%s!\n",StringToHex16String(k," ")).data());
      wasm_assert(pWasmContext->EraseData(contract, k),
                  StringPrint("wasm db_remove EraseContractData failed, key:%s!\n",StringToHex16String(k," ")).data());

      //wasmContext.AppendUndo(contract, k, oldValue);

      return 0;
  }

  int32_t db_get( const void* key, uint32_t key_len, void* value, uint32_t value_len ) {
      //std::cout << "db_get" << std::endl;
      string k = string((const char*)key, key_len);

      //std::cout << "db_get" << " key: "<<StringToHex16String(k,"")<<" key_len: "<<key_len << std::endl;
      string v;
      //const uint64_t contract = wasmContext.receiver;
      const uint64_t contract = pWasmContext->Receiver();
      // if(! wasmContext.cache.GetContractData(contract, k, v))
      //   return 0;

      if(! pWasmContext->GetData(contract, k, v))
        return 0;

      auto size = v.size();
      if(value_len == 0)
        return size;

      auto value_size = value_len > size ? size : value_len;
      memcpy(value, v.data(), value_size);

      return value_size;
  }

  int32_t db_update( const void* key, uint32_t key_len, const void* value, uint32_t value_len ) {
      //std::cout << "db_update" << std::endl;
      string k = string((const char*)key, key_len);
      string v = string((const char*)value, value_len);

      //const uint64_t payer = wasmContext.receiver;
      const uint64_t payer = pWasmContext->Receiver();

      //std::cout << "db_update key: "<<StringToHex16String(k,"")<<" key_len:"<<key_len << " value: "<<StringToHex16String(v,"")<<" value_len:"<<value_len <<std::endl;

      // const uint64_t contract = wasmContext.receiver;
      const uint64_t contract = pWasmContext->Receiver();
      // wasm_assert(wasmContext.cache.SetContractData(contract, k, v), 
      //             StringPrint("wasm db_update SetContractData key fail, key:%s!\n",StringToHex16String(k,"")).data());
      wasm_assert(pWasmContext->SetData(contract, k, v), 
                  StringPrint("wasm db_update SetContractData key fail, key:%s!\n",StringToHex16String(k,"")).data());

      return 1;
  }


 //memory
  char* memcpy( void* dest, const void* src, int length ) {
      // EOS_ASSERT((size_t)(std::abs((ptrdiff_t)dest.value - (ptrdiff_t)src.value)) >= length,
      //       overlapping_memory_error, "memcpy can only accept non-aliasing pointers");
      //std::cout << "memcpy" << std::endl;
      return (char *)std::memcpy(dest, src, length);
  }

  char* memmove( void*  dest, const void*  src, int length ) {
      //std::cout << "memmove" << std::endl;
      return (char *)std::memmove(dest, src, length);
  }

  int memcmp( const void*  dest, const void*  src, int length ) {
      //std::cout << "memcmp" << std::endl;
      int ret = std::memcmp(dest, src, length);
      if(ret < 0)
        return -1;
      if(ret > 0)
        return 1;
      return 0;
  }

  char* memset( void*  dest, int value, int length ) {
      //std::cout << "memset" << std::endl;
      return (char *)std::memset( dest, value, length );
  }


  //print for contract debug
  void print_num( uint64_t n ) { std::cout << "Number : " << n << "\n"; }


    // example of a host "method"
  void print_name( const char* nm ) { std::cout << std::string("Name : ") << nm 
                                             << std::string(" Receiver:") << pWasmContext->Receiver() 
                                             << std::string(" Code:")<< name_to_string(pWasmContext->Contract()) 
                                             << std::string(" Action:")<< name_to_string(pWasmContext->Action()) 
                                             << std::string("\n"); }

  void printn( uint64_t value ) {//should be name
      //std::cout << "printn" << std::endl;
      //std::cout << name_to_string(value);
  }

  void printui( uint64_t value ) {
      //std::cout << "printui" << std::endl;
      //std::cout << value;
  }

  void printi( uint64_t value ) {
      //std::cout << "printui" << std::endl;
      //std::cout << value;
  }

  void prints( const char* str ) {
      //std::cout << "prints" << std::endl ;
      //std::cout << str ;
  }

  void prints_l( const char* str, uint32_t len ) {
      //std::cout << "prints_l" << std::endl;
      //std::cout << string(str,len);
  }

  //authorization
  void require_auth( uint64_t account ) {
      //std::cout << "require_auth:" << account << std::endl;
      //std::cout << "require_authorization" << std::endl;
   }

   void require_auth2( uint64_t account, uint64_t permission ) {
      std::cout << "require_auth2" << account << " "<< permission << std::endl;
      //std::cout << "account:"<< account << "permission" << permission << std::endl;
      //std::cout << "require_authorization" << std::endl;
      //return;
   }

  bool has_authorization( uint64_t account )const {
      //std::cout << "has_authorization:" << account << std::endl;
      return true;
   }

  void require_recipient( uint64_t recipient ) {
      //context.require_recipient( recipient );
      //std::cout << "require_recipient:" << recipient << std::endl;
      //wasmContext.RequireRecipient(recipient);
      pWasmContext->RequireRecipient(recipient);

   }

  bool is_account( uint64_t account ) {
     //return context.is_account( account );
     //std::cout << "is_account:" << account << std::endl;
     // std::cout << "account:"<< account << std::endl;
     return true;
   }

  //transaction
  void send_inline( void* data, uint32_t data_len ) {
      // //TODO: Why is this limit even needed? And why is it not consistently checked on actions in input or deferred transactions
      // EOS_ASSERT( data_len < context.control.get_global_properties().configuration.max_inline_action_size, inline_action_too_big,
      //           "inline action too big" );

      CInlineTransaction trx = wasm::unpack<CInlineTransaction>((const char*)data,data_len);
      //wasmContext.ExecuteInline(trx);
      pWasmContext->ExecuteInline(trx);
      //std::cout << "send_inline" << std::endl;
      //std::string d((const char*)data, data_len); 
      //std::cout << "send_inline:" << StringToHex16String(d) << " data_len:"<< data_len << std::endl;

  }


  //llvm compiler builtins rt apis( GCC low-level runtime library ), eg. std:string in contract
  void __ashlti3(__int128& ret, uint64_t low, uint64_t high, uint32_t shift) {
     uint128 i(high, low);
     i <<= shift;
     ret = (unsigned __int128)i;
  }

  void __ashrti3(__int128& ret, uint64_t low, uint64_t high, uint32_t shift) {
     // retain the signedness
     ret = high;
     ret <<= 64;
     ret |= low;
     ret >>= shift;
  }

  void __lshlti3(__int128& ret, uint64_t low, uint64_t high, uint32_t shift) {
     uint128 i(high, low);
     i <<= shift;
     ret = (unsigned __int128)i;
  }

  void __lshrti3(__int128& ret, uint64_t low, uint64_t high, uint32_t shift) {
     uint128 i(high, low);
     i >>= shift;
     ret = (unsigned __int128)i;
  }

  void __divti3(__int128& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
     __int128 lhs = ha;
     __int128 rhs = hb;

     lhs <<= 64;
     lhs |=  la;

     rhs <<= 64;
     rhs |=  lb;

     wasm_assert(rhs != 0,  "divide by zero");

     lhs /= rhs;

     ret = lhs;
  }

  void __udivti3(unsigned __int128& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
     unsigned __int128 lhs = ha;
     unsigned __int128 rhs = hb;

     lhs <<= 64;
     lhs |=  la;

     rhs <<= 64;
     rhs |=  lb;

     wasm_assert(rhs != 0, "divide by zero");

     lhs /= rhs;
     ret = lhs;
  }

  void __multi3(__int128& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
     __int128 lhs = ha;
     __int128 rhs = hb;

     lhs <<= 64;
     lhs |=  la;

     rhs <<= 64;
     rhs |=  lb;

     lhs *= rhs;
     ret = lhs;
  }

  void __modti3(__int128& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
     __int128 lhs = ha;
     __int128 rhs = hb;

     lhs <<= 64;
     lhs |=  la;

     rhs <<= 64;
     rhs |=  lb;

     wasm_assert(rhs != 0,  "divide by zero");

     lhs %= rhs;
     ret = lhs;
  }

  void __umodti3(unsigned __int128& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb) {
     unsigned __int128 lhs = ha;
     unsigned __int128 rhs = hb;

     lhs <<= 64;
     lhs |=  la;

     rhs <<= 64;
     rhs |=  lb;

     wasm_assert(rhs != 0, "divide by zero");

     lhs %= rhs;
     ret = lhs;
  }

  // arithmetic long double
  void __addtf3( float128_t& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
     float128_t a = {{ la, ha }};
     float128_t b = {{ lb, hb }};
     ret = f128_add( a, b );
  }
  void __subtf3( float128_t& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
     float128_t a = {{ la, ha }};
     float128_t b = {{ lb, hb }};
     ret = f128_sub( a, b );
  }
  void __multf3( float128_t& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
     float128_t a = {{ la, ha }};
     float128_t b = {{ lb, hb }};
     ret = f128_mul( a, b );
  }
  void __divtf3( float128_t& ret, uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
     float128_t a = {{ la, ha }};
     float128_t b = {{ lb, hb }};
     ret = f128_div( a, b );
  }
  void __negtf2( float128_t& ret, uint64_t la, uint64_t ha ) {
     ret = {{ la, (ha ^ (uint64_t)1 << 63) }};
  }

  // conversion long double
  void __extendsftf2( float128_t& ret, float f ) {
     ret = f32_to_f128( to_softfloat32(f) );
  }
  void __extenddftf2( float128_t& ret, double d ) {
     ret = f64_to_f128( to_softfloat64(d) );
  }
  double __trunctfdf2( uint64_t l, uint64_t h ) {
     float128_t f = {{ l, h }};
     return from_softfloat64(f128_to_f64( f ));
  }
  float __trunctfsf2( uint64_t l, uint64_t h ) {
     float128_t f = {{ l, h }};
     return from_softfloat32(f128_to_f32( f ));
  }
  int32_t __fixtfsi( uint64_t l, uint64_t h ) {
     float128_t f = {{ l, h }};
     return f128_to_i32( f, 0, false );
  }
  int64_t __fixtfdi( uint64_t l, uint64_t h ) {
     float128_t f = {{ l, h }};
     return f128_to_i64( f, 0, false );
  }
  void __fixtfti( __int128& ret, uint64_t l, uint64_t h ) {
     float128_t f = {{ l, h }};
     ret = ___fixtfti( f );
  }
  uint32_t __fixunstfsi( uint64_t l, uint64_t h ) {
     float128_t f = {{ l, h }};
     return f128_to_ui32( f, 0, false );
  }
  uint64_t __fixunstfdi( uint64_t l, uint64_t h ) {
     float128_t f = {{ l, h }};
     return f128_to_ui64( f, 0, false );
  }
  void __fixunstfti( unsigned __int128& ret, uint64_t l, uint64_t h ) {
     float128_t f = {{ l, h }};
     ret = ___fixunstfti( f );
  }
  void __fixsfti( __int128& ret, float a ) {
     ret = ___fixsfti( to_softfloat32(a).v );
  }
  void __fixdfti( __int128& ret, double a ) {
     ret = ___fixdfti( to_softfloat64(a).v );
  }
  void __fixunssfti( unsigned __int128& ret, float a ) {
     ret = ___fixunssfti( to_softfloat32(a).v );
  }
  void __fixunsdfti( unsigned __int128& ret, double a ) {
     ret = ___fixunsdfti( to_softfloat64(a).v );
  }
  double __floatsidf( int32_t i ) {
     return from_softfloat64(i32_to_f64(i));
  }
  void __floatsitf( float128_t& ret, int32_t i ) {
     ret = i32_to_f128(i);
  }
  void __floatditf( float128_t& ret, uint64_t a ) {
     ret = i64_to_f128( a );
  }
  void __floatunsitf( float128_t& ret, uint32_t i ) {
     ret = ui32_to_f128(i);
  }
  void __floatunditf( float128_t& ret, uint64_t a ) {
     ret = ui64_to_f128( a );
  }
  double __floattidf( uint64_t l, uint64_t h ) {
     uint128 v(h, l);
     unsigned __int128 val = (unsigned __int128)v;
     return ___floattidf( *(__int128*)&val );
  }
  double __floatuntidf( uint64_t l, uint64_t h ) {
     uint128 v(h, l);
     return ___floatuntidf( (unsigned __int128)v );
  }
  int ___cmptf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb, int return_value_if_nan ) {
     float128_t a = {{ la, ha }};
     float128_t b = {{ lb, hb }};
     if ( __unordtf2(la, ha, lb, hb) )
        return return_value_if_nan;
     if ( f128_lt( a, b ) )
        return -1;
     if ( f128_eq( a, b ) )
        return 0;
     return 1;
  }
  int __eqtf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
     return ___cmptf2(la, ha, lb, hb, 1);
  }
  int __netf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
     return ___cmptf2(la, ha, lb, hb, 1);
  }
  int __getf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
     return ___cmptf2(la, ha, lb, hb, -1);
  }
  int __gttf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
     return ___cmptf2(la, ha, lb, hb, 0);
  }
  int __letf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
     return ___cmptf2(la, ha, lb, hb, 1);
  }
  int __lttf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
     return ___cmptf2(la, ha, lb, hb, 0);
  }
  int __cmptf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
     return ___cmptf2(la, ha, lb, hb, 1);
  }
  int __unordtf2( uint64_t la, uint64_t ha, uint64_t lb, uint64_t hb ) {
     float128_t a = {{ la, ha }};
     float128_t b = {{ lb, hb }};
     if ( is_nan(a) || is_nan(b) )
        return 1;
     return 0;
  }

  static bool is_nan( const float32_t f ) {
    return f32_is_nan( f );
  }
  static bool is_nan( const float64_t f ) {
    return f64_is_nan( f );
  }
  static bool is_nan( const float128_t& f ) {
    return f128_is_nan( f );
  }

   
};
}