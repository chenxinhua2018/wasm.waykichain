// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ENTITIES_CDP_H
#define ENTITIES_CDP_H

#include "asset.h"
#include "id.h"

#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"

using namespace json_spirit;

/**
 * CDP Cache Item: stake in BaseCoin to get StableCoins
 *
 * Ij =  TNj * (Hj+1 - Hj)/Y * 0.2a/Log10(1+b*TNj)
 *
 * Persisted in LDB as:
 *      cdp{$RegID}{$CTxCord} --> { blockHeight, total_staked_bcoins, total_owed_scoins }
 *
 */
struct CUserCDP {
    uint256 cdpid;                      // CDP TxID
    CRegID owner_regid;                 // CDP Owner RegId
    int32_t block_height;               // persisted: Hj (Hj+1 refer to current height) - last op block height
    TokenSymbol bcoin_symbol;           // persisted
    TokenSymbol scoin_symbol;           // persisted
    uint64_t total_staked_bcoins;       // persisted: total staked bcoins
    uint64_t total_owed_scoins;         // persisted: TNj = last + minted = total minted - total redempted

    mutable double collateralRatioBase; // ratioBase = total_staked_bcoins / total_owed_scoins, mem-only

    CUserCDP() : block_height(0), total_staked_bcoins(0), total_owed_scoins(0) {}

    CUserCDP(const CRegID &regId, const uint256 &cdpTxIdIn)
        : cdpid(cdpTxIdIn), owner_regid(regId), block_height(0), bcoin_symbol(SYMB::WICC), scoin_symbol(SYMB::WUSD), total_staked_bcoins(0), total_owed_scoins(0) {}

    bool operator<(const CUserCDP &cdp) const {
        if (collateralRatioBase == cdp.collateralRatioBase) {
            if (owner_regid == cdp.owner_regid)
                return cdpid < cdp.cdpid;
            else
                return owner_regid < cdp.owner_regid;

        } else
            return collateralRatioBase < cdp.collateralRatioBase;
    }

    IMPLEMENT_SERIALIZE(
        READWRITE(cdpid);
        READWRITE(owner_regid);
        READWRITE(VARINT(block_height));
        READWRITE(bcoin_symbol);
        READWRITE(scoin_symbol);
        READWRITE(VARINT(total_staked_bcoins));
        READWRITE(VARINT(total_owed_scoins));
        if (fRead) {
            collateralRatioBase = double (total_staked_bcoins) / total_owed_scoins;
        }
    )

    string ToString() {
        return strprintf(
            "cdpid=%s, owner_regid=%s, block_height=%d, bcoin_symbol=%s, total_staked_bcoins=%d, "
            "scoin_symbol=%s, tatal_owed_scoins=%d, collateralRatioBase=%f",
            cdpid.ToString(), owner_regid.ToString(), block_height, bcoin_symbol, total_staked_bcoins,
            scoin_symbol, total_owed_scoins, collateralRatioBase);
    }

    Object ToJson() {
        Object result;
        result.push_back(Pair("cdpid",          cdpid.GetHex()));
        result.push_back(Pair("regid",          owner_regid.ToString()));
        result.push_back(Pair("last_height",    block_height));
        result.push_back(Pair("bcoin_symbol",   bcoin_symbol));
        result.push_back(Pair("total_bcoin",    total_staked_bcoins));
        result.push_back(Pair("scoin_symbol",   scoin_symbol));
        result.push_back(Pair("total_scoin",    total_owed_scoins));
        result.push_back(Pair("ratio_base",     collateralRatioBase));
        return result;
    }

    // FIXME: need to set other members empty?
    bool IsEmpty() const { return cdpid.IsEmpty(); }
    void SetEmpty() { cdpid.SetEmpty(); }
};

#endif //ENTITIES_CDP_H