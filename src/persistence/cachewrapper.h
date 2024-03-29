// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_CACHEWRAPPER_H
#define PERSIST_CACHEWRAPPER_H

#include "accountdb.h"
#include "cdpdb.h"
#include "contractdb.h"
#include "delegatedb.h"
#include "dexdb.h"
#include "pricefeeddb.h"
#include "sysparamdb.h"
#include "txdb.h"
#include "txreceiptdb.h"

class CCacheDBManager;
class CBlockUndo;

class CCacheWrapper {
public:
    CSysParamDBCache    sysParamCache;
    CAccountDBCache     accountCache;
    CContractDBCache    contractCache;
    CDelegateDBCache    delegateCache;
    CCdpDBCache         cdpCache;
    CDexDBCache         dexCache;
    CTxReceiptDBCache   txReceiptCache;

    CTxMemCache         txCache;
    CPricePointMemCache ppCache;

    CTxUndo             txUndo;

public:
    CCacheWrapper();

    CCacheWrapper(CSysParamDBCache* pSysParamCacheIn,
                  CAccountDBCache* pAccountCacheIn,
                  CContractDBCache* pContractCacheIn,
                  CDelegateDBCache* pDelegateCacheIn,
                  CCdpDBCache* pCdpCacheIn,
                  CDexDBCache* pDexCacheIn,
                  CTxReceiptDBCache* pTxReceiptCacheIn,
                  CTxMemCache *pTxCacheIn,
                  CPricePointMemCache *pPpCacheIn);
    CCacheWrapper(CCacheWrapper& cwIn);
    CCacheWrapper(CCacheDBManager* pCdMan);

    void EnableTxUndoLog(const uint256 &txid);
    void DisableTxUndoLog();
    const CTxUndo& GetTxUndo() const {
        return txUndo;
    }

    bool UndoDatas(CBlockUndo &blockUndo);

    void Flush();
private:
    void SetDbOpMapLog(CDBOpLogMap *pDbOpLogMap);
};

#endif //PERSIST_CACHEWRAPPER_H