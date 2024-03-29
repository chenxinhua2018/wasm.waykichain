// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2017-2019 The WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#ifndef ACCOUNT_REGISTER_H
#define ACCOUNT_REGISTER_H

#include "tx.h"

class CAccountRegisterTx : public CBaseTx {
public:
    mutable CUserID minerUid;  // miner pubkey

public:
    CAccountRegisterTx(const CBaseTx *pBaseTx): CBaseTx(ACCOUNT_REGISTER_TX) {
        assert(ACCOUNT_REGISTER_TX == pBaseTx->nTxType);
        *this = *(CAccountRegisterTx *)pBaseTx;
    }
    CAccountRegisterTx(const CUserID &txUidIn, const CUserID &minerUidIn, int64_t feesIn, int validHeightIn) :
        CBaseTx(ACCOUNT_REGISTER_TX, txUidIn, validHeightIn, feesIn) {
        minerUid    = minerUidIn;
    }
    CAccountRegisterTx(): CBaseTx(ACCOUNT_REGISTER_TX) {}

    ~CAccountRegisterTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);

        READWRITE(minerUid);
        READWRITE(VARINT(llFees));
        READWRITE(signature);)

    map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{SYMB::WICC, 0}}; }

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            assert(txUid.type() == typeid(CPubKey) &&
                   (minerUid.type() == typeid(CPubKey) || minerUid.type() == typeid(CNullID)));

            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid << minerUid << VARINT(llFees);

            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CAccountRegisterTx>(this); }
    virtual string ToString(CAccountDBCache &view);
    virtual Object ToJson(const CAccountDBCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper& cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int height, CCacheWrapper& cw, CValidationState &state);
    virtual bool ExecuteTx(int height, int index, CCacheWrapper& cw, CValidationState &state);
};

#endif