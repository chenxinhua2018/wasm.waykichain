// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef PERSIST_ACCOUNTDB_H
#define PERSIST_ACCOUNTDB_H

#include "entities/asset.h"
#include "leveldbwrapper.h"
#include "entities/asset.h"
#include "entities/asset.h"
#include "commons/arith_uint256.h"
#include "dbconf.h"
#include "dbaccess.h"

#include <map>
#include <string>
#include <utility>
#include <vector>


class CAssetDBCache {
public:
    CAssetDBCache() {}

    CAssetDBCache(CDBAccess *pDbAccess) : assetCache(pDbAccess) {
        assert(pDbAccess->GetDbNameType() == DBNameType::ASSET);
    }

    ~CAssetDBCache() {}

public:
    bool GetAsset(const TokenSymbol &tokenSymbol, CAsset &asset);
    bool SaveAsset(const CAsset &asset);
    bool ExistAssetSymbol(const TokenSymbol &tokenSymbol);

    bool AddAssetTradingPair(const CAssetTradingPair &assetTradingPair);
    bool ExistAssetTradingPair(const CAssetTradingPair &TradingPair);
    bool EraseAssetTradingPair(const CAssetTradingPair &assetTradingPair);

    bool Flush();

    void SetBaseViewPtr(CAssetDBCache *pBaseIn) {
        assetCache.SetBase(&pBaseIn->assetCache);
        assetTradingPairCache.SetBase(&pBaseIn->assetTradingPairCache);
    };

private:
/*  CCompositeKVCache     prefixType            key              value           variable           */
/*  -------------------- --------------------   --------------  -------------   --------------------- */
    // <asset_tokenSymbol -> asset>
    CCompositeKVCache< dbk::ASSET,              TokenSymbol,        CAsset>         assetCache;
    // <asset_trading_pair -> 1>
    CCompositeKVCache< dbk::ASSET_TRADING_PAIR, CAssetTradingPair,  uint8_t>        assetTradingPairCache;
};

#endif  // PERSIST_ACCOUNTDB_H
