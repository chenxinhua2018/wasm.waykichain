// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pricefeedtx.h"

#include "commons/serialize.h"
#include "crypto/hash.h"
#include "main.h"
#include "miner/miner.h"
#include "persistence/pricefeeddb.h"
#include "tx.h"
#include "commons/util.h"
#include "config/version.h"

bool CPriceFeedTx::CheckTx(int32_t height, CCacheWrapper &cw, CValidationState &state) {

    IMPLEMENT_CHECK_TX_FEE;
    IMPLEMENT_CHECK_TX_REGID(txUid.type());

    if (pricePoints.size() == 0 || pricePoints.size() > 3) { //FIXME: hardcode here
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, tx price points number not within 1..3"),
            REJECT_INVALID, "bad-tx-pricepoint-size-error");
    }

    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, read txUid %s account info error",
                        txUid.ToString()), PRICE_FEED_FAIL, "bad-read-accountdb");

    CRegID sendRegId = txUid.get<CRegID>();
    if (!cw.delegateCache.ExistDelegate(sendRegId.ToString())) { // must be a miner
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, txUid %s account is not a delegate error",
                        txUid.ToString()), PRICE_FEED_FAIL, "account-isn't-delegate");
    }

    uint64_t priceFeedstaked_fcoinsMin;
    if (!cw.sysParamCache.GetParam(PRICE_FEED_FCOIN_STAKE_AMOUNT_MIN, priceFeedstaked_fcoinsMin)) {
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, read PRICE_FEED_FCOIN_STAKE_AMOUNT_MIN error",
                        txUid.ToString()), READ_SYS_PARAM_FAIL, "read-sysparamdb-error");
    }

    CAccountToken accountToken = account.GetToken(SYMB::WGRT);
    if (accountToken.staked_amount < priceFeedstaked_fcoinsMin) // must stake enough fcoins
        return state.DoS(100, ERRORMSG("CPriceFeedTx::CheckTx, Staked Fcoins insufficient by txUid %s account error",
                        txUid.ToString()), PRICE_FEED_FAIL, "account-stakedfoins-insufficient");

    IMPLEMENT_CHECK_TX_SIGNATURE(account.owner_pubkey);
    return true;
}

bool CPriceFeedTx::ExecuteTx(int32_t height, int32_t index, CCacheWrapper &cw, CValidationState &state) {
    CAccount account;
    if (!cw.accountCache.GetAccount(txUid, account))
        return state.DoS(100, ERRORMSG("CPriceFeedTx::ExecuteTx, read txUid %s account info error",
                        txUid.ToString()), PRICE_FEED_FAIL, "bad-read-accountdb");

    if (!account.OperateBalance(SYMB::WICC, SUB_FREE, llFees)) {
        return state.DoS(100, ERRORMSG("CPriceFeedTx::ExecuteTx, deduct fee from account failed ,regId=%s",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "deduct-account-fee-failed");
    }

    if (!cw.accountCache.SaveAccount(account)) {
        return state.DoS(100, ERRORMSG("CPriceFeedTx::ExecuteTx, update account %s failed",
                        txUid.ToString()), UPDATE_ACCOUNT_FAIL, "bad-save-account");
    }

    // update the price feed cache accordingly
    if (!cw.ppCache.AddBlockPricePointInBatch(height, txUid.get<CRegID>(), pricePoints)) {
        return state.DoS(100, ERRORMSG("CPriceFeedTx::ExecuteTx, txUid %s account duplicated price feed exits",
                        txUid.ToString()), PRICE_FEED_FAIL, "duplicated-pricefeed");
    }

    if (!SaveTxAddresses(height, index, cw, state, {txUid}))
        return false;

    return true;
}

string CPriceFeedTx::ToString(CAccountDBCache &accountCache) {
    string str;
    for (auto pp : pricePoints) {
        str += pp.ToString() + ", ";
    }

    return strprintf("txType=%s, hash=%s, ver=%d, txUid=%s, llFees=%ld, pricePoints=%s, nValidHeight=%d\n",
                     GetTxType(nTxType), GetHash().ToString(), nVersion, txUid.ToString(), llFees, str, nValidHeight);
}

Object CPriceFeedTx::ToJson(const CAccountDBCache &accountCache) const {
    Array pricePointArray;
    for (const auto &pp : pricePoints) {
        pricePointArray.push_back(pp.ToJson());
    }

    Object result;
    IMPLEMENT_UNIVERSAL_ITEM_TO_JSON(accountCache);
    result.push_back(Pair("price_points", pricePointArray));

    return result;
}

bool CPriceFeedTx::GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds) {
    // TODO:
    return true;
}

bool CPriceFeedTx::GetTopPriceFeederList(CCacheWrapper &cw, vector<CAccount> &priceFeederAccts) {
    // TODO:
    return true;
}