// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2017-2018 WaykiChain Developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php

#include "commons/base58.h"
#include "rpc/core/rpcserver.h"
#include "rpc/core/rpccommons.h"
#include "init.h"
#include "net.h"
#include "netbase.h"
#include "commons/util.h"
#include "wallet/wallet.h"
#include "wallet/walletdb.h"
#include "persistence/blockdb.h"
#include "persistence/txdb.h"
#include "config/configuration.h"
#include "miner/miner.h"
#include "main.h"
#include "vm/luavm/vmrunenv.h"

#include <stdint.h>

#include <boost/assign/list_of.hpp>
#include "json/json_spirit_utils.h"
#include "json/json_spirit_value.h"
#include "json/json_spirit_reader.h"

#include "boost/tuple/tuple.hpp"
#define revert(height) ((height<<24) | (height << 8 & 0xff0000) |  (height>>8 & 0xff00) | (height >> 24))

using namespace std;
using namespace boost;
using namespace boost::assign;
using namespace json_spirit;

Value gettransaction(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "gettransaction \"txid\"\n"
            "\nget the transaction detail by given transaction hash.\n"
            "\nArguments:\n"
            "1.txid   (string, required) The hast of transaction.\n"
            "\nResult a object about the transaction detail\n"
            "\nResult:\n"
            "\n\"txid\"\n"
            "\nExamples:\n" +
            HelpExampleCli("gettransaction",
                           "c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\n") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("gettransaction",
                           "c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\n"));

    uint256 txid(uint256S(params[0].get_str()));
    std::shared_ptr<CBaseTx> pBaseTx;
    Object obj;
    LOCK(cs_main);
    CBlock genesisblock;
    CBlockIndex* pGenesisBlockIndex = mapBlockIndex[SysCfg().GetGenesisBlockHash()];
    ReadBlockFromDisk(pGenesisBlockIndex, genesisblock);
    assert(genesisblock.GetMerkleRootHash() == genesisblock.BuildMerkleTree());
    for (unsigned int i = 0; i < genesisblock.vptx.size(); ++i) {
        if (txid == genesisblock.GetTxid(i)) {
            double dAmount = static_cast<double>(genesisblock.vptx.at(i)->GetValues()[SYMB::WICC]) / COIN;
            obj.push_back(Pair("amount", dAmount));
            obj.push_back(Pair("confirmations", chainActive.Tip()->height));
            obj.push_back(Pair("block_hash", genesisblock.GetHash().GetHex()));
            obj.push_back(Pair("block_index", (int)i));
            obj.push_back(Pair("block_time", (int)genesisblock.GetTime()));
            obj.push_back(Pair("txid", genesisblock.vptx.at(i)->GetHash().GetHex()));
            obj.push_back(Pair("details", GetTxAddressDetail(genesisblock.vptx.at(i))));
            CDataStream ds(SER_DISK, CLIENT_VERSION);
            ds << genesisblock.vptx[i];
            obj.push_back(Pair("hex", HexStr(ds.begin(), ds.end())));
            return obj;
        }
    }
    bool findTx(false);
    if (SysCfg().IsTxIndex()) {
        CDiskTxPos postx;
        if (pCdMan->pContractCache->ReadTxIndex(txid, postx)) {
            findTx = true;
            CAutoFile file(OpenBlockFile(postx, true), SER_DISK, CLIENT_VERSION);
            CBlockHeader header;
            try {
                file >> header;
                fseek(file, postx.nTxOffset, SEEK_CUR);
                file >> pBaseTx;
                // TODO:
                double dAmount = static_cast<double>(pBaseTx->GetValues()[SYMB::WICC]) / COIN;
                obj.push_back(Pair("amount", dAmount));
                obj.push_back(
                    Pair("confirmations", chainActive.Tip()->height - (int)header.GetHeight()));
                obj.push_back(Pair("blockhash", header.GetHash().GetHex()));
                obj.push_back(Pair("blocktime", (int)header.GetTime()));
                obj.push_back(Pair("txid", pBaseTx->GetHash().GetHex()));
                obj.push_back(Pair("details", GetTxAddressDetail(pBaseTx)));
                CDataStream ds(SER_DISK, CLIENT_VERSION);
                ds << pBaseTx;
                obj.push_back(Pair("hex", HexStr(ds.begin(), ds.end())));
            } catch (std::exception& e) {
                throw runtime_error(
                    tfm::format("%s : Deserialize or I/O error - %s", __func__, e.what()).c_str());
            }
            return obj;
        }
    }

    if (!findTx) {
        pBaseTx = mempool.Lookup(txid);
        if (pBaseTx == nullptr) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid txid");
        }
        // TODO:
        double dAmount = static_cast<double>(pBaseTx->GetValues()[SYMB::WICC]) / COIN;
        obj.push_back(Pair("amount", dAmount));
        obj.push_back(Pair("confirmations", 0));
        obj.push_back(Pair("txid", pBaseTx->GetHash().GetHex()));
        obj.push_back(Pair("details", GetTxAddressDetail(pBaseTx)));
        CDataStream ds(SER_DISK, CLIENT_VERSION);
        ds << pBaseTx;
        obj.push_back(Pair("hex", HexStr(ds.begin(), ds.end())));
    }

    return obj;
}

Value gettxdetail(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "gettxdetail \"txid\"\n"
            "\nget the transaction detail by given transaction hash.\n"
            "\nArguments:\n"
            "1.txid   (string,required) The hash of transaction.\n"
            "\nResult an object of the transaction detail\n"
            "\nResult:\n"
            "\n\"txid\"\n"
            "\nExamples:\n"
            + HelpExampleCli("gettxdetail","c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\n")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("gettxdetail","c5287324b89793fdf7fa97b6203dfd814b8358cfa31114078ea5981916d7a8ac\n"));

    uint256 txid(uint256S(params[0].get_str()));
    return GetTxDetailJSON(txid);
}

//create a register account tx
Value registeraccounttx(const Array& params, bool fHelp) {
    if (fHelp || params.size() == 0)
        throw runtime_error("registeraccounttx \"addr\" (\"fee\")\n"
            "\nregister local account public key to get its RegId\n"
            "\nArguments:\n"
            "1.addr: (string, required)\n"
            "2.fee: (numeric, optional) pay tx fees to miner\n"
            "\nResult:\n"
            "\"txid\": (string)\n"
            "\nExamples:\n"
            + HelpExampleCli("registeraccounttx", "n2dha9w3bz2HPVQzoGKda3Cgt5p5Tgv6oj 100000 ")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("registeraccounttx", "n2dha9w3bz2HPVQzoGKda3Cgt5p5Tgv6oj 100000 "));

    string addr = params[0].get_str();
    uint64_t fee = 0;
    uint64_t nDefaultFee = SysCfg().GetTxFee();
    if (params.size() > 1) {
        fee = params[1].get_uint64();
        if (fee < nDefaultFee) {
            throw JSONRPCError(RPC_INSUFFICIENT_FEE,
                               strprintf("Input fee smaller than mintxfee: %ld sawi", nDefaultFee));
        }
    } else {
        fee = nDefaultFee;
    }

    CKeyID keyId;
    if (!GetKeyId(addr, keyId))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Address invalid");

    CAccountRegisterTx rtx;
    assert(pWalletMain != nullptr);
    {
        EnsureWalletIsUnlocked();

        CAccountDBCache view(*pCdMan->pAccountCache);
        CAccount account;
        CUserID userId = keyId;
        if (!pCdMan->pAccountCache->GetAccount(userId, account))
            throw JSONRPCError(RPC_WALLET_ERROR, "Account does not exist");


        if (account.HaveOwnerPubKey())
            throw JSONRPCError(RPC_WALLET_ERROR, "Account was already registered");

        uint64_t balance = account.GetToken(SYMB::WICC).free_amount;
        if (balance < fee) {
            LogPrint("ERROR", "balance=%d, vs fee=%d", balance, fee);
            throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient");
        }

        CPubKey pubkey;
        if (!pWalletMain->GetPubKey(keyId, pubkey))
            throw JSONRPCError(RPC_WALLET_ERROR, "Key not found in local wallet");

        CPubKey minerPubKey;
        if (pWalletMain->GetPubKey(keyId, minerPubKey, true)) {
            rtx.minerUid = minerPubKey;
        } else {
            CNullID nullId;
            rtx.minerUid = nullId;
        }
        rtx.txUid        = pubkey;
        rtx.llFees       = fee;
        rtx.nValidHeight = chainActive.Tip()->height;

        if (!pWalletMain->Sign(keyId, rtx.ComputeSignatureHash(), rtx.signature))
            throw JSONRPCError(RPC_WALLET_ERROR, "in registeraccounttx Error: Sign failed.");
    }

    std::tuple<bool, string> ret;
    ret = pWalletMain->CommitTx((CBaseTx *) &rtx);
    if (!std::get<0>(ret))
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));

    Object obj;
    obj.push_back(Pair("txid", std::get<1>(ret)));
    return obj;
}

Value callcontracttx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 5 || params.size() > 6) {
        throw runtime_error(
            "callcontracttx \"sender addr\" \"app regid\" \"arguments\" \"amount\" \"fee\" (\"height\")\n"
            "\ncreate contract invocation transaction\n"
            "\nArguments:\n"
            "1.\"sender addr\": (string, required) tx sender's base58 addr\n"
            "2.\"app regid\":   (string, required) contract RegId\n"
            "3.\"arguments\":   (string, required) contract arguments (Hex encode required)\n"
            "4.\"amount\":      (numeric, required) amount of WICC to be sent to the contract account\n"
            "5.\"fee\":         (numeric, required) pay to miner\n"
            "6.\"height\":      (numberic, optional) valid height\n"
            "\nResult:\n"
            "\"txid\":        (string)\n"
            "\nExamples:\n" +
            HelpExampleCli("callcontracttx",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\" \"411994-1\" \"01020304\" 10000 10000 100") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("callcontracttx",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\", \"411994-1\", \"01020304\", 10000, 10000, 100"));
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(str_type)(int_type)(int_type)(int_type));

    EnsureWalletIsUnlocked();

    CKeyID sendKeyId, recvKeyId;
    if (!GetKeyId(params[0].get_str(), sendKeyId))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sendaddress");

    if (!GetKeyId(params[1].get_str(), recvKeyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid app regid");
    }

    string arguments = ParseHexStr(params[2].get_str());
    if (arguments.size() >= kContractArgumentMaxSize) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Arguments's size out of range");
    }

    int64_t amount = AmountToRawValue(params[3]);
    int64_t fee    = AmountToRawValue(params[4]);
    int height     = (params.size() > 5) ? params[5].get_int() : chainActive.Height();
    if (fee == 0) {
        fee = GetTxMinFee(TxType::LCONTRACT_INVOKE_TX, height);
    }

    CPubKey sendPubKey;
    if (!pWalletMain->GetPubKey(sendKeyId, sendPubKey)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Key not found in the local wallet.");
    }

    CUserID sendUserId;
    CRegID sendRegId;
    sendUserId = (pCdMan->pAccountCache->GetRegId(CUserID(sendKeyId), sendRegId) && pCdMan->pAccountCache->RegIDIsMature(sendRegId))
                     ? CUserID(sendRegId)
                     : CUserID(sendPubKey);

    CRegID recvRegId;
    if (!pCdMan->pAccountCache->GetRegId(CUserID(recvKeyId), recvRegId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid app regid");
    }

    if (!pCdMan->pContractCache->HaveContract(recvRegId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to get contract");
    }

    CLuaContractInvokeTx tx;
    tx.nTxType      = LCONTRACT_INVOKE_TX;
    tx.txUid        = sendUserId;
    tx.app_uid      = recvRegId;
    tx.bcoins       = amount;
    tx.llFees       = fee;
    tx.arguments    = arguments;
    tx.nValidHeight = height;

    if (!pWalletMain->Sign(sendKeyId, tx.ComputeSignatureHash(), tx.signature)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
    }

    std::tuple<bool, string> ret = pWalletMain->CommitTx((CBaseTx*)&tx);
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }

    Object obj;
    obj.push_back(Pair("txid", std::get<1>(ret)));
    return obj;
}

// register a contract app tx
Value registercontracttx(const Array& params, bool fHelp)
{
    if (fHelp || params.size() < 3 || params.size() > 5) {
        throw runtime_error("registercontracttx \"addr\" \"filepath\" \"fee\" (\"height\") (\"appdesc\")\n"
            "\ncreate a transaction of registering a contract app\n"
            "\nArguments:\n"
            "1.\"addr\": (string required) contract owner address from this wallet\n"
            "2.\"filepath\": (string required), the file path of the app script\n"
            "3.\"fee\": (numeric required) pay to miner (the larger the size of script, the bigger fees are required)\n"
            "4.\"height\": (numeric optional) valid height, when not specified, the tip block hegiht in chainActive will be used\n"
            "5.\"appdesc\": (string optional) new app description\n"
            "\nResult:\n"
            "\"txid\": (string)\n"
            "\nExamples:\n"
            + HelpExampleCli("registercontracttx",
                "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"myapp.lua\" 1000000 (10000) (\"appdesc\")") +
                "\nAs json rpc call\n"
            + HelpExampleRpc("registercontracttx",
                "WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH \"myapp.lua\" 1000000 (10000) (\"appdesc\")"));
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(int_type)(int_type)(str_type));

    string luaScriptFilePath = GetAbsolutePath(params[1].get_str()).string();
    if (luaScriptFilePath.empty())
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "Lua Script file not exist!");

    if (luaScriptFilePath.compare(0, kContractScriptPathPrefix.size(), kContractScriptPathPrefix.c_str()) != 0)
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_INVALID, "Lua Script file not inside /tmp/lua dir or its subdir!");

    std::tuple<bool, string> result = CVmlua::CheckScriptSyntax(luaScriptFilePath.c_str());
    bool bOK = std::get<0>(result);
    if (!bOK)
        throw JSONRPCError(RPC_INVALID_PARAMETER, std::get<1>(result));

    FILE* file = fopen(luaScriptFilePath.c_str(), "rb+");
    if (!file)
        throw runtime_error("registercontracttx open script file (" + luaScriptFilePath + ") error");

    long lSize;
    fseek(file, 0, SEEK_END);
    lSize = ftell(file);
    rewind(file);

    if (lSize <= 0 || lSize > kContractScriptMaxSize) { // contract script file size must be <= 64 KB)
        fclose(file);
        throw JSONRPCError(
            RPC_INVALID_PARAMETER,
            (lSize == -1) ? "File size is unknown"
                          : ((lSize == 0) ? "File is empty" : "File size exceeds 64 KB limit"));
    }

    // allocate memory to contain the whole file:
    char *buffer = (char*) malloc(sizeof(char) * lSize);
    if (buffer == nullptr) {
        fclose(file);
        throw runtime_error("allocate memory failed");
    }
    if (fread(buffer, 1, lSize, file) != (size_t) lSize) {
        free(buffer);
        fclose(file);
        throw runtime_error("read script file error");
    } else {
        fclose(file);
    }

    CLuaContract luaContract;
    luaContract.code.assign(buffer, lSize);

    if (buffer)
        free(buffer);

    if (params.size() > 4) {
        luaContract.memo = params[4].get_str();
        if (luaContract.memo.size() > kContractMemoMaxSize) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "App desc is too large");
        }
    }

    uint64_t fee = params[2].get_uint64();
    int height   = params.size() > 3 ? params[3].get_int() : chainActive.Height();

    if (fee > 0 && fee < CBaseTx::nMinTxFee) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Fee is smaller than nMinTxFee");
    }
    CKeyID keyId;
    if (!GetKeyId(params[0].get_str(), keyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid send address");
    }

    assert(pWalletMain != nullptr);
    CLuaContractDeployTx tx;
    {
        EnsureWalletIsUnlocked();

        CAccount account;
        if (!pCdMan->pAccountCache->GetAccount(keyId, account)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Invalid send address");
        }

        if (!account.HaveOwnerPubKey()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account is unregistered");
        }

        uint64_t balance = account.GetToken(SYMB::WICC).free_amount;
        if (balance < fee) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient");
        }

        if (!pWalletMain->HaveKey(keyId)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Send address is not in wallet");
        }

        CRegID regId;
        pCdMan->pAccountCache->GetRegId(keyId, regId);

        tx.txUid          = regId;
        tx.contract       = luaContract;
        tx.llFees         = fee;
        tx.nRunStep       = tx.contract.GetContractSize();
        if (0 == height) {
            height = chainActive.Tip()->height;
        }
        tx.nValidHeight = height;

        if (!pWalletMain->Sign(keyId, tx.ComputeSignatureHash(), tx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }

        std::tuple<bool, string> ret;
        ret = pWalletMain->CommitTx((CBaseTx*)&tx);
        if (!std::get<0>(ret)) {
            throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
        }
        Object obj;
        obj.push_back(Pair("txid", std::get<1>(ret)));
        return obj;
    }
}

//vote a delegate transaction
Value votedelegatetx(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 3 || params.size() > 4) {
        throw runtime_error(
            "votedelegatetx \"sendaddr\" \"votes\" \"fee\" (\"height\") \n"
            "\ncreate a delegate vote transaction\n"
            "\nArguments:\n"
            "1.\"sendaddr\": (string required) The address from which votes are sent to other "
            "delegate addresses\n"
            "2. \"votes\"    (string, required) A json array of votes to delegate candidates\n"
            " [\n"
            "   {\n"
            "      \"delegate\":\"address\", (string, required) The delegate address where votes "
            "are received\n"
            "      \"votes\": n (numeric, required) votes, increase votes when positive or reduce "
            "votes when negative\n"
            "   }\n"
            "       ,...\n"
            " ]\n"
            "3.\"fee\": (numeric required) pay fee to miner\n"
            "4.\"height\": (numeric optional) valid height. When not supplied, the tip block "
            "height in chainActive will be used.\n"
            "\nResult:\n"
            "\"txid\": (string)\n"
            "\nExamples:\n" +
            HelpExampleCli("votedelegatetx",
                           "\"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\" "
                           "\"[{\\\"delegate\\\":\\\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\\\", "
                           "\\\"votes\\\":100000000}]\" 10000 ") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("votedelegatetx",
                           " \"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\", "
                           "[{\"delegate\":\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\", "
                           "\"votes\":100000000}], 10000 "));
    }
    RPCTypeCheck(params, list_of(str_type)(array_type)(int_type)(int_type));

    string sendAddr = params[0].get_str();
    uint64_t fee    = params[2].get_uint64();  // real type
    int height     = 0;
    if (params.size() > 3) {
        height = params[3].get_int();
    }
    Array arrVotes = params[1].get_array();

    CKeyID keyId;
    if (!GetKeyId(sendAddr, keyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid send address");
    }
    CDelegateVoteTx delegateVoteTx;
    assert(pWalletMain != nullptr);
    {
        EnsureWalletIsUnlocked();
        CAccount account;

        CUserID userId = keyId;
        if (!pCdMan->pAccountCache->GetAccount(userId, account)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account does not exist");
        }

        if (!account.HaveOwnerPubKey()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account is unregistered");
        }

        uint64_t balance = account.GetToken(SYMB::WICC).free_amount;
        if (balance < fee) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient");
        }

        if (!pWalletMain->HaveKey(keyId)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Send address is not in wallet");
        }

        delegateVoteTx.llFees = fee;
        if (0 != height) {
            delegateVoteTx.nValidHeight = height;
        } else {
            delegateVoteTx.nValidHeight = chainActive.Tip()->height;
        }
        delegateVoteTx.txUid = account.regid;

        for (auto objVote : arrVotes) {

            const Value& delegateAddr = find_value(objVote.get_obj(), "delegate");
            const Value& delegateVotes = find_value(objVote.get_obj(), "votes");
            if (delegateAddr.type() == null_type || delegateVotes == null_type) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Vote fund address error or fund value error");
            }
            CKeyID delegateKeyId;
            if (!GetKeyId(delegateAddr.get_str(), delegateKeyId)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Delegate address error");
            }
            CAccount delegateAcct;
            if (!pCdMan->pAccountCache->GetAccount(CUserID(delegateKeyId), delegateAcct)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Delegate address is not exist");
            }
            if (!delegateAcct.HaveOwnerPubKey()) {
                throw JSONRPCError(RPC_WALLET_ERROR, "Delegate address is unregistered");
            }

            VoteType voteType = (delegateVotes.get_int64() > 0) ? VoteType::ADD_BCOIN : VoteType::MINUS_BCOIN;
            CUserID candidateUid = CUserID(delegateAcct.regid);
            uint64_t bcoins = (uint64_t)abs(delegateVotes.get_int64());
            CCandidateVote candidateVote(voteType, candidateUid, bcoins);

            delegateVoteTx.candidateVotes.push_back(candidateVote);
        }

        if (!pWalletMain->Sign(keyId, delegateVoteTx.ComputeSignatureHash(), delegateVoteTx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }
    }

    std::tuple<bool, string> ret;
    ret = pWalletMain->CommitTx((CBaseTx*)&delegateVoteTx);
    if (!std::get<0>(ret)) {
        throw JSONRPCError(RPC_WALLET_ERROR, std::get<1>(ret));
    }

    Object objRet;
    objRet.push_back(Pair("txid", std::get<1>(ret)));
    return objRet;
}

// create a vote delegate raw transaction
Value genvotedelegateraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() <  3  || params.size() > 4) {
        throw runtime_error(
            "genvotedelegateraw \"addr\" \"candidate votes\" \"fees\" [height]\n"
            "\ncreate a vote delegate raw transaction\n"
            "\nArguments:\n"
            "1.\"addr\":                    (string required) from address that votes delegate(s)\n"
            "2. \"candidate votes\"         (string, required) A json array of json oper vote to delegates\n"
            " [\n"
            " {\n"
            "    \"delegate\":\"address\",  (string, required) The transaction id\n"
            "    \"votes\":n                (numeric, required) votes\n"
            " }\n"
            "       ,...\n"
            " ]\n"
            "3.\"fees\":                    (numeric required) pay to miner\n"
            "4.\"height\":                  (numeric optional) valid height, If not provide, use the active chain's block height"
            "\nResult:\n"
            "\"rawtx\":                     (string) raw transaction string\n"
            "\nExamples:\n" +
            HelpExampleCli("genvotedelegateraw",
                           "\"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\" "
                           "\"[{\\\"delegate\\\":\\\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\\\", "
                           "\\\"votes\\\":100000000}]\" 10000") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("genvotedelegateraw",
                           " \"wQquTWgzNzLtjUV4Du57p9YAEGdKvgXs9t\", "
                           "[{\"delegate\":\"wNDue1jHcgRSioSDL4o1AzXz3D72gCMkP6\", "
                           "\"votes\":100000000}], 10000"));
    }
    RPCTypeCheck(params, list_of(str_type)(array_type)(int_type)(int_type));

    string sendAddr = params[0].get_str();
    uint64_t fees   = params[2].get_uint64();
    int32_t height = chainActive.Tip()->height;
    if (params.size() > 3) {
        height = params[3].get_int();
        if (height <= 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid height");
        }
    }
    Array arrVotes = params[1].get_array();

    CKeyID keyId;
    if (!GetKeyId(sendAddr, keyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid send address");
    }
    CDelegateVoteTx delegateVoteTx;
    assert(pWalletMain != nullptr);
    {
        EnsureWalletIsUnlocked();
        CAccountDBCache view(*pCdMan->pAccountCache);
        CAccount account;

        CUserID userId = keyId;
        if (!pCdMan->pAccountCache->GetAccount(userId, account)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account does not exist");
        }

        if (!account.HaveOwnerPubKey()) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account is unregistered");
        }

        uint64_t balance = account.GetToken(SYMB::WICC).free_amount;
        if (balance < fees) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Account balance is insufficient");
        }

        if (!pWalletMain->HaveKey(keyId)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Send address is not in wallet");
        }

        delegateVoteTx.llFees       = fees;
        delegateVoteTx.nValidHeight = height;
        delegateVoteTx.txUid        = account.regid;

        for (auto objVote : arrVotes) {

            const Value& delegateAddress = find_value(objVote.get_obj(), "delegate");
            const Value& delegateVotes   = find_value(objVote.get_obj(), "votes");
            if (delegateAddress.type() == null_type || delegateVotes == null_type) {
                throw JSONRPCError(RPC_INVALID_PARAMETER,
                                   "Voted delegator's address type "
                                   "error or vote value error");
            }
            CKeyID delegateKeyId;
            if (!GetKeyId(delegateAddress.get_str(), delegateKeyId)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Voted delegator's address error");
            }
            CAccount delegateAcct;
            if (!pCdMan->pAccountCache->GetAccount(CUserID(delegateKeyId), delegateAcct)) {
                throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Voted delegator's address is unregistered");
            }

            VoteType voteType    = (delegateVotes.get_int64() > 0) ? VoteType::ADD_BCOIN : VoteType::MINUS_BCOIN;
            CUserID candidateUid = CUserID(delegateAcct.owner_pubkey);
            uint64_t bcoins      = (uint64_t)abs(delegateVotes.get_int64());
            CCandidateVote candidateVote(voteType, candidateUid, bcoins);

            delegateVoteTx.candidateVotes.push_back(candidateVote);
        }

        if (!pWalletMain->Sign(keyId, delegateVoteTx.ComputeSignatureHash(), delegateVoteTx.signature)) {
            throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
        }
    }

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = delegateVoteTx.GetNewInstance();
    ds << pBaseTx;
    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    return obj;
}

Value listaddr(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error(
            "listaddr\n"
            "\nreturn Array containing address,balance,haveminerkey,regid information.\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("listaddr", "")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("listaddr", ""));
    }

    Array retArray;
    assert(pWalletMain != nullptr);
    {
        set<CKeyID> setKeyId;
        pWalletMain->GetKeys(setKeyId);
        if (setKeyId.size() == 0) {
            return retArray;
        }
        CAccountDBCache accView(*pCdMan->pAccountCache);

        for (const auto &keyId : setKeyId) {
            CUserID userId(keyId);
            CAccount account;
            pCdMan->pAccountCache->GetAccount(userId, account);
            CKeyCombi keyCombi;
            pWalletMain->GetKeyCombi(keyId, keyCombi);

            Object obj;
            obj.push_back(Pair("addr",  keyId.ToAddress()));
            obj.push_back(Pair("regid", account.regid.ToString()));

            Object tokenMapObj;
            for (auto tokenPair : account.tokens) {
                Object tokenObj;
                const CAccountToken& token = tokenPair.second;
                tokenObj.push_back(Pair("free_amount",      token.free_amount));
                tokenObj.push_back(Pair("staked_amount",    token.staked_amount));
                tokenObj.push_back(Pair("frozen_amount",    token.frozen_amount));

                tokenMapObj.push_back(Pair(tokenPair.first, tokenObj));
            }

            obj.push_back(Pair("tokens",        tokenMapObj));
            obj.push_back(Pair("hasminerkey",   keyCombi.HaveMinerKey()));

            retArray.push_back(obj);
        }
    }

    return retArray;
}

Value listtransactions(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 3)
            throw runtime_error(
                "listtransactions ( \"account\" count from includeWatchonly)\n"
                "\nReturns up to 'count' most recent transactions skipping the first 'from' transactions for account 'account'.\n"
                "\nArguments:\n"
                "1. \"address\"    (string, optional) DEPRECATED. The account name. Should be \"*\"\n"
                "2. count          (numeric, optional, default=10) The number of transactions to return\n"
                "3. from           (numeric, optional, default=0) The number of transactions to skip\n"    "\nExamples:\n"
                "\nList the most recent 10 transactions in the systems\n"
                   + HelpExampleCli("listtransactions", "") +
                   "\nList transactions 100 to 120\n"
                   + HelpExampleCli("listtransactions", "\"*\" 20 100") +
                   "\nAs a json rpc call\n"
                   + HelpExampleRpc("listtransactions", "\"*\", 20, 100")
              );
    assert(pWalletMain != nullptr);
    string strAddress = "*";
    if (params.size() > 0)
        strAddress = params[0].get_str();
    if ("" == strAddress) {
        strAddress = "*";
    }

    Array arrayData;
    int nCount = -1;
    int nFrom = 0;
    if(params.size() > 1) {
        nCount =  params[1].get_int();
    }
    if(params.size() > 2) {
        nFrom = params[2].get_int();
    }

    LOCK2(cs_main, pWalletMain->cs_wallet);

    map<int, uint256, std::greater<int> > blockInfoMap;
    for (auto const &wtx : pWalletMain->mapInBlockTx) {
        CBlockIndex *pIndex = mapBlockIndex[wtx.first];
        if (pIndex != nullptr)
            blockInfoMap.insert(make_pair(pIndex->height, wtx.first));
    }

    int txnCount(0);
    int index(0);
    for (auto const &wtx : blockInfoMap) {
        CAccountTx accountTx = pWalletMain->mapInBlockTx[wtx.second];
        for (auto const & item : accountTx.mapAccountTx) {
            if (item.second->nTxType == BCOIN_TRANSFER_TX) {
                CBaseCoinTransferTx* ptx = (CBaseCoinTransferTx*)item.second.get();
                CKeyID sendKeyID;
                if (ptx->txUid.type() == typeid(CPubKey)) {
                    sendKeyID = ptx->txUid.get<CPubKey>().GetKeyId();
                } else if (ptx->txUid.type() == typeid(CRegID)) {
                    sendKeyID = ptx->txUid.get<CRegID>().GetKeyId(*pCdMan->pAccountCache);
                }

                CKeyID recvKeyId;
                if (ptx->toUid.type() == typeid(CKeyID)) {
                    recvKeyId = ptx->toUid.get<CKeyID>();
                } else if (ptx->toUid.type() == typeid(CRegID)) {
                    CRegID desRegID = ptx->toUid.get<CRegID>();
                    recvKeyId       = desRegID.GetKeyId(*pCdMan->pAccountCache);
                }

                bool bSend = true;
                if ("*" != strAddress && sendKeyID.ToAddress() != strAddress) {
                    bSend = false;
                }

                bool bRecv = true;
                if ("*" != strAddress && recvKeyId.ToAddress() != strAddress) {
                    bRecv = false;
                }

                if(nFrom > 0 && index++ < nFrom) {
                    continue;
                }

                if(!(bSend || bRecv)) {
                    continue;
                }

                if(nCount > 0 && txnCount > nCount) {
                    return arrayData;
                }

                if (bSend) {
                    if (pWalletMain->HaveKey(sendKeyID)) {
                        Object obj;
                        obj.push_back(Pair("address", recvKeyId.ToAddress()));
                        obj.push_back(Pair("category", "send"));
                        double dAmount = static_cast<double>(item.second->GetValues()[SYMB::WICC]) / COIN;
                        obj.push_back(Pair("amount", -dAmount));
                        obj.push_back(Pair("confirmations", chainActive.Tip()->height - accountTx.blockHeight));
                        obj.push_back(Pair("blockhash", (chainActive[accountTx.blockHeight]->GetBlockHash().GetHex())));
                        obj.push_back(Pair("blocktime", (int64_t)(chainActive[accountTx.blockHeight]->nTime)));
                        obj.push_back(Pair("txid", item.second->GetHash().GetHex()));
                        obj.push_back(Pair("tx_type", "BCOIN_TRANSFER_TX"));
                        obj.push_back(Pair("memo", HexStr(ptx->memo)));
                        arrayData.push_back(obj);

                        txnCount++;
                    }
                }

                if (bRecv) {
                    if (pWalletMain->HaveKey(recvKeyId)) {
                        Object obj;
                        obj.push_back(Pair("srcaddr", sendKeyID.ToAddress()));
                        obj.push_back(Pair("address", recvKeyId.ToAddress()));
                        obj.push_back(Pair("category", "receive"));
                        double dAmount = static_cast<double>(item.second->GetValues()[SYMB::WICC]) / COIN;
                        obj.push_back(Pair("amount", dAmount));
                        obj.push_back(Pair("confirmations", chainActive.Tip()->height - accountTx.blockHeight));
                        obj.push_back(Pair("blockhash", (chainActive[accountTx.blockHeight]->GetBlockHash().GetHex())));
                        obj.push_back(Pair("blocktime", (int64_t)(chainActive[accountTx.blockHeight]->nTime)));
                        obj.push_back(Pair("txid", item.second->GetHash().GetHex()));
                        obj.push_back(Pair("tx_type", "BCOIN_TRANSFER_TX"));
                        obj.push_back(Pair("memo", HexStr(ptx->memo)));

                        arrayData.push_back(obj);

                        txnCount++;
                    }
                }
            } else if (item.second->nTxType == LCONTRACT_INVOKE_TX) {
                CLuaContractInvokeTx* ptx = (CLuaContractInvokeTx*)item.second.get();
                CKeyID sendKeyID;
                if (ptx->txUid.type() == typeid(CPubKey)) {
                    sendKeyID = ptx->txUid.get<CPubKey>().GetKeyId();
                } else if (ptx->txUid.type() == typeid(CRegID)) {
                    sendKeyID = ptx->txUid.get<CRegID>().GetKeyId(*pCdMan->pAccountCache);
                }

                CKeyID recvKeyId;
                if (ptx->app_uid.type() == typeid(CRegID)) {
                    CRegID appUid = ptx->app_uid.get<CRegID>();
                    recvKeyId     = appUid.GetKeyId(*pCdMan->pAccountCache);
                }

                bool bSend = true;
                if ("*" != strAddress && sendKeyID.ToAddress() != strAddress) {
                    bSend = false;
                }

                bool bRecv = true;
                if ("*" != strAddress && recvKeyId.ToAddress() != strAddress) {
                    bRecv = false;
                }

                if(nFrom > 0 && index++ < nFrom) {
                    continue;
                }

                if(!(bSend || bRecv)) {
                    continue;
                }

                if(nCount > 0 && txnCount > nCount) {
                    return arrayData;
                }

                if (bSend) {
                    if (pWalletMain->HaveKey(sendKeyID)) {
                        Object obj;
                        obj.push_back(Pair("address", recvKeyId.ToAddress()));
                        obj.push_back(Pair("category", "send"));
                        double dAmount = static_cast<double>(item.second->GetValues()[SYMB::WICC]) / COIN;
                        obj.push_back(Pair("amount", -dAmount));
                        obj.push_back(Pair("confirmations", chainActive.Tip()->height - accountTx.blockHeight));
                        obj.push_back(Pair("blockhash", (chainActive[accountTx.blockHeight]->GetBlockHash().GetHex())));
                        obj.push_back(Pair("blocktime", (int64_t)(chainActive[accountTx.blockHeight]->nTime)));
                        obj.push_back(Pair("txid", item.second->GetHash().GetHex()));
                        obj.push_back(Pair("tx_type", "LCONTRACT_INVOKE_TX"));
                        obj.push_back(Pair("arguments", HexStr(ptx->arguments)));

                        arrayData.push_back(obj);

                        txnCount++;
                    }
                }

                if (bRecv) {
                    if (pWalletMain->HaveKey(recvKeyId)) {
                        double dAmount = static_cast<double>(item.second->GetValues()[SYMB::WICC]) / COIN;
                        Object obj;
                        obj.push_back(Pair("srcaddr", sendKeyID.ToAddress()));
                        obj.push_back(Pair("address", recvKeyId.ToAddress()));
                        obj.push_back(Pair("category", "receive"));
                        obj.push_back(Pair("amount", dAmount));
                        obj.push_back(Pair("confirmations", chainActive.Tip()->height - accountTx.blockHeight));
                        obj.push_back(Pair("blockhash", (chainActive[accountTx.blockHeight]->GetBlockHash().GetHex())));
                        obj.push_back(Pair("blocktime", (int64_t)(chainActive[accountTx.blockHeight]->nTime)));
                        obj.push_back(Pair("txid", item.second->GetHash().GetHex()));
                        obj.push_back(Pair("tx_type", "LCONTRACT_INVOKE_TX"));
                        obj.push_back(Pair("arguments", HexStr(ptx->arguments)));

                        arrayData.push_back(obj);

                        txnCount++;
                    }
                }
            }
            // TODO: BCOIN_TRANSFER_MTX
        }
    }
    return arrayData;
}

Value listtransactionsv2(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 3)
            throw runtime_error(
                "listtransactionsv2 ( \"account\" count from includeWatchonly)\n"
                "\nReturns up to 'count' most recent transactions skipping the first 'from' transactions for account 'account'.\n"
                "\nArguments:\n"
                "1. \"address\"    (string, optional) DEPRECATED. The account name. Should be \"*\".\n"
                "2. count          (numeric, optional, default=10) The number of transactions to return\n"
                "3. from           (numeric, optional, default=0) The number of transactions to skip\n"    "\nExamples:\n"
                "\nList the most recent 10 transactions in the systems\n"
                   + HelpExampleCli("listtransactionsv2", "") +
                   "\nList transactions 100 to 120\n"
                   + HelpExampleCli("listtransactionsv2", "\"*\" 20 100") +
                   "\nAs a json rpc call\n"
                   + HelpExampleRpc("listtransactionsv2", "\"*\", 20, 100")
              );

    assert(pWalletMain != nullptr);
    string strAddress = "*";
    if (params.size() > 0)
        strAddress = params[0].get_str();

    if("" == strAddress) {
        strAddress = "*";
    }

    Array arrayData;
    int nCount = -1;
    int nFrom = 0;
    if(params.size() > 1) {
        nCount =  params[1].get_int();
    }
    if(params.size() > 2) {
        nFrom = params[2].get_int();
    }

    LOCK2(cs_main, pWalletMain->cs_wallet);

    int txnCount(0);
    int index(0);
    CAccountDBCache accView(*pCdMan->pAccountCache);
    for (auto const &wtx : pWalletMain->mapInBlockTx) {
        for (auto const & item : wtx.second.mapAccountTx) {
            Object obj;
            CKeyID keyId;
            if (item.second.get() && item.second->nTxType == BCOIN_TRANSFER_TX) {
                CBaseCoinTransferTx* ptx = (CBaseCoinTransferTx*)item.second.get();

                if (!pCdMan->pAccountCache->GetKeyId(ptx->txUid, keyId)) {
                    continue;
                }
                string srcAddr = keyId.ToAddress();
                if (!pCdMan->pAccountCache->GetKeyId(ptx->toUid, keyId)) {
                    continue;
                }
                string desAddr = keyId.ToAddress();
                if ("*" != strAddress && desAddr != strAddress) {
                    continue;
                }
                if (nFrom > 0 && index++ < nFrom) {
                    continue;
                }
                if (nCount > 0 && txnCount++ > nCount) {
                    return arrayData;
                }
                obj.push_back(Pair("srcaddr", srcAddr));
                obj.push_back(Pair("desaddr", desAddr));
                double dAmount = static_cast<double>(item.second->GetValues()[SYMB::WICC]) / COIN;
                obj.push_back(Pair("amount", dAmount));
                obj.push_back(Pair("confirmations", chainActive.Tip()->height - wtx.second.blockHeight));
                obj.push_back(Pair("blockhash", (chainActive[wtx.second.blockHeight]->GetBlockHash().GetHex())));
                obj.push_back(Pair("blocktime", (int64_t)(chainActive[wtx.second.blockHeight]->nTime)));
                obj.push_back(Pair("txid", item.second->GetHash().GetHex()));
                arrayData.push_back(obj);
            }
        }
    }
    return arrayData;

}

Value listcontracttx(const Array& params, bool fHelp)
{
    if (fHelp || params.size() > 3 || params.size() < 1)
            throw runtime_error("listcontracttx ( \"account\" count from )\n"
                "\nReturns up to 'count' most recent transactions skipping the first 'from' transactions for account 'account'.\n"
                "\nArguments:\n"
                "1. \"account\"    (string). The contract RegId. \n"
                "2. count          (numeric, optional, default=10) The number of transactions to return\n"
                "3. from           (numeric, optional, default=0) The number of transactions to skip\n"    "\nExamples:\n"
                "\nList the most recent 10 transactions in the systems\n"
                   + HelpExampleCli("listcontracttx", "") +
                   "\nList transactions 100 to 120\n"
                   + HelpExampleCli("listcontracttx", "\"*\" 20 100") +
                   "\nAs a json rpc call\n"
                   + HelpExampleRpc("listcontracttx", "\"*\", 20, 100")
              );
    assert(pWalletMain != nullptr);

    string strRegId = params[0].get_str();
    CRegID regId(strRegId);
    if (regId.IsEmpty() == true) {
        throw runtime_error("in listcontracttx: contractRegId size error!\n");
    }

    if (!pCdMan->pContractCache->HaveContract(regId)) {
        throw runtime_error("in listcontracttx: contractRegId does not exist!\n");
    }

    Array arrayData;
    int nCount = -1;
    int nFrom = 0;
    if (params.size() > 1) {
        nCount = params[1].get_int();
    }
    if (params.size() > 2) {
        nFrom = params[2].get_int();
    }

    auto getregidstring = [&](CUserID const &userId) {
        if(userId.type() == typeid(CRegID))
            return userId.get<CRegID>().ToString();
        return string(" ");
    };

    LOCK2(cs_main, pWalletMain->cs_wallet);

    map<int, uint256, std::greater<int> > blockInfoMap;
    for (auto const &wtx : pWalletMain->mapInBlockTx) {
        CBlockIndex *pIndex = mapBlockIndex[wtx.first];
        if (pIndex != nullptr)
            blockInfoMap.insert(make_pair(pIndex->height, wtx.first));
    }

    int txnCount(0);
    int index(0);
    for (auto const &wtx : blockInfoMap) {
        CAccountTx accountTx = pWalletMain->mapInBlockTx[wtx.second];
        for (auto const & item : accountTx.mapAccountTx) {
            if (item.second.get() && item.second->nTxType == LCONTRACT_INVOKE_TX) {
                if (nFrom > 0 && index++ < nFrom) {
                    continue;
                }
                if (nCount > 0 && txnCount > nCount) {
                    return arrayData;
                }

                CLuaContractInvokeTx* ptx = (CLuaContractInvokeTx*) item.second.get();
                if (strRegId != getregidstring(ptx->app_uid)) {
                    continue;
                }

                CKeyID keyId;
                Object obj;

                CAccountDBCache accView(*pCdMan->pAccountCache);
                obj.push_back(Pair("txid",  ptx->GetHash().GetHex()));
                obj.push_back(Pair("regid",  getregidstring(ptx->txUid)));
                pCdMan->pAccountCache->GetKeyId(ptx->txUid, keyId);
                obj.push_back(Pair("addr",  keyId.ToAddress()));
                obj.push_back(Pair("dest_regid", getregidstring(ptx->app_uid)));
                pCdMan->pAccountCache->GetKeyId(ptx->txUid, keyId);
                obj.push_back(Pair("dest_addr", keyId.ToAddress()));
                obj.push_back(Pair("money", ptx->bcoins));
                obj.push_back(Pair("fees", ptx->llFees));
                obj.push_back(Pair("valid_height", ptx->nValidHeight));
                obj.push_back(Pair("arguments", HexStr(ptx->arguments)));
                arrayData.push_back(obj);

                txnCount++;
            }
        }
    }
    return arrayData;
}

Value listtx(const Array& params, bool fHelp) {
if (fHelp || params.size() > 2) {
        throw runtime_error("listtx\n"
                "\nget all confirmed transactions and all unconfirmed transactions from wallet.\n"
                "\nArguments:\n"
                "1. count          (numeric, optional, default=10) The number of transactions to return\n"
                "2. from           (numeric, optional, default=0) The number of transactions to skip\n"
                "\nExamples:\n"
                "\nResult:\n"
                "\nExamples:\n"
                "\nList the most recent 10 transactions in the system\n"
                + HelpExampleCli("listtx", "") +
                "\nList transactions 100 to 120\n"
                + HelpExampleCli("listtx", "20 100")
            );
    }

    Object retObj;
    int nDefCount = 10;
    int nFrom = 0;
    if(params.size() > 0) {
        nDefCount = params[0].get_int();
    }
    if(params.size() > 1) {
        nFrom = params[1].get_int();
    }
    assert(pWalletMain != nullptr);

    Array confirmedTxArray;
    int nCount = 0;
    map<int, uint256, std::greater<int> > blockInfoMap;
    for (auto const &wtx : pWalletMain->mapInBlockTx) {
        CBlockIndex *pIndex = mapBlockIndex[wtx.first];
        if (pIndex != nullptr)
            blockInfoMap.insert(make_pair(pIndex->height, wtx.first));
    }
    bool bUpLimited = false;
    for (auto const &blockInfo : blockInfoMap) {
        CAccountTx accountTx = pWalletMain->mapInBlockTx[blockInfo.second];
        for (auto const & item : accountTx.mapAccountTx) {
            if (nFrom-- > 0)
                continue;
            if (++nCount > nDefCount) {
                bUpLimited = true;
                break;
            }
            confirmedTxArray.push_back(item.first.GetHex());
        }
        if (bUpLimited) {
            break;
        }
    }
    retObj.push_back(Pair("confirmed_tx", confirmedTxArray));

    Array unconfirmedTxArray;
    for (auto const &tx : pWalletMain->unconfirmedTx) {
        unconfirmedTxArray.push_back(tx.first.GetHex());
    }
    retObj.push_back(Pair("unconfirmed_tx", unconfirmedTxArray));

    return retObj;
}

Value getaccountinfo(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "getaccountinfo \"addr\"\n"
            "\nget account information\n"
            "\nArguments:\n"
            "1.\"addr\": (string, required) account base58 address"
            "Returns account details.\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("getaccountinfo", "\"WT52jPi8DhHUC85MPYK8y8Ajs8J7CshgaB\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("getaccountinfo", "\"WT52jPi8DhHUC85MPYK8y8Ajs8J7CshgaB\""));
    }
    RPCTypeCheck(params, list_of(str_type));
    CKeyID keyId;
    CUserID userId;
    string addr = params[0].get_str();
    if (!GetKeyId(addr, keyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid address");
    }

    userId = keyId;
    Object obj;
    {
        CAccount account;
        if (pCdMan->pAccountCache->GetAccount(userId, account)) {
            if (!account.owner_pubkey.IsValid()) {
                CPubKey pubKey;
                CPubKey minerPubKey;
                if (pWalletMain->GetPubKey(keyId, pubKey)) {
                    pWalletMain->GetPubKey(keyId, minerPubKey, true);
                    account.owner_pubkey = pubKey;
                    account.keyid        = pubKey.GetKeyId();
                    if (pubKey != minerPubKey && !account.miner_pubkey.IsValid()) {
                        account.miner_pubkey = minerPubKey;
                    }
                }
            }
            obj = account.ToJsonObj();
            obj.push_back(Pair("position", "inblock"));
        } else {  // unregistered keyId
            CPubKey pubKey;
            CPubKey minerPubKey;
            if (pWalletMain->GetPubKey(keyId, pubKey)) {
                pWalletMain->GetPubKey(keyId, minerPubKey, true);
                account.owner_pubkey = pubKey;
                account.keyid        = pubKey.GetKeyId();
                if (minerPubKey != pubKey) {
                    account.miner_pubkey = minerPubKey;
                }
                obj = account.ToJsonObj();
                obj.push_back(Pair("position", "inwallet"));
            }
        }
    }
    return obj;
}

//list unconfirmed transaction of mine
Value listunconfirmedtx(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
         throw runtime_error("listunconfirmedtx \n"
                "\nget the list  of unconfirmedtx.\n"
                "\nArguments:\n"
                "\nResult a object about the unconfirm transaction\n"
                "\nResult:\n"
                "\nExamples:\n"
                + HelpExampleCli("listunconfirmedtx", "")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("listunconfirmedtx", ""));
    }

    Object retObj;
    Array unconfirmedTxArray;

    for (auto const& tx : pWalletMain->unconfirmedTx) {
        unconfirmedTxArray.push_back(tx.first.GetHex());
    }

    retObj.push_back(Pair("unconfirmed_tx", unconfirmedTxArray));

    return retObj;
}

Value gettxoperationlog(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("gettxoperationlog \"txid\"\n"
                    "\nget transaction operation log\n"
                    "\nArguments:\n"
                    "1.\"txid\": (string required) \n"
                    "\nResult:\n"
                    "\"vOperFund\": (string)\n"
                    "\"authorLog\": (string)\n"
                    "\nExamples:\n"
                    + HelpExampleCli("gettxoperationlog",
                            "\"0001a87352387b5b4d6d01299c0dc178ff044f42e016970b0dc7ea9c72c08e2e494a01020304100000\"")
                    + "\nAs json rpc call\n"
                    + HelpExampleRpc("gettxoperationlog",
                            "\"0001a87352387b5b4d6d01299c0dc178ff044f42e016970b0dc7ea9c72c08e2e494a01020304100000\""));
    }
    RPCTypeCheck(params, list_of(str_type));
    uint256 txid(uint256S(params[0].get_str()));
    vector<CAccount> vLog;
    Object retobj;
    retobj.push_back(Pair("txid", txid.GetHex()));
    // TODO: improve the tx oper log
    if (!GetTxOperLog(txid, vLog))
        throw JSONRPCError(RPC_INVALID_PARAMETER, "error hash");
    {
        Array arrayvLog;
        for (auto const &account : vLog) {
            Object obj;
            obj.push_back(Pair("addr", account.keyid.ToAddress()));
            Array array;
            array.push_back(account.ToJsonObj());
            arrayvLog.push_back(obj);
        }
        retobj.push_back(Pair("AccountOperLog", arrayvLog));

    }
    return retobj;

}

static Value TestDisconnectBlock(int number) {
    CBlock block;
    Object obj;

    CValidationState state;
    if ((chainActive.Tip()->height - number) < 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "restclient Error: number");
    }
    if (number > 0) {
        do {
            CBlockIndex * pTipIndex = chainActive.Tip();
            LogPrint("DEBUG", "current height:%d\n", pTipIndex->height);
            if (!DisconnectBlockFromTip(state))
                return false;
            chainMostWork.SetTip(pTipIndex->pprev);
            if (!EraseBlockIndexFromSet(pTipIndex))
                return false;
            if (!pCdMan->pBlockTreeDb->EraseBlockIndex(pTipIndex->GetBlockHash()))
                return false;
            mapBlockIndex.erase(pTipIndex->GetBlockHash());
        } while (--number);
    }

    obj.push_back(Pair("tip", strprintf("hash:%s hight:%s",chainActive.Tip()->GetBlockHash().ToString(),chainActive.Tip()->height)));
    return obj;
}

Value disconnectblock(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("disconnectblock \"numbers\"\n"
                "\ndisconnect block\n"
                "\nArguments:\n"
                "1. \"numbers \"  (numeric, required) the block numbers.\n"
                "\nResult:\n"
                "\"disconnect result\"  (bool) \n"
                "\nExamples:\n"
                + HelpExampleCli("disconnectblock", "\"1\"")
                + "\nAs json rpc call\n"
                + HelpExampleRpc("disconnectblock", "\"1\""));
    }
    int number = params[0].get_int();

    Value te = TestDisconnectBlock(number);

    return te;
}

Value listcontracts(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "listcontracts \"show detail\"\n"
            "\nget the list of all registered contracts\n"
            "\nArguments:\n"
            "1. show detail  (boolean, required) show contract script content if true.\n"
            "\nReturn an object contains all contract script data\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("listcontracts", "true") + "\nAs json rpc call\n" + HelpExampleRpc("listcontracts", "true"));
    }
    bool showDetail = false;
    showDetail      = params[0].get_bool();

    map<string, CUniversalContract> contracts;
    if (!pCdMan->pContractCache->GetContracts(contracts)) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "Failed to acquire contract scripts.");
    }

    Object obj;
    Array scriptArray;
    for (const auto &item : contracts) {
        Object scriptObject;
        const CUniversalContract &contract = item.second;
        CRegID regid(item.first);
        scriptObject.push_back( Pair("contract_regid", regid.ToString()) );
        scriptObject.push_back( Pair("memo", HexStr(contract.memo)) );
        if (showDetail) {
            scriptObject.push_back(Pair("contract", HexStr(contract.code)));
        }

        scriptArray.push_back(scriptObject);
    }

    obj.push_back(Pair("count", contracts.size()));
    obj.push_back(Pair("contracts", scriptArray));

    return obj;
}

Value getcontractinfo(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "getcontractinfo ( \"contract regid\" )\n"
            "\nget contract information.\n"
            "\nArguments:\n"
            "1. \"contract regid\"    (string, required) the script ID. \n"
            "\nget contract information\n"
            "\nExamples:\n" +
            HelpExampleCli("getcontractinfo", "123-1") + "\nAs json rpc call\n" +
            HelpExampleRpc("getcontractinfo", "123-1"));

    string strRegId = params[0].get_str();
    CRegID regId(strRegId);
    if (regId.IsEmpty() == true) {
        throw runtime_error("in getcontractinfo: contract regid size invalid!\n");
    }

    if (!pCdMan->pContractCache->HaveContract(regId)) {
        throw runtime_error("in getcontractinfo: contract regid not exist!\n");
    }

    CUniversalContract contract;
    if (!pCdMan->pContractCache->GetContract(regId, contract)) {
        throw JSONRPCError(RPC_DATABASE_ERROR, "get script error: cannot get registered script.");
    }

    Object obj;
    obj.push_back(Pair("contract_regid", regId.ToString()));
    obj.push_back(Pair("contract_memo", HexStr(contract.memo)));
    obj.push_back(Pair("contract_content", HexStr(contract.code)));
    return obj;
}

Value getaddrbalance(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        string msg = "getaddrbalance nrequired [\"key\",...] ( \"account\" )\n"
                "\nAdd a nrequired-to-sign multisignature address to the wallet.\n"
                "Each key is a  address or hex-encoded public key.\n" + HelpExampleCli("getaddrbalance", "")
                + "\nAs json rpc call\n" + HelpExampleRpc("getaddrbalance", "");
        throw runtime_error(msg);
    }

    assert(pWalletMain != nullptr);

    CKeyID keyId;
    if (!GetKeyId(params[0].get_str(), keyId))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid  address");

    double balance = 0.0;
    CAccount account;
    if (pCdMan->pAccountCache->GetAccount(keyId, account)) {
        balance = (double)account.GetToken(SYMB::WICC).free_amount / (double)COIN;
    }

    return balance;
}

Value generateblock(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("generateblock \"addr\"\n"
            "\ncreate a block with the appointed address\n"
            "\nArguments:\n"
            "1.\"addr\": (string, required)\n"
            "\nResult:\n"
            "\nblockhash\n"
            "\nExamples:\n" +
            HelpExampleCli("generateblock", "\"5Vp1xpLT8D2FQg3kaaCcjqxfdFNRhxm4oy7GXyBga9\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("generateblock", "\"5Vp1xpLT8D2FQg3kaaCcjqxfdFNRhxm4oy7GXyBga9\""));
    }
    //get keyId
    CKeyID keyId;

    if (!GetKeyId(params[0].get_str(), keyId))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "in generateblock :address err");

//  uint256 hash = CreateBlockWithAppointedAddr(keyId);
//  if (hash.IsNull()) {
//      throw runtime_error("in generateblock :cannot generate block\n");
//  }
    Object obj;
//  obj.push_back(Pair("blockhash", hash.GetHex()));
    return obj;
}

Value listtxcache(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error("listtxcache\n"
                "\nget all transactions in cahce\n"
                "\nArguments:\n"
                "\nResult:\n"
                "\"txcache\"  (string) \n"
                "\nExamples:\n" + HelpExampleCli("listtxcache", "")+ HelpExampleRpc("listtxcache", ""));
    }
    const map<uint256, UnorderedHashSet> &mapBlockTxHashSet = pCdMan->pTxCache->GetTxHashCache();

    Array retTxHashArray;
    for (auto &item : mapBlockTxHashSet) {
        Object blockObj;
        Array txHashArray;
        blockObj.push_back(Pair("blockhash", item.first.GetHex()));
        for (auto &txid : item.second)
            txHashArray.push_back(txid.GetHex());
        blockObj.push_back(Pair("txcache", txHashArray));
        retTxHashArray.push_back(blockObj);
    }

    return retTxHashArray;
}

Value reloadtxcache(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 0) {
        throw runtime_error("reloadtxcache \n"
            "\nreload transactions catch\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("reloadtxcache", "")
            + HelpExampleRpc("reloadtxcache", ""));
    }
    pCdMan->pTxCache->Clear();
    CBlockIndex *pIndex = chainActive.Tip();
    if ((chainActive.Tip()->height - SysCfg().GetTxCacheHeight()) >= 0) {
        pIndex = chainActive[(chainActive.Tip()->height - SysCfg().GetTxCacheHeight())];
    } else {
        pIndex = chainActive.Genesis();
    }
    CBlock block;
    do {
        if (!ReadBlockFromDisk(pIndex, block))
            return ERRORMSG("reloadtxcache() : *** ReadBlockFromDisk failed at %d, hash=%s",
                pIndex->height, pIndex->GetBlockHash().ToString());

        pCdMan->pTxCache->AddBlockToCache(block);
        pIndex = chainActive.Next(pIndex);
    } while (nullptr != pIndex);

    Object obj;
    obj.push_back(Pair("info", "reload tx cache succeed"));
    return obj;
}

Value getcontractdata(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 2 && params.size() != 3)) {
        throw runtime_error(
            "getcontractdata \"contract regid\" \"key\" [hexadecimal]\n"
            "\nget contract data with key\n"
            "\nArguments:\n"
            "1.\"contract regid\":      (string, required) contract regid\n"
            "2.\"key\":                 (string, required)\n"
            "3.\"hexadecimal format\":  (boolean, optional) in hexadecimal if true, otherwise in plaintext, default to "
            "false\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("getcontractdata", "\"1304166-1\" \"key\" true") + "\nAs json rpc call\n" +
            HelpExampleRpc("getcontractdata", "\"1304166-1\", \"key\", true"));
    }

    CRegID regId(params[0].get_str());
    if (regId.IsEmpty()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid contract regid");
    }

    bool hexadecimal = params.size() > 2 ? params[2].get_bool() : false;
    string key;
    if (hexadecimal) {
        vector<unsigned char> hexKey = ParseHex(params[1].get_str());
        key                          = string(hexKey.begin(), hexKey.end());
    } else {
        key = params[1].get_str();
    }
    string value;
    if (!pCdMan->pContractCache->GetContractData(regId, key, value)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to acquire contract data");
    }

    Object obj;
    obj.push_back(Pair("contract_regid",    regId.ToString()));
    obj.push_back(Pair("key",               hexadecimal ? HexStr(key) : key));
    obj.push_back(Pair("value",             hexadecimal ? HexStr(value) : value));

    return obj;
}

Value saveblocktofile(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2) {
        throw runtime_error(
            "saveblocktofile \"blockhash\" \"filepath\"\n"
            "\n save the given block info to the given file\n"
            "\nArguments:\n"
            "1.\"blockhash\": (string, required)\n"
            "2.\"filepath\": (string, required)\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("saveblocktofile",
                           "\"c78d162b40625cc8b088fa88302e0e4f08aba0d1c92612e9dd14e77108cbc11a\" \"block.log\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("saveblocktofile",
                           "\"c78d162b40625cc8b088fa88302e0e4f08aba0d1c92612e9dd14e77108cbc11a\", \"block.log\""));
    }
    string strblockhash = params[0].get_str();
    uint256 blockHash(uint256S(params[0].get_str()));
    if(0 == mapBlockIndex.count(blockHash)) {
        throw JSONRPCError(RPC_MISC_ERROR, "block hash is not exist!");
    }
    CBlockIndex *pIndex = mapBlockIndex[blockHash];
    CBlock blockInfo;
    if (!pIndex || !ReadBlockFromDisk(pIndex, blockInfo))
        throw runtime_error(_("Failed to read block"));
    assert(strblockhash == blockInfo.GetHash().ToString());
    string file = params[1].get_str();
    try {
        FILE* fp = fopen(file.c_str(), "wb+");
        CAutoFile fileout = CAutoFile(fp, SER_DISK, CLIENT_VERSION);
        if (!fileout)
            throw JSONRPCError(RPC_MISC_ERROR, "open file:" + strblockhash + "failed!");
        if(chainActive.Contains(pIndex))
            fileout << pIndex->height;
        fileout << blockInfo;
        fflush(fileout);
    } catch (std::exception &e) {
        throw JSONRPCError(RPC_MISC_ERROR, "save block to file error");
    }
    return "save succeed";
}

Value genregisteraccountraw(const Array& params, bool fHelp) {
    if (fHelp || (params.size() < 3 || params.size() > 4)) {
        throw runtime_error(
            "genregisteraccountraw \"fee\" \"height\" \"publickey\" (\"minerpublickey\") \n"
            "\ncreate a register account transaction\n"
            "\nArguments:\n"
            "1.fee: (numeric, required) pay to miner\n"
            "2.height: (numeric, required)\n"
            "3.publickey: (string, required)\n"
            "4.minerpublickey: (string, optional)\n"
            "\nResult:\n"
            "\"txid\": (string)\n"
            "\nExamples:\n" +
            HelpExampleCli(
                "genregisteraccountraw",
                "10000  3300 "
                "\"038f679e8b63d6f9935e8ca6b7ce1de5257373ac5461874fc794004a8a00a370ae\" "
                "\"026bc0668c767ab38a937cb33151bcf76eeb4034bcb75e1632fd1249d1d0b32aa9\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc(
                "genregisteraccountraw",
                " 10000, 3300, "
                "\"038f679e8b63d6f9935e8ca6b7ce1de5257373ac5461874fc794004a8a00a370ae\", "
                "\"026bc0668c767ab38a937cb33151bcf76eeb4034bcb75e1632fd1249d1d0b32aa9\""));
    }
    CUserID userId  = CNullID();
    CUserID minerId = CNullID();

    int64_t fee         = AmountToRawValue(params[0]);
    int64_t nDefaultFee = SysCfg().GetTxFee();

    if (fee < nDefaultFee) {
        throw JSONRPCError(RPC_INSUFFICIENT_FEE,
                           strprintf("Input fee smaller than mintxfee: %ld sawi", nDefaultFee));
    }

    int height = params[1].get_int();
    if (height <= 0) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid height");
    }

    CPubKey pubKey = CPubKey(ParseHex(params[2].get_str()));
    if (!pubKey.IsCompressed() || !pubKey.IsFullyValid()) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid public key");
    }
    userId = pubKey;

    if (params.size() > 3) {
        CPubKey minerPubKey = CPubKey(ParseHex(params[3].get_str()));
        if (!minerPubKey.IsCompressed() || !minerPubKey.IsFullyValid()) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid public key");
        }
        minerId = minerPubKey;
    }

    EnsureWalletIsUnlocked();
    std::shared_ptr<CAccountRegisterTx> tx =
        std::make_shared<CAccountRegisterTx>(userId, minerId, fee, height);
    if (!pWalletMain->Sign(pubKey.GetKeyId(), tx->ComputeSignatureHash(), tx->signature)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");
    }
    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
    ds << pBaseTx;
    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));
    return obj;
}

Value sendtxraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "sendtxraw \"transaction\" \n"
            "\nsend raw transaction\n"
            "\nArguments:\n"
            "1.\"transaction\": (string, required)\n"
            "\nExamples:\n"
            + HelpExampleCli("sendtxraw", "\"n2dha9w3bz2HPVQzoGKda3Cgt5p5Tgv6oj\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("sendtxraw", "\"n2dha9w3bz2HPVQzoGKda3Cgt5p5Tgv6oj\""));
    }
    vector<unsigned char> vch(ParseHex(params[0].get_str()));
    if (vch.size() > MAX_RPC_SIG_STR_LEN) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "The rawtx str is too long");
    }

    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);

    std::shared_ptr<CBaseTx> tx;
    stream >> tx;
    std::tuple<bool, string> ret;
    ret = pWalletMain->CommitTx((CBaseTx *) tx.get());
    if (!std::get<0>(ret))
        throw JSONRPCError(RPC_WALLET_ERROR, "sendtxraw error: " + std::get<1>(ret));

    Object obj;
    obj.push_back(Pair("txid", std::get<1>(ret)));
    return obj;
}

Value gencallcontractraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 5 || params.size() > 6) {
        throw runtime_error(
            "gencallcontractraw \"sender addr\" \"app regid\" \"amount\" \"contract\" \"fee\" (\"height\")\n"
            "\ncreate contract invocation raw transaction\n"
            "\nArguments:\n"
            "1.\"sender addr\": (string, required)\n tx sender's base58 addr\n"
            "2.\"app regid\":(string, required) contract RegId\n"
            "3.\"arguments\": (string, required) contract arguments (Hex encode required)\n"
            "4.\"amount\":(numeric, required)\n amount of WICC to be sent to the contract account\n"
            "5.\"fee\": (numeric, required) pay to miner\n"
            "6.\"height\": (numeric, optional)create height,If not provide use the tip block height in "
            "chainActive\n"
            "\nResult:\n"
            "\"rawtx\"  (string) The raw transaction\n"
            "\nExamples:\n" +
            HelpExampleCli("gencallcontractraw",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\" \"411994-1\" \"01020304\" 10000 10000 100") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("gencallcontractraw",
                           "\"wQWKaN4n7cr1HLqXY3eX65rdQMAL5R34k6\", \"411994-1\", \"01020304\", 10000, 10000, 100"));
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(int_type)(str_type)(int_type)(int_type));

    EnsureWalletIsUnlocked();

    CKeyID sendKeyId, recvKeyId;
    if (!GetKeyId(params[0].get_str(), sendKeyId))
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid sendaddress");

    if (!GetKeyId(params[1].get_str(), recvKeyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Invalid app regid");
    }

    string arguments = ParseHexStr(params[2].get_str());
    if (arguments.size() >= kContractArgumentMaxSize) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Arguments's size out of range");
    }

    int64_t amount = AmountToRawValue(params[3]);;
    int64_t fee = AmountToRawValue(params[4]);

    int height = chainActive.Tip()->height;
    if (params.size() > 5)
        height = params[5].get_int();

    CPubKey sendPubKey;
    if (!pWalletMain->GetPubKey(sendKeyId, sendPubKey)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Key not found in the local wallet.");
    }

    CUserID sendUserId;
    CRegID sendRegId;
    sendUserId = (  pCdMan->pAccountCache->GetRegId(CUserID(sendKeyId), sendRegId) &&
                    pCdMan->pAccountCache->RegIDIsMature(sendRegId))
                        ? CUserID(sendRegId)
                        : CUserID(sendPubKey);

    CRegID recvRegId;
    if (!pCdMan->pAccountCache->GetRegId(CUserID(recvKeyId), recvRegId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid app regid");
    }

    if (!pCdMan->pContractCache->HaveContract(recvRegId)) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Failed to get contract");
    }

    CLuaContractInvokeTx tx;
    tx.nTxType      = LCONTRACT_INVOKE_TX;
    tx.txUid        = sendUserId;
    tx.app_uid      = recvRegId;
    tx.bcoins       = amount;
    tx.llFees       = fee;
    tx.arguments    = arguments;
    tx.nValidHeight = height;

    if (!pWalletMain->Sign(sendKeyId, tx.ComputeSignatureHash(), tx.signature)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Sign failed");
    }

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = tx.GetNewInstance();
    ds << pBaseTx;
    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

    return obj;
}

Value genregistercontractraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() < 3 || params.size() > 5) {
        throw runtime_error(
            "genregistercontractraw \"addr\" \"filepath\" \"fee\"  \"height\" (\"script description\")\n"
            "\nregister script\n"
            "\nArguments:\n"
            "1.\"addr\": (string required)\n from address that registers the contract"
            "2.\"filepath\": (string required), script's file path\n"
            "3.\"fee\": (numeric required) pay to miner\n"
            "4.\"height\": (int optional) valid height\n"
            "5.\"script description\":(string optional) new script description\n"
            "\nResult:\n"
            "\"txid\": (string)\n"
            "\nExamples:\n"
            + HelpExampleCli("genregistercontractraw",
                    "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\" \"/tmp/lua/hello.lua\" \"10000\" ")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("genregistercontractraw",
                    "\"WiZx6rrsBn9sHjwpvdwtMNNX2o31s3DEHH\", \"/tmp/lua/hello.lua\", \"10000\" "));
    }

    RPCTypeCheck(params, list_of(str_type)(str_type)(int_type)(int_type)(str_type));

    string luaScriptFilePath = GetAbsolutePath(params[1].get_str()).string();
    if (luaScriptFilePath.empty())
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_NOT_EXIST, "Lua Script file not exist!");

    if (luaScriptFilePath.compare(0, kContractScriptPathPrefix.size(), kContractScriptPathPrefix.c_str()) != 0)
        throw JSONRPCError(RPC_SCRIPT_FILEPATH_INVALID, "Lua Script file not inside /tmp/lua dir or its subdir!");

    FILE* file = fopen(luaScriptFilePath.c_str(), "rb+");
    if (!file)
        throw runtime_error("genregistercontractraw open App Lua Script file" + luaScriptFilePath + "error");

    long lSize;
    fseek(file, 0, SEEK_END);
    lSize = ftell(file);
    rewind(file);

    // allocate memory to contain the whole file:
    char *buffer = (char*) malloc(sizeof(char) * lSize);
    if (buffer == nullptr) {
        fclose(file);
        throw runtime_error("allocate memory failed");
    }

    if (fread(buffer, 1, lSize, file) != (size_t) lSize) {
        free(buffer);
        fclose(file);
        throw runtime_error("read contract script file error");
    } else {
        fclose(file);
    }
    string code(buffer, lSize);

    if (buffer) {
        free(buffer);
    }

    string memo;
    if (params.size() > 4) {
        memo = params[4].get_str();
    }

    uint64_t fee = params[2].get_uint64();
    if (fee > 0 && fee < CBaseTx::nMinTxFee) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Fee smaller than nMinTxFee");
    }
    CKeyID keyId;
    if (!GetKeyId(params[0].get_str(), keyId)) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Recv address invalid");
    }

    std::shared_ptr<CAccountDBCache> pAccountCache(new CAccountDBCache(pCdMan->pAccountCache));
    CAccount account;
    CUserID userId = keyId;
    if (!pAccountCache->GetAccount(userId, account)) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Account does not exist");
    }
    if (!account.HaveOwnerPubKey()) {
        throw JSONRPCError(RPC_WALLET_ERROR, "Account is unregistered");
    }

    std::shared_ptr<CLuaContractDeployTx> tx = std::make_shared<CLuaContractDeployTx>();
    CRegID regId;
    pAccountCache->GetRegId(keyId, regId);

    tx->txUid          = regId;
    tx->contract.code  = code;
    tx->contract.memo  = memo;
    tx->llFees         = fee;

    uint32_t height = chainActive.Tip()->height;
    if (params.size() > 3) {
        height =  params[3].get_int();
        if (height <= 0) {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid height");
        }
    }
    tx.get()->nValidHeight = height;

    CDataStream ds(SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
    ds << pBaseTx;

    Object obj;
    obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

    return obj;
}

Value signtxraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2) {
        throw runtime_error(
            "signtxraw \"str\" \"addr\"\n"
            "\nsignature transaction\n"
            "\nArguments:\n"
            "1.\"str\": (string, required) Hex-format string, no longer than 65K in binary bytes\n"
            "2.\"addr\": (string, required) A json array of WICC addresses\n"
            "[\n"
            "  \"address\"  (string) WICC address\n"
            "  ...,\n"
            "]\n"
            "\nExamples:\n" +
            HelpExampleCli("signtxraw",
                           "\"0701ed7f0300030000010000020002000bcd10858c200200\" "
                           "\"[\\\"wKwPHfCJfUYZyjJoa6uCVdgbVJkhEnguMw\\\", "
                           "\\\"wQT2mY1onRGoERTk4bgAoAEaUjPLhLsrY4\\\", "
                           "\\\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\\\"]\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("signtxraw",
                           "\"0701ed7f0300030000010000020002000bcd10858c200200\", "
                           "\"[\\\"wKwPHfCJfUYZyjJoa6uCVdgbVJkhEnguMw\\\", "
                           "\\\"wQT2mY1onRGoERTk4bgAoAEaUjPLhLsrY4\\\", "
                           "\\\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\\\"]\""));
    }

    vector<unsigned char> vch(ParseHex(params[0].get_str()));
    if (vch.size() > MAX_RPC_SIG_STR_LEN) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "The sig str is too long");
    }

    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx;
    stream >> pBaseTx;
    if (!pBaseTx.get()) {
        return Value::null;
    }

    const Array& addresses = params[1].get_array();
    if (pBaseTx.get()->nTxType != BCOIN_TRANSFER_MTX && addresses.size() != 1) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "To many addresses provided");
    }

    std::set<CKeyID> keyIds;
    CKeyID keyId;
    for (unsigned int i = 0; i < addresses.size(); i++) {
        if (!GetKeyId(addresses[i].get_str(), keyId)) {
            throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "Failed to get keyId");
        }
        keyIds.insert(keyId);
    }

    if (keyIds.empty()) {
        throw JSONRPCError(RPC_INVALID_ADDRESS_OR_KEY, "No valid address provided");
    }

    Object obj;
    switch (pBaseTx.get()->nTxType) {
        case BCOIN_TRANSFER_TX: {
            std::shared_ptr<CBaseCoinTransferTx> tx = std::make_shared<CBaseCoinTransferTx>(pBaseTx.get());
            if (!pWalletMain->Sign(*keyIds.begin(), tx.get()->ComputeSignatureHash(), tx.get()->signature))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");

            CDataStream ds(SER_DISK, CLIENT_VERSION);
            std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }

        case ACCOUNT_REGISTER_TX: {
            std::shared_ptr<CAccountRegisterTx> tx =
                std::make_shared<CAccountRegisterTx>(pBaseTx.get());
            if (!pWalletMain->Sign(*keyIds.begin(), tx.get()->ComputeSignatureHash(), tx.get()->signature))
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");

            CDataStream ds(SER_DISK, CLIENT_VERSION);
            std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }

        case LCONTRACT_INVOKE_TX: {
            std::shared_ptr<CLuaContractInvokeTx> tx = std::make_shared<CLuaContractInvokeTx>(pBaseTx.get());
            if (!pWalletMain->Sign(*keyIds.begin(), tx.get()->ComputeSignatureHash(), tx.get()->signature)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");
            }
            CDataStream ds(SER_DISK, CLIENT_VERSION);
            std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }

        case BLOCK_REWARD_TX: {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Block reward transation is forbidden");
        }

        case LCONTRACT_DEPLOY_TX: {
            std::shared_ptr<CLuaContractDeployTx> tx =
                std::make_shared<CLuaContractDeployTx>(pBaseTx.get());
            if (!pWalletMain->Sign(*keyIds.begin(), tx.get()->ComputeSignatureHash(), tx.get()->signature)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");
            }
            CDataStream ds(SER_DISK, CLIENT_VERSION);
            std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }

        case DELEGATE_VOTE_TX: {
            std::shared_ptr<CDelegateVoteTx> tx = std::make_shared<CDelegateVoteTx>(pBaseTx.get());
            if (!pWalletMain->Sign(*keyIds.begin(), tx.get()->ComputeSignatureHash(), tx.get()->signature)) {
                throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");
            }
            CDataStream ds(SER_DISK, CLIENT_VERSION);
            std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }

        case BCOIN_TRANSFER_MTX: {
            std::shared_ptr<CMulsigTx> tx = std::make_shared<CMulsigTx>(pBaseTx.get());

            vector<CSignaturePair>& signaturePairs = tx.get()->signaturePairs;
            for (const auto& keyIdItem : keyIds) {
                CRegID regId;
                if (!pCdMan->pAccountCache->GetRegId(CUserID(keyIdItem), regId)) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Address is unregistered");
                }

                bool valid = false;
                for (auto& signatureItem : signaturePairs) {
                    if (regId == signatureItem.regid) {
                        if (!pWalletMain->Sign(keyIdItem, tx.get()->ComputeSignatureHash(),
                                               signatureItem.signature)) {
                            throw JSONRPCError(RPC_INVALID_PARAMETER, "Sign failed");
                        } else {
                            valid = true;
                        }
                    }
                }

                if (!valid) {
                    throw JSONRPCError(RPC_INVALID_PARAMETER, "Provided address is unmatched");
                }
            }

            CDataStream ds(SER_DISK, CLIENT_VERSION);
            std::shared_ptr<CBaseTx> pBaseTx = tx->GetNewInstance();
            ds << pBaseTx;
            obj.push_back(Pair("rawtx", HexStr(ds.begin(), ds.end())));

            break;
        }

        default: {
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Unsupported transaction type");
        }
    }
    return obj;
}

Value decodemulsigscript(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "decodemulsigscript \"hex\"\n"
            "\nDecode a hex-encoded script.\n"
            "\nArguments:\n"
            "1. \"hex\"     (string) the hex encoded mulsig script\n"
            "\nResult:\n"
            "{\n"
            "  \"type\":\"type\", (string) The transaction type\n"
            "  \"reqSigs\": n,    (numeric) The required signatures\n"
            "  \"addr\",\"address\" (string) mulsig script address\n"
            "  \"addresses\": [   (json array of string)\n"
            "     \"address\"     (string) bitcoin address\n"
            "     ,...\n"
            "  ]\n"
            "}\n"
            "\nExamples:\n" +
            HelpExampleCli("decodemulsigscript", "\"hexstring\"") +
            HelpExampleRpc("decodemulsigscript", "\"hexstring\""));

    RPCTypeCheck(params, list_of(str_type));

    vector<unsigned char> multiScript = ParseHex(params[0].get_str());
    if (multiScript.empty() || multiScript.size() > KMultisigScriptMaxSize) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid script size");
    }

    CDataStream ds(multiScript, SER_DISK, CLIENT_VERSION);
    CMulsigScript script;
    try {
        ds >> script;
    } catch (std::exception& e) {
        throw JSONRPCError(RPC_INVALID_PARAMETER, "Invalid script content");
    }

    CKeyID scriptId           = script.GetID();
    int8_t required           = (int8_t)script.GetRequired();
    std::set<CPubKey> pubKeys = script.GetPubKeys();

    Array addressArray;
    for (const auto& pubKey : pubKeys) {
        addressArray.push_back(pubKey.GetKeyId().ToAddress());
    }

    Object obj;
    obj.push_back(Pair("type", "mulsig"));
    obj.push_back(Pair("req_sigs", required));
    obj.push_back(Pair("addr", scriptId.ToAddress()));
    obj.push_back(Pair("addresses", addressArray));

    return obj;
}

Value decodetxraw(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error(
            "decodetxraw \"hexstring\"\n"
            "\ndecode transaction\n"
            "\nArguments:\n"
            "1.\"str\": (string, required) hexstring\n"
            "\nExamples:\n" +
            HelpExampleCli("decodetxraw",
                           "\"03015f020001025a0164cd10004630440220664de5ec373f44d2756a23d5267ab25f2"
                           "2af6162d166b1cca6c76631701cbeb5022041959ff75f7c7dd39c1f9f6ef9a237a6ea46"
                           "7d02d2d2c3db62a1addaa8009ccd\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("decodetxraw",
                           "\"03015f020001025a0164cd10004630440220664de5ec373f44d2756a23d5267ab25f2"
                           "2af6162d166b1cca6c76631701cbeb5022041959ff75f7c7dd39c1f9f6ef9a237a6ea46"
                           "7d02d2d2c3db62a1addaa8009ccd\""));
    }
    Object obj;
    vector<unsigned char> vch(ParseHex(params[0].get_str()));
    CDataStream stream(vch, SER_DISK, CLIENT_VERSION);
    std::shared_ptr<CBaseTx> pBaseTx;
    stream >> pBaseTx;
    if (!pBaseTx.get()) {
        return obj;
    }

    switch (pBaseTx.get()->nTxType) {
        case BCOIN_TRANSFER_TX: {
            std::shared_ptr<CBaseCoinTransferTx> tx = std::make_shared<CBaseCoinTransferTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }
        case ACCOUNT_REGISTER_TX: {
            std::shared_ptr<CAccountRegisterTx> tx =
                std::make_shared<CAccountRegisterTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }
        case WASM_CONTRACT_TX: {
            std::shared_ptr<CWasmContractTx> tx = std::make_shared<CWasmContractTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }
        case LCONTRACT_INVOKE_TX: {
            std::shared_ptr<CLuaContractInvokeTx> tx = std::make_shared<CLuaContractInvokeTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }
        case BLOCK_REWARD_TX: {
            std::shared_ptr<CBlockRewardTx> tx = std::make_shared<CBlockRewardTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }
        case LCONTRACT_DEPLOY_TX: {
            std::shared_ptr<CLuaContractDeployTx> tx =
                std::make_shared<CLuaContractDeployTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }
        case DELEGATE_VOTE_TX: {
            std::shared_ptr<CDelegateVoteTx> tx = std::make_shared<CDelegateVoteTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }
        case BCOIN_TRANSFER_MTX: {
            std::shared_ptr<CMulsigTx> tx = std::make_shared<CMulsigTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }

        // TODO: UCOIN_LCONTRACT_INVOKE_TX
        case UCOIN_TRANSFER_TX: {
           std::shared_ptr<CCoinTransferTx> tx = std::make_shared<CCoinTransferTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }
        case UCOIN_REWARD_TX: {
            std::shared_ptr<CCoinRewardTx> tx = std::make_shared<CCoinRewardTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }

        case CDP_STAKE_TX: {
            std::shared_ptr<CCDPStakeTx> tx = std::make_shared<CCDPStakeTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }
        case CDP_REDEEM_TX: {
            std::shared_ptr<CCDPRedeemTx> tx = std::make_shared<CCDPRedeemTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }
        case CDP_LIQUIDATE_TX: {
            std::shared_ptr<CCDPLiquidateTx> tx = std::make_shared<CCDPLiquidateTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }

        case PRICE_FEED_TX: {
            std::shared_ptr<CPriceFeedTx> tx = std::make_shared<CPriceFeedTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }
        case PRICE_MEDIAN_TX: {
            std::shared_ptr<CBlockPriceMedianTx> tx = std::make_shared<CBlockPriceMedianTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }

        // TODO: SFC_PARAM_MTX
        // TODO: SFC_GLOBAL_HALT_MTX
        // TODO: SFC_GLOBAL_SETTLE_MTX

        case FCOIN_STAKE_TX: {
            std::shared_ptr<CFcoinStakeTx> tx = std::make_shared<CFcoinStakeTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }

        case DEX_TRADE_SETTLE_TX: {
            std::shared_ptr<CDEXSettleTx> tx = std::make_shared<CDEXSettleTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }
        case DEX_CANCEL_ORDER_TX: {
            std::shared_ptr<CDEXCancelOrderTx> tx = std::make_shared<CDEXCancelOrderTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }
        case DEX_LIMIT_BUY_ORDER_TX: {
            std::shared_ptr<CDEXBuyLimitOrderTx> tx = std::make_shared<CDEXBuyLimitOrderTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }
        case DEX_LIMIT_SELL_ORDER_TX: {
            std::shared_ptr<CDEXSellLimitOrderTx> tx = std::make_shared<CDEXSellLimitOrderTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }
        case DEX_MARKET_BUY_ORDER_TX: {
            std::shared_ptr<CDEXBuyMarketOrderTx> tx = std::make_shared<CDEXBuyMarketOrderTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }
        case DEX_MARKET_SELL_ORDER_TX: {
            std::shared_ptr<CDEXSellMarketOrderTx> tx = std::make_shared<CDEXSellMarketOrderTx>(pBaseTx.get());
            if (tx.get()) {
                obj = tx->ToJson(*pCdMan->pAccountCache);
            }
            break;
        }

        default:
            break;
    }
    return obj;
}

Value getalltxinfo(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 0 && params.size() != 1)) {
        throw runtime_error("getalltxinfo \n"
            "\nget all transaction info\n"
            "\nArguments:\n"
            "1.\"nlimitCount\": (numeric, optional, default=0) 0 return all tx, else return number of nlimitCount txs \n"
            "\nResult:\n"
            "\nExamples:\n" + HelpExampleCli("getalltxinfo", "") + "\nAs json rpc call\n"
            + HelpExampleRpc("getalltxinfo", ""));
    }

    Object retObj;
    int nLimitCount(0);
    if(params.size() == 1)
        nLimitCount = params[0].get_int();
    assert(pWalletMain != nullptr);
    if (nLimitCount <= 0) {
        Array confirmedTx;
        for (auto const &wtx : pWalletMain->mapInBlockTx) {
            for (auto const & item : wtx.second.mapAccountTx) {
                Object objtx = GetTxDetailJSON(item.first);
                confirmedTx.push_back(objtx);
            }
        }
        retObj.push_back(Pair("confirmed", confirmedTx));

        Array unconfirmedTx;
        CAccountDBCache view(*pCdMan->pAccountCache);
        for (auto const &wtx : pWalletMain->unconfirmedTx) {
            Object objtx = GetTxDetailJSON(wtx.first);
            unconfirmedTx.push_back(objtx);
        }
        retObj.push_back(Pair("unconfirmed", unconfirmedTx));
    } else {
        Array confirmedTx;
        multimap<int, Object, std::greater<int> > mapTx;
        for (auto const &wtx : pWalletMain->mapInBlockTx) {
            for (auto const & item : wtx.second.mapAccountTx) {
                Object objtx = GetTxDetailJSON(item.first);
                int nConfHeight = find_value(objtx, "confirmedheight").get_int();
                mapTx.insert(pair<int, Object>(nConfHeight, objtx));
            }
        }
        int nSize(0);
        for(auto & txItem : mapTx) {
            if(++nSize > nLimitCount)
                break;
            confirmedTx.push_back(txItem.second);
        }
        retObj.push_back(Pair("Confirmed", confirmedTx));
    }

    return retObj;
}

Value gettxreceipt(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 0 && params.size() != 1)) {
        throw runtime_error("gettxreceipt \n"
            "\nget tx receipts by txid\n"
            "\nArguments:\n"
            "1.\"txid\": (string) txid \n"
            "\nResult:\n"
            "\nExamples:\n" + HelpExampleCli("gettxreceipt", "") + "\nAs json rpc call\n"
            + HelpExampleRpc("gettxreceipt", ""));
    }
    uint256 txid = uint256S(params[0].get_str());
    vector<CReceipt> receipts;
    pCdMan->pTxReceiptCache->GetTxReceipts(txid, receipts);

    Array retArray;
    for (const auto &receipt : receipts) {
        Object obj;

        obj.push_back(Pair("tx_type",           std::get<0>(kTxFeeTable.at(receipt.tx_type))));
        obj.push_back(Pair("from_uid",         receipt.from_uid.ToString()));
        obj.push_back(Pair("to_uid",           receipt.to_uid.ToString()));
        obj.push_back(Pair("coin_symbol",       receipt.coin_symbol));
        obj.push_back(Pair("transfer_amount",   receipt.send_amount));
        retArray.push_back(obj);
    }
    return retArray;
}

Value getcontractaccountinfo(const Array& params, bool fHelp) {
    if (fHelp || (params.size() != 2 && params.size() != 3)) {
        throw runtime_error(
            "getcontractaccountinfo \"contract regid\" \"account address or regid\""
            "\nget contract account info\n"
            "\nArguments:\n"
            "1.\"contract regid\":              (string, required) contract regid\n"
            "2.\"account address or regid\":    (string, required) contract account address or its regid\n"
            "3.\"minconf\"                      (numeric, optional, default=1) Only include contract transactions "
            "confirmed\n"
            "\nExamples:\n" +
            HelpExampleCli("getcontractaccountinfo", "\"452974-3\" \"WUZBQZZqyWgJLvEEsHrXL5vg5qaUwgfjco\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("getcontractaccountinfo", "\"452974-3\", \"WUZBQZZqyWgJLvEEsHrXL5vg5qaUwgfjco\""));
    }

    string strAppRegId = params[0].get_str();
    if (!CRegID::IsSimpleRegIdStr(strAppRegId))
        throw runtime_error("getcontractaccountinfo: invalid contract regid: " + strAppRegId);

    CRegID appRegId(strAppRegId);

    string acctKey;
    if (CRegID::IsSimpleRegIdStr(params[1].get_str())) {
        CRegID acctRegId(params[1].get_str());
        CUserID acctUserId(acctRegId);
        acctKey = RegIDToAddress(acctUserId);
    } else { //in wicc address format
        acctKey = params[1].get_str();
    }

    std::shared_ptr<CAppUserAccount> appUserAccount = std::make_shared<CAppUserAccount>();
    if (params.size() == 3 && params[2].get_int() == 0) {
        if (!mempool.cw->contractCache.GetContractAccount(appRegId, acctKey, *appUserAccount.get())) {
            appUserAccount = std::make_shared<CAppUserAccount>(acctKey);
        }
    } else {
        if (!pCdMan->pContractCache->GetContractAccount(appRegId, acctKey, *appUserAccount.get())) {
            appUserAccount = std::make_shared<CAppUserAccount>(acctKey);
        }
    }
    appUserAccount.get()->AutoMergeFreezeToFree(chainActive.Tip()->height);

    return Value(appUserAccount.get()->ToJson());
}

Value listcontractassets(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("listcontractassets regid\n"
            "\nreturn Array containing address, asset information.\n"
            "\nArguments: regid: Contract RegId\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("listcontractassets", "1-1")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("listcontractassets", "1-1"));
    }

    if (!CRegID::IsSimpleRegIdStr(params[0].get_str()))
        throw runtime_error("in listcontractassets :regid is invalid!\n");

    CRegID script(params[0].get_str());

    Array retArray;
    assert(pWalletMain != nullptr);
    {
        set<CKeyID> setKeyId;
        pWalletMain->GetKeys(setKeyId);
        if (setKeyId.size() == 0)
            return retArray;

        CContractDBCache contractScriptTemp(*pCdMan->pContractCache);

        for (const auto &keyId : setKeyId) {

            string key = keyId.ToAddress();

            std::shared_ptr<CAppUserAccount> tem = std::make_shared<CAppUserAccount>();
            if (!contractScriptTemp.GetContractAccount(script, key, *tem.get())) {
                tem = std::make_shared<CAppUserAccount>(key);
            }
            tem.get()->AutoMergeFreezeToFree(chainActive.Tip()->height);

            Object obj;
            obj.push_back(Pair("addr", key));
            obj.push_back(Pair("asset", (double) tem.get()->GetBcoins() / (double) COIN));
            retArray.push_back(obj);
        }
    }

    return retArray;
}


Value gethash(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1) {
        throw runtime_error("gethash  \"str\"\n"
            "\nget the hash of given str\n"
            "\nArguments:\n"
            "1.\"str\": (string, required) \n"
            "\nresult an object \n"
            "\nExamples:\n"
            + HelpExampleCli("gethash", "\"0000001000005zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG\"")
            + "\nAs json rpc call\n"
            + HelpExampleRpc("gethash", "\"0000001000005zQPcC1YpFMtwxiH787pSXanUECoGsxUq3KZieJxVG\""));
    }

    string str = params[0].get_str();
    vector<unsigned char> vTemp;
    vTemp.assign(str.c_str(), str.c_str() + str.length());
    uint256 strhash = Hash(vTemp.begin(), vTemp.end());
    Object obj;
    obj.push_back(Pair("txid", strhash.ToString()));
    return obj;

}

Value validateaddr(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 1)
        throw runtime_error(
            "validateaddr \"wicc_address\"\n"
            "\ncheck whether address is valid or not\n"
            "\nArguments:\n"
            "1. \"wicc_address\"  (string, required) WICC address\n"
            "\nResult:\n"
            "\nExamples:\n" +
            HelpExampleCli("validateaddr", "\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\"") +
            HelpExampleRpc("validateaddr", "\"wNw1Rr8cHPerXXGt6yxEkAPHDXmzMiQBn4\""));

    Object obj;
    CKeyID keyId;
    string addr = params[0].get_str();
    if (!GetKeyId(addr, keyId)) {
        obj.push_back(Pair("is_valid", false));
    } else {
        obj.push_back(Pair("is_valid", true));
    }
    return obj;
}

Value gettotalcoins(const Array& params, bool fHelp) {
    if(fHelp || params.size() != 0) {
        throw runtime_error(
            "gettotalcoins \n"
            "\nget the total number of circulating coins excluding those locked for votes\n"
            "\nand the toal number of registered addresses\n"
            "\nArguments:\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("gettotalcoins", "")
            + HelpExampleRpc("gettotalcoins", ""));
    }

    Object obj;
    {
        uint64_t totalCoins(0);
        uint64_t totalRegIds(0);
        std::tie(totalCoins, totalRegIds) = pCdMan->pAccountCache->TraverseAccount();
        // auto [totalCoins, totalRegIds] = pCdMan->pAccountCache->TraverseAccount(); //C++17
        obj.push_back( Pair("total_coins", ValueFromAmount(totalCoins)) );
        obj.push_back( Pair("total_regids", totalRegIds) );
    }
    return obj;
}

Value gettotalassets(const Array& params, bool fHelp) {
    if(fHelp || params.size() != 1) {
        throw runtime_error("gettotalassets \n"
            "\nget all assets belonging to a contract\n"
            "\nArguments:\n"
            "1.\"contract_regid\": (string, required)\n"
            "\nResult:\n"
            "\nExamples:\n"
            + HelpExampleCli("gettotalassets", "11-1")
            + HelpExampleRpc("gettotalassets", "11-1"));
    }
    CRegID regId(params[0].get_str());
    if (regId.IsEmpty() == true)
        throw runtime_error("contract regid invalid!\n");

    if (!pCdMan->pContractCache->HaveContract(regId))
        throw runtime_error("contract regid not exist!\n");

    Object obj;
    {
        map<string, string> mapAcc;
        bool bRet = pCdMan->pContractCache->GetContractAccounts(regId, mapAcc);
        if (bRet) {
            uint64_t totalassets = 0;
            for (auto & it : mapAcc) {
                CAppUserAccount appAccOut;
                CDataStream ds(it.second, SER_DISK, CLIENT_VERSION);
                ds >> appAccOut;

                totalassets += appAccOut.GetBcoins();
                totalassets += appAccOut.GetAllFreezedValues();
            }

            obj.push_back(Pair("total_assets", ValueFromAmount(totalassets)));
        } else
            throw runtime_error("failed to find contract account!\n");
    }
    return obj;
}

Value listtxbyaddr(const Array& params, bool fHelp) {
    if (fHelp || params.size() != 2) {
        throw runtime_error(
            "listtxbyaddr \n"
            "\nlist all transactions by their sender/receiver addresss\n"
            "\nArguments:\n"
            "1.\"address\":     (string, required)\n"
            "2.\"height\":      (numeric, required)\n"
            "\nResult: address related tx hash as array\n"
            "\nExamples:\n" +
            HelpExampleCli("listtxbyaddr", "\"wcoA7yUW4fc4m6a2HSk36t4VVxzKUnvq4S\" \"10000\"") +
            "\nAs json rpc call\n" +
            HelpExampleRpc("listtxbyaddr", "\"wcoA7yUW4fc4m6a2HSk36t4VVxzKUnvq4S\", \"10000\""));
    }

    string address = params[0].get_str();
    int height     = params[1].get_int();
    if (height < 0 || height > chainActive.Height())
        throw runtime_error("Height out of range.");

    CKeyID keyId;
    if (!GetKeyId(address, keyId))
        throw runtime_error("Address invalid.");

    map<string, string> mapTxHash;
    if (!pCdMan->pContractCache->GetTxHashByAddress(keyId, height, mapTxHash))
        throw runtime_error("Failed to fetch data.");

    Object obj;
    Array arrayObj;
    for (auto item : mapTxHash) {
        arrayObj.push_back(item.second);
    }
    obj.push_back(Pair("address", address));
    obj.push_back(Pair("height", height));
    obj.push_back(Pair("tx_array", arrayObj));

    return obj;
}

Value listdelegates(const Array& params, bool fHelp) {
    if (fHelp || params.size() > 1) {
        throw runtime_error(
                "listdelegates \n"
                "\nreturns the specified number delegates by reversed order voting number.\n"
                "\nArguments:\n"
                "1. number           (number, optional) the number of the delegates, default to all delegates.\n"
                "\nResult:\n"
                "\nExamples:\n"
                + HelpExampleCli("listdelegates", "11")
                + HelpExampleRpc("listdelegates", "11"));
    }

    int delegateNum = (params.size() == 1) ? params[0].get_int() : IniCfg().GetTotalDelegateNum();
    if (delegateNum < 1 || delegateNum > 11) {
        throw JSONRPCError(RPC_INVALID_PARAMETER,
                           strprintf("Delegate number not between 1 and %u", IniCfg().GetTotalDelegateNum()));
    }

    vector<CRegID> delegatesList;
    if (!pCdMan->pDelegateCache->GetTopDelegates(delegatesList)) {
        throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to get delegates list");
    }

    delegatesList.resize(std::min(delegateNum, (int)delegatesList.size()));

    Object obj;
    Array delegateArray;

    CAccount account;
    for (const auto& delegate : delegatesList) {
        if (!pCdMan->pAccountCache->GetAccount(delegate, account)) {
            throw JSONRPCError(RPC_INTERNAL_ERROR, "Failed to get account info");
        }
        delegateArray.push_back(account.ToJsonObj());
    }

    obj.push_back(Pair("delegates", delegateArray));

    return obj;
}
