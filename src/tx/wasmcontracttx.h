#ifndef TX_WASM_CONTRACT_TX_H
#define TX_WASM_CONTRACT_TX_H

#include "tx.h"
#include "wasm/wasmtrace.hpp"

class CWasmContractTx : public CBaseTx {
public:
    uint64_t contract;
    uint64_t action;
    std::vector<char> data;

    uint64_t amount;
    TokenSymbol symbol;

public:
    CWasmContractTx(const CBaseTx *pBaseTx): CBaseTx(WASM_CONTRACT_TX) {
        assert(WASM_CONTRACT_TX == pBaseTx->nTxType);
        *this = *(CWasmContractTx *)pBaseTx;
    }
    CWasmContractTx(): CBaseTx(WASM_CONTRACT_TX) {}
    ~CWasmContractTx() {}

    IMPLEMENT_SERIALIZE(
        READWRITE(VARINT(this->nVersion));
        nVersion = this->nVersion;
        READWRITE(VARINT(nValidHeight));
        READWRITE(txUid);
        READWRITE(contract);
        READWRITE(action);
        READWRITE(data);
        READWRITE(amount);
        READWRITE(symbol);
        READWRITE(VARINT(llFees));
        READWRITE(signature);)

    uint256 ComputeSignatureHash(bool recalculate = false) const {
        if (recalculate || sigHash.IsNull()) {
            CHashWriter ss(SER_GETHASH, 0);
            ss << VARINT(nVersion) << uint8_t(nTxType) << VARINT(nValidHeight) << txUid 
               << contract << action << data << amount << symbol
               << VARINT(llFees);
            sigHash = ss.GetHash();
        }

        return sigHash;
    }

    virtual uint256 GetHash() const { return ComputeSignatureHash(); }
    virtual std::shared_ptr<CBaseTx> GetNewInstance() { return std::make_shared<CWasmContractTx>(this); }
    virtual uint64_t GetFuel(uint32_t nFuelRate);
    virtual map<TokenSymbol, uint64_t> GetValues() const { return map<TokenSymbol, uint64_t>{{SYMB::WICC, 0}}; }
    virtual string ToString(CAccountDBCache &view);
    virtual Object ToJson(const CAccountDBCache &AccountView) const;
    virtual bool GetInvolvedKeyIds(CCacheWrapper &cw, set<CKeyID> &keyIds);

    virtual bool CheckTx(int height, CCacheWrapper &cw, CValidationState &state);
    virtual bool ExecuteTx(int height, int index, CCacheWrapper &cw, CValidationState &state); 

public:
    void DispatchInlineTransaction( wasm::inline_transaction_trace& trace, 
                                                 const CInlineTransaction& trx, 
                                                 uint64_t receiver, 
                                                 CCacheWrapper &cache, 
                                                 CValidationState &state,
                                                 uint32_t recurse_depth);
    
};

#endif //TX_WASM_CONTRACT_TX_H