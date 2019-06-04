// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/transaction.h"
#include "platform/specialtx.h"
#include "platform/platform-utils.h"
#include "platform/nf-token/nf-token-reg-tx-builder.h"
#include "platform/nf-token/nf-tokens-manager.h"
#include "specialtx-rpc-utils.h"
#include "rpc-nf-token.h"

json_spirit::Value nftoken(const json_spirit::Array& params, bool fHelp)
{
    std::string command = Platform::GetCommand(params, "usage: nftoken register|list|get|getbytxid|totalsupply");

    if (command == "register")
        return Platform::RegisterNfToken(params, fHelp);
    else if (command == "list")
        return Platform::ListNfTokenTxs(params, fHelp);
    else if (command == "get")
        return Platform::GetNfToken(params, fHelp);
    else if (command == "getbytxid")
        return Platform::GetNfTokenByTxId(params,fHelp);
    else if (command == "totalsupply")
        return Platform::NfTokenTotalSupply(params, fHelp);

    throw std::runtime_error("Invalid command: " + command);
}

namespace Platform
{
    void RegisterNfTokenHelp()
    {
        static std::string helpMessage =
                "nftoken register \"nfTokenProtocol\" \"tokenId\" \"tokenOwnerAddr\" \"tokenMetadataAdminAddr\" \"metadata\"\n"
                "Creates and sends a new non-fungible token transaction.\n"
                "\nArguments:\n"
                "1. \"nfTokenProtocol\"           (string, required) A non-fungible token protocol symbol to use in the token creation.\n"
                "                                 The protocol name must be valid and registered previously.\n"

                "2. \"nfTokenId\"                 (string, required) The token id in hex.\n"
                "                                 The token id must be unique in the token type space.\n"

                "3. \"nfTokenOwnerAddr\"          (string, required) The token owner key, can be used in any operations with the token.\n"
                "                                 The private key belonging to this address may be or may be not known in your wallet.\n"

                "4. \"nfTokenMetadataAdminAddr\"  (string, optional, default = \"0\") The metadata token administration key, can be used to modify token metadata.\n"
                "                                 The private key does not have to be known by your wallet. Can be set to 0.\n"

                "5. \"nfTokenMetadata\"           (string, optional) metadata describing the token.\n"
                "                                 It may contain text or binary in hex/base64 //TODO .\n";//TODO: add examples

        throw std::runtime_error(helpMessage);
    }

    json_spirit::Value RegisterNfToken(const json_spirit::Array& params, bool fHelp)
    {
        if (fHelp || params.size() < 4 || params.size() > 6)
            RegisterNfTokenHelp();

        CKey ownerPrivKey;
        NfTokenRegTxBuilder nfTokenRegTxBuilder;
        nfTokenRegTxBuilder.SetTokenProtocol(params[1]).SetTokenId(params[2]).SetTokenOwnerKey(params[3], ownerPrivKey);

        if (params.size() > 4)
            nfTokenRegTxBuilder.SetMetadataAdminKey(params[4]);
        if (params.size() > 5)
            nfTokenRegTxBuilder.SetMetadata(params[5]);

        NfTokenRegTx nfTokenRegTx = nfTokenRegTxBuilder.BuildTx();

        CMutableTransaction tx;
        tx.nVersion = 3;
        tx.nType = TRANSACTION_NF_TOKEN_REGISTER;

        FundSpecialTx(tx, nfTokenRegTx);
        SignSpecialTxPayload(tx, nfTokenRegTx, ownerPrivKey);
        SetTxPayload(tx, nfTokenRegTx);

        std::string result = SignAndSendSpecialTx(tx);
        return result;
    }

    void ListNfTokenTxsHelp()
    {
        static std::string helpMessage =
                R"(nftoken list
Lists all nftoken records on chain

Arguments:
1. "nfTokenProtocol"  (string, optional) The non-fungible token protocol symbol of the registered token record
                      The protocol name must be valid and registered previously. If "*" is set, it will list NFTs for all protocols.
2. "nfTokenOwnerAddr" (string, optional) The token owner address, it can be used in any operations with the token.
                      The private key belonging to this address may be or may be not known in your wallet. If "*" is set, it will list NFTs for all addresses.
3. height             (numeric, optional) If height is not specified, it defaults to the current chain-tip
4. count              (numeric, optional, default=20) The number of transactions to return
5. from               (numeric, optional, default=0) The number of transactions to skip
6. regTxOnly          (boolean, optional, default=false) false for a detailed list, true for an array of transaction IDs

Examples:
List the most recent 20 NFT records
)"
+ HelpExampleCli("nftoken", R"(list)")
+ R"(List the most recent 20 records of the "doc" NFT protocol
)"
+ HelpExampleCli("nftoken", R"(list "doc")")
+ HelpExampleCli("nftoken", R"(list "doc" "*")")
+ R"(List the most recent 20 records of the "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" address
)"
+ HelpExampleCli("nftoken", R"(list "*" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc")")
+ R"(List the most recent 20 records of the "doc" NFT protocol and "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" address
)"
+ HelpExampleCli("nftoken", R"(list "doc" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc")")
 + R"(List the most recent 20 records of the "doc" NFT protocol and "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" address up to 5050st block
)"
+ HelpExampleCli("nftoken", R"(list "doc" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 5050)")
+ R"(List records 100 to 150 of the "doc" NFT protocol and "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" address up to 5050st block
)"
+ HelpExampleCli("nftoken", R"(list "doc" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 5050 100 50)")
+ R"(List records 100 to 150 of the "doc" NFT protocol and "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" address up to 5050st block. List only registration tx IDs.
)"
+ HelpExampleCli("nftoken", R"(list "doc" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 5050 100 50 true)")
+ R"(As JSON-RPC calls
)"
+ HelpExampleRpc("nftoken", R"(list "*" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc")")
+ HelpExampleRpc("nftoken", R"(list "doc" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 5050)")
+ HelpExampleCli("nftoken", R"(list "doc" "CRWS78Yf5kbWAyfcES6RfiTVzP87csPNhZzc" 5050 100 50)");

        throw std::runtime_error(helpMessage);
    }

    json_spirit::Object BuildNftRecord(const NfTokenIndex & nftIndex)
    {
        json_spirit::Object nftJsonObj;

        nftJsonObj.push_back(json_spirit::Pair("blockHash", nftIndex.BlockIndex()->phashBlock->ToString()));
        nftJsonObj.push_back(json_spirit::Pair("registrationTxHash", nftIndex.RegTxHash().ToString()));
        nftJsonObj.push_back(json_spirit::Pair("height", nftIndex.BlockIndex()->nHeight));
        //auto blockTime = static_cast<time_t>(nftIndex.BlockIndex()->nTime);
        //std::string timeStr(asctime(gmtime(&blockTime)));
        //nftJsonObj.push_back(json_spirit::Pair("timestamp", timeStr));
        nftJsonObj.push_back(json_spirit::Pair("timestamp", nftIndex.BlockIndex()->GetBlockTime()));

        nftJsonObj.push_back(json_spirit::Pair("nftProtocolId", ProtocolName{nftIndex.NfTokenPtr()->tokenProtocolId}.ToString()));
        nftJsonObj.push_back(json_spirit::Pair("nftId", nftIndex.NfTokenPtr()->tokenId.ToString()));
        nftJsonObj.push_back(json_spirit::Pair("nftOwnerKeyId", CBitcoinAddress(nftIndex.NfTokenPtr()->tokenOwnerKeyId).ToString()));
        nftJsonObj.push_back(json_spirit::Pair("metadataAdminKeyId", CBitcoinAddress(nftIndex.NfTokenPtr()->metadataAdminKeyId).ToString()));

        //TODO: mimetype text/plan only for now
        std::string textMeta(nftIndex.NfTokenPtr()->metadata.begin(), nftIndex.NfTokenPtr()->metadata.end());
        nftJsonObj.push_back(json_spirit::Pair("metadata", textMeta));

        return nftJsonObj;
    }

    json_spirit::Value ListNfTokenTxs(const json_spirit::Array& params, bool fHelp)
    {
        if (fHelp || params.empty() || params.size() > 7)
            ListNfTokenTxsHelp();

        uint64_t nftProtoId = NfToken::UNKNOWN_TOKEN_PROTOCOL;
        if (params.size() > 1)
        {
            std::string nftProtoIdStr = params[1].get_str();
            if (nftProtoIdStr != "*")
                nftProtoId = StringToProtocolName(nftProtoIdStr.c_str());
        }

        CKeyID filterKeyId;
        if (params.size() > 2 && params[2].get_str() != "*")
        {
            filterKeyId = ParsePubKeyIDFromAddress(params[2].get_str(), "nfTokenOwnerAddr");
        }

        int height = (params.size() > 3) ? ParseInt32V(params[3], "height") : chainActive.Height();
        if (height < 0 || height > chainActive.Height())
            throw JSONRPCError(RPC_INVALID_PARAMETER, "Block height is out of range");

        static const int defaultTxsCount = 20;
        static const int defaultStartFrom = 20;
        int count = (params.size() > 4) ? ParseInt32V(params[4], "count") : defaultTxsCount;
        int startFrom = (params.size() > 5) ? ParseInt32V(params[5], "from") : defaultStartFrom;
        bool regTxOnly = (params.size() > 6) ? ParseBoolV(params[6], "regTxOnly") : false;

        json_spirit::Array nftList;

        auto nftIndexHandler = [&](const NfTokenIndex & nftIndex) -> bool
        {
            if (regTxOnly)
            {
                json_spirit::Object hashObj;
                hashObj.push_back(json_spirit::Pair("registrationTxHash", nftIndex.RegTxHash().GetHex()));
                nftList.push_back(hashObj);
            }
            else
            {
                nftList.push_back(BuildNftRecord(nftIndex));
            }
            return true;
        };

        if (nftProtoId == NfToken::UNKNOWN_TOKEN_PROTOCOL && filterKeyId.IsNull())
            NfTokensManager::Instance().ProcessNftIndexRangeByHeight(nftIndexHandler, height, count, startFrom);
        else if (nftProtoId != NfToken::UNKNOWN_TOKEN_PROTOCOL && filterKeyId.IsNull())
            NfTokensManager::Instance().ProcessNftIndexRangeByHeight(nftIndexHandler, nftProtoId, height, count, startFrom);
        else if (nftProtoId == NfToken::UNKNOWN_TOKEN_PROTOCOL && !filterKeyId.IsNull())
            NfTokensManager::Instance().ProcessNftIndexRangeByHeight(nftIndexHandler, filterKeyId, height, count);
        else if (nftProtoId != NfToken::UNKNOWN_TOKEN_PROTOCOL && !filterKeyId.IsNull())
            NfTokensManager::Instance().ProcessNftIndexRangeByHeight(nftIndexHandler, nftProtoId, filterKeyId, height, count);

        return nftList;
    }

    void GetNfTokenHelp()
    {
        static std::string helpMessage =
                R"(nftoken get
Get an nftoken record by an NFT protocol ID and token ID
Arguments:

1. "nfTokenProtocol"  (string, required) The non-fungible token protocol symbol of the registered token record
                      The protocol name must be valid and registered previously.
2. "nfTokenId"        (string, required) The token id in hex.
                      The token id must be registered previously.

Examples:
)"
 + HelpExampleCli("nftoken", R"(get "doc" "a103d4bdfaa7d22591c4dacda81ba540e37f705bae41681c082b102e647aa8e8")")
 + HelpExampleRpc("nftoken", R"(get "doc" "a103d4bdfaa7d22591c4dacda81ba540e37f705bae41681c082b102e647aa8e8")");

        throw std::runtime_error(helpMessage);
    }

    json_spirit::Value GetNfToken(const json_spirit::Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 3)
            GetNfTokenHelp();

        uint64_t tokenProtocolId = StringToProtocolName(params[1].get_str().c_str());
        uint256 tokenId = ParseHashV(params[2].get_str(), "nfTokenId");

        auto nftIndex = NfTokensManager::Instance().GetNfTokenIndex(tokenProtocolId, tokenId);
        if (nftIndex.IsNull())
            throw std::runtime_error("Can't find an NFT record: " + std::to_string(tokenProtocolId) + " " + tokenId.ToString());

        return BuildNftRecord(nftIndex);
    }

    void GetNfTokenByTxIdHelp()
    {
        static std::string helpMessage =
                R"(nftoken getbytxid
Get an nftoken record by a transaction ID
Arguments:

1. "registrationTxId"   (string, required) The NFT registration transaction ID

Examples:
)"
 + HelpExampleCli("nftoken", R"(get "3840804e62350b6337ca0b4653547477aa46dab2677c0514a8dccf80b51a899a")")
 + HelpExampleRpc("nftoken", R"(get "3840804e62350b6337ca0b4653547477aa46dab2677c0514a8dccf80b51a899a")");

        throw std::runtime_error(helpMessage);
    }

    json_spirit::Value GetNfTokenByTxId(const json_spirit::Array& params, bool fHelp)
    {
        if (fHelp || params.size() != 2)
            GetNfTokenByTxIdHelp();

        uint256 regTxHash = ParseHashV(params[1].get_str(), "registrationTxHash");
        auto nftIndex = NfTokensManager::Instance().GetNfTokenIndex(regTxHash);
        if (nftIndex.IsNull())
            throw std::runtime_error("Can't find an NFT record by tx id: " + regTxHash.ToString());
        return BuildNftRecord(nftIndex);
    }

    void NfTokenTotalSupplyHelp()
    {
        static std::string helpMessage = R"(nftoken totalsupply
Get NFTs current total supply

nArguments:
1. "nfTokenProtocol"  (string, optional) The non-fungible token protocol symbol
                      The protocol name must be valid and registered previously.

Examples:
)"
+ HelpExampleCli("nftoken",   "totalsupply")
+ HelpExampleCli("nftoken", R"(totalsupply "doc")")
+ HelpExampleRpc("nftoken", R"(totalsupply "doc")");

        throw std::runtime_error(helpMessage);
    }

    json_spirit::Value NfTokenTotalSupply(const json_spirit::Array& params, bool fHelp)
    {
        if (fHelp)
            NfTokenTotalSupplyHelp();

        std::size_t totalSupply = 0;
        if (params.size() == 2)
        {
            uint64_t tokenProtocolId = StringToProtocolName(params[1].get_str().c_str());
            totalSupply = NfTokensManager::Instance().TotalSupply(tokenProtocolId);
        }
        else
        {
            totalSupply = NfTokensManager::Instance().TotalSupply();
        }

        return totalSupply;
    }
}
