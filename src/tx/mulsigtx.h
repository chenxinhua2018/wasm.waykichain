// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2016 The Coin developers
// Copyright (c) 2014-2019 The WaykiChain developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php


#ifndef COIN_MULSIGTX_H
#define COIN_MULSIGTX_H

#include "tx.h"

class CSignaturePair {
public:
    CRegID regid;  //!< regid only
    UnsignedCharArray signature;

    IMPLEMENT_SERIALIZE(
        READWRITE(regid);
        READWRITE(signature);)

public:
    CSignaturePair(const CSignaturePair &signaturePair) {
        regid     = signaturePair.regid;
        signature = signaturePair.signature;
    }

    CSignaturePair(const CRegID &regIdIn, const UnsignedCharArray &signatureIn) {
        regid     = regIdIn;
        signature = signatureIn;
    }

    CSignaturePair() {}

    string ToString() const;
    Object ToJson() const;
};

class CMulsigTx : public CBaseTx {
public:
    mutable CUserID desUserId;              //!< keyid or regid
    uint64_t bcoins;                        //!< transfer amount
    uint8_t required;                       //!< number of required keys
    UnsignedCharArray memo;                 //!< memo
    vector<CSignaturePair> signaturePairs;  //!< signature pair

    CKeyID keyId;  //!< only in memory

public:
    CMulsigTx() : CBaseTx(BCOIN_TRANSFER_MTX) {}

    CMulsigTx(const CBaseTx *pBaseTx) : CBaseTx(BCOIN_TRANSFER_MTX) {
        assert(BCOIN_TRANSFER_MTX == pBaseTx->nTxType);
        *this = *(CMulsigTx *)pBaseTx;
    }

    CMulsigTx(const vector<CSignaturePair> &signaturePairsIn, const CUserID &desUserIdIn,
                uint64_t feesIn, const uint64_t valueIn, const int validHeightIn,
                const uint8_t requiredIn, const UnsignedCharArray &memoIn)
        : CBaseTx(BCOIN_TRANSFER_MTX, CNullID(), validHeightIn, feesIn) {
        if (desUserIdIn.type() == typeid(CRegID))
            assert(!desUserIdIn.get<CRegID>().IsEmpty());

        signaturePairs = signaturePairsIn;
        desUserId      = desUserIdIn;
        bcoins         = valueIn;
        required       = requiredIn;
        memo           = memoIn;
    }

    CMulsigTx(const vector<CSignaturePair> &signaturePairsIn, const CUserID &desUserIdIn,
                uint64_t feesIn, const uint64_t valueIn, const int validHeightIn,
                const uint8_t requiredIn)
        : CBaseTx(BCOIN_TRANSFER_MTX, CNullID(), validHeightIn, feesIn) {
        if (desUserIdIn.type() == typeid(CRegID))
            assert(!desUserIdIn.get<CRegID>().IsEmpty());

        signaturePairs = signaturePairsIn;
        desUserId      = desUserIdIn;
        bcoins         = valueIn;
        required       = requiredIn;
    }

    ~CMulsigTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(signaturePairs);
        READWRITE(desUserId);
        READWRITE(VARINT(llFees));
        READWRITE(VARINT(bcoins));
        READWRITE(VARINT(required));
        READWRITE(memo);
    )

    TxID ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight);
            // Do NOT add item.signature.
            for (const auto &item : signaturePairs) {
                ss << item.regid;
            }
            ss << desUserId << VARINT(llFees) << VARINT(bcoins) << VARINT(required) << memo;
            sigHash = ss.GetHash();
        }
        return sigHash;
    }

    virtual map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{SYMB::WICC, bcoins}}; }
    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CMulsigTx>(this); }
    virtual string ToString(CAccountDBCache &view);
    virtual Object ToJson(const CAccountDBCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int height, int index, CCacheWrapper &cw, CValidationState &state);
};

#endif //COIN_MULSIGTX_H