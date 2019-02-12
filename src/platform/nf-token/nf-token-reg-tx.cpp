// Copyright (c) 2014-2018 Crown Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "sync.h"
#include "main.h"

#include "primitives/transaction.h"
#include "platform/platform-utils.h"
#include "platform/specialtx.h"
#include "nf-tokens-manager.h"
#include "nf-token-reg-tx.h"

namespace Platform
{

    template <typename NfTokenTx>
    static bool CheckNfTokenTxSignature(const CTransaction &tx, const NfTokenTx& nfTokenTx, const CKeyID& keyId, CValidationState& state)
    {
        //uint256 inputsHash = CalcTxInputsHash(tx);
        //if (inputsHash != proTx.inputsHash)
        //    return state.DoS(100, false, REJECT_INVALID, "bad-protx-inputs-hash");

        std::string strError;
        if (!CHashSigner::VerifyHash(::SerializeHash(nfTokenTx), keyId, nfTokenTx.signature, strError))
            return state.DoS(100, false, REJECT_INVALID, "bad-token-reg-tx-sig", false, strError);

        return true;
    }

    bool NfTokenRegTx::CheckTx(const CTransaction& tx, const CBlockIndex* pindexLast, CValidationState& state)
    {
        AssertLockHeld(cs_main);

        NfTokenRegTx nfTokenRegTx;
        if (!GetTxPayload(tx, nfTokenRegTx))
            return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

        const NfToken & nfToken = nfTokenRegTx.GetNfToken();

        if (nfTokenRegTx.m_version != NfTokenRegTx::CURRENT_VERSION)
            return state.DoS(100, false, REJECT_INVALID, "bad-token-reg-tx-version");

        if (nfToken.tokenId.IsNull())
            return state.DoS(10, false, REJECT_INVALID, "bad-token-reg-tx-token");

        if (nfToken.tokenOwnerKeyId.IsNull())
            return state.DoS(10, false, REJECT_INVALID, "bad-token-reg-tx-owner-key-null");

        if (nfToken.metadataAdminKeyId.IsNull())
            return state.DoS(10, false, REJECT_INVALID, "bad-token-reg-tx-metadata-admin-key-null");

        //TODO: Validate metadata

        if (pindexLast != nullptr)
        {
            if (NfTokensManager::Instance().Contains(nfToken.tokenProtocolId, nfToken.tokenId, pindexLast->nHeight))
                return state.DoS(10, false, REJECT_DUPLICATE, "bad-token-reg-tx-dup-token");
        }

        return CheckNfTokenTxSignature(tx, nfTokenRegTx, nfTokenRegTx.GetNfToken().tokenOwnerKeyId, state);
    }

    bool NfTokenRegTx::ProcessTx(const CTransaction &tx, const CBlockIndex *pindex, CValidationState &state)
    {
        NfTokenRegTx nfTokenRegTx;
        bool result = GetTxPayload(tx, nfTokenRegTx);
        // should have been checked already
        assert(result);

        //TODO: remove after extensive testing of release version
        if (!result)
            return state.DoS(100, false, REJECT_INVALID, "bad-tx-payload");

        auto nfToken = nfTokenRegTx.GetNfToken();

        if (!NfTokensManager::Instance().AddNfToken(nfToken, tx, pindex))
            return state.DoS(100, false, REJECT_DUPLICATE/*TODO: REJECT_CONFLICT*/, "token-reg-tx-conflit");
        return true;
    }

    bool NfTokenRegTx::UndoTx(const CTransaction& tx, const CBlockIndex * pindex)
    {
        NfTokenRegTx nfTokenRegTx;
        bool result = GetTxPayload(tx, nfTokenRegTx);
        // should have been checked already
        assert(result);

        //TODO: remove after extensive testing of release version
        if (!result)
            return false;

        auto nfToken = nfTokenRegTx.GetNfToken();
        return NfTokensManager::Instance().Delete(nfToken.tokenProtocolId, nfToken.tokenId, pindex->nHeight);
    }

    std::string NfTokenRegTx::ToString() const
    {
        std::ostringstream out;
        out << "NfTokenRegTx(nft protocol ID=" << ProtocolName{m_nfToken.tokenProtocolId}.ToString()
            << ", nft ID=" << m_nfToken.tokenId.ToString()
            << ", nft owner address=" << CBitcoinAddress(m_nfToken.tokenOwnerKeyId).ToString()
            << ", metadata admin address=" << CBitcoinAddress(m_nfToken.metadataAdminKeyId).ToString() << ")";
            //TODO: << ", metadata" << m_nfToken.metadata << ")";
        return out.str();
    }
}
