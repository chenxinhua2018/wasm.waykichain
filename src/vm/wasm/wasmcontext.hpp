#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <queue>
#include <map>

#include "tx/wasmcontracttx.h"
#include "wasm/types/inlinetransaction.hpp"
#include "wasm/wasminterface.hpp"
#include "wasm/datastream.hpp"
#include "wasm/wasmtrace.hpp"
#include "persistence/cachewrapper.h"

using namespace std;
using namespace wasm;
namespace wasm {

CRegID Name2RegID(uint64_t account);
uint64_t RegID2Name(CRegID regID);

class CWasmContext;
using nativeHandler = std::function<void(CWasmContext&)>;

// class CInlineTransactionsQueue {

// public:
// 	CInlineTransactionsQueue(){
// 		trxNumber = 0;
// 	};
// 	~CInlineTransactionsQueue(){};

// 	CInlineTransaction popFront(){
// 		CInlineTransaction trx = queue.front();
// 		queue.pop();
// 		return  trx;
// 	}

// 	bool pushBack(CInlineTransaction trx){
// 		trxNumber ++;
// 		queue.push(trx);
// 		return true;
// 	}

//     int size(){
//         return queue.size();
//     }
// public:
// 	uint32_t trxNumber;
// 	std::queue<CInlineTransaction> queue;
// };


class CWasmContext : public CWasmContextInterface {

public:
	//CWasmContext(){};
	CWasmContext( CWasmContractTx& ctrl, CInlineTransaction& t, CCacheWrapper& cw, CValidationState& s, uint32_t depth=0)
	:control_trx(ctrl)
    ,trx(t)
	,cache(cw)
    ,state(s)
    ,recurse_depth(depth)
    {
        // queue = q;
        // db = std::move(CWasmDbWrapper(c));
        // controlTx = ctrl;
        // state = s;
        //trx = queue.popFront();
        //receiver = trx.contract;
        //notified.push_back(receiver);

    };
	~CWasmContext(){};

public:
    vector<uint8_t> GetCode(uint64_t account);
    void RegisterNativeHandler(uint64_t receiver, uint64_t action, nativeHandler v);
    nativeHandler* FindNativeHandle(uint64_t receiver, uint64_t action);
    void ExecuteOne();
    void Initialize();

public:
    void ExecuteInline(CInlineTransaction t);
    bool HasRecipient( uint64_t account ) const;
    void RequireRecipient( uint64_t recipient );

    uint64_t Receiver() { return receiver;} 
    uint64_t Contract() { return trx.contract;} 
    uint64_t Action() { return trx.action;} 
    const char* GetActionData() {return trx.data.data();}
    uint32_t GetActionDataSize() {return trx.data.size();}

    bool SetData( uint64_t contract, string k ,string v) { 
        return cache.contractCache.SetContractData(Name2RegID(contract), k, v);
        //return true;
    } 
    bool GetData( uint64_t contract, string k ,string& v) { 
        return cache.contractCache.GetContractData(Name2RegID(contract), k, v);
        //return true;
    } 
    bool EraseData( uint64_t contract, string k ) { 
        return cache.contractCache.EraseContractData(Name2RegID(contract), k );
        //return true;
    } 
       
public:
	//std::string  sParam = "hello world";
	uint64_t receiver;
    vector<uint64_t> notified;

    //CInlineTransactionsQueue& queue;
	//CWasmDbWrapper db;
    CWasmContractTx& control_trx;
    CInlineTransaction& trx;
    CCacheWrapper& cache;
    CValidationState& state;
	// CAccountDBCache *pAccountCache;

    CWasmInterface wasmInterface;
    map< pair<uint64_t,uint64_t>, nativeHandler>   native_handlers;

    uint32_t recurse_depth;
    vector<CInlineTransaction> inline_transactions;

};
}
