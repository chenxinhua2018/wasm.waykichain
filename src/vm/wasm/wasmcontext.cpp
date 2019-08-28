#pragma once
#include "wasm/wasmcontext.hpp"
#include "wasm/wasmnativecontract.hpp"
#include "wasm/exceptions.hpp"
#include "wasm/types/name.hpp"
#include "wasm/wasmconfig.hpp"

using namespace std;
using namespace wasm;

namespace wasm {

CRegID Name2RegID(uint64_t account){
  uint32_t height = uint32_t(account) ;
  uint16_t index = uint16_t(account >> 32);
  return CRegID(height, index);
}

uint64_t RegID2Name(CRegID regID){

  uint64_t account = uint64_t(regID.GetIndex());
  account = (account << 32) + uint64_t(regID.GetHeight());

  return account;
}

void CWasmContext::ExecuteInline(CInlineTransaction t){
    //wasm_assert check the code and authorization
    inline_transactions.push_back(t);
    //queue.pushBack(t);
}

vector<uint8_t> CWasmContext::GetCode(uint64_t account) {
 
  CUniversalContract contract;
  cache.contractCache.GetContract(Name2RegID(account), contract);
  vector<uint8_t> code;
  code.insert(code.begin(), contract.code.begin(), contract.code.end());

  return code;

}

void CWasmContext::Initialize(){

  wasmInterface.Initialize(wasm::vmType::eosvm);
  RegisterNativeHandler(name("wasmio").value, name("setcode").value, WasmNativeSetcode);

}

void CWasmContext::Execute(inline_transaction_trace& trace){

      Initialize();

      notified.push_back(receiver);
      ExecuteOne(trace);

      //receipt for accounts in notified
      for(uint32_t i = 1; i < notified.size(); ++i){
          receiver = notified[i];
          trace.inline_traces.emplace_back();
          ExecuteOne(trace.inline_traces.back());
      }

      WASM_ASSERT( recurse_depth < wasm::max_inline_action_depth,
                  TRANSACTION_EXCEPTION,"transaction_exception", "max inline action depth per transaction reached" );

      for( auto& inline_trx : inline_transactions ) {
          trace.inline_traces.emplace_back();
          control_trx.DispatchInlineTransaction( trace.inline_traces.back(), inline_trx, inline_trx.contract, cache, state, recurse_depth + 1 );
      }    

}

void CWasmContext::ExecuteOne( inline_transaction_trace& trace ){

    trace.trx = trx;
    trace.receiver = receiver;

    auto native = FindNativeHandle(receiver, trx.action);
    if( native ){
       (*native)(*this);
    }else{
        vector<uint8_t> code = GetCode(receiver);
        if (code.size() > 0){
            try{
                wasmInterface.Execute(code, this);
            }catch( CException& e ) {
               //std::cout << "CWasmContext ExecuteOne:" <<  e.errMsg << std::endl;
               throw e;
            }
        }
    }

    trace.trx_id = control_trx.GetHash();
    // trace.block_height = 
    // trace.block_time =   

}

bool CWasmContext::HasRecipient( uint64_t account ) const {
    for( auto a : notified )
        if( a == account )
         return true;
    return false;
}

void CWasmContext::RequireRecipient( uint64_t recipient ){

    if( !HasRecipient(recipient) ) {
        notified.push_back(recipient);
     }

}

void CWasmContext::RegisterNativeHandler(uint64_t receiver, uint64_t action, nativeHandler v){
    native_handlers[std::pair(receiver, action)] = v;
}

nativeHandler* CWasmContext::FindNativeHandle(uint64_t receiver, uint64_t action){

  auto handler = native_handlers.find(std::pair(receiver, action));
  if( handler != native_handlers.end()){
    return &handler->second;
  }

  return nullptr;

}

// bool CWasmContext::SetData( uint64_t contract, string k ,string v) { 
//   return cache.contractCache.SetContractData(Name2RegID(contract), k, v);
// } 
// bool CWasmContext::GetData( uint64_t contract, string k ,string& v) { 
//   return cache.contractCache.GetContractData(Name2RegID(contract), k, v);
// } 
// bool CWasmContext::EraseData( uint64_t contract, string k ) { 
//   return cache.contractCache.EraseContractData(Name2RegID(contract), k );
// } 


}