// Copyright (c) 2017-2018 The PIVX developers
// Copyright (c) 2018 TheGCCcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// @ [

#include "thegcc/stakeinput.h"

#include "chain.h"
#include "validation.h"
#include "wallet/wallet.h"


// @ ]
// vars [

using namespace std;

typedef std::vector<unsigned char> valtype;

extern BlockMap mapBlockIndex;

// vars ]
// k.1 kernel [

bool fTestNet = false; //Params().NetworkID() == CBaseChainParams::TESTNET;

static const unsigned int MODIFIER_INTERVAL = 60;
static const unsigned int MODIFIER_INTERVAL_TESTNET = 60;
static const int MODIFIER_INTERVAL_RATIO = 3;

unsigned int getIntervalVersion(bool fTestNet);
static int64_t GetStakeModifierSelectionIntervalSection(int nSection);
bool GetKernelStakeModifier(uint256 hashBlockFrom, uint64_t& nStakeModifier, int& nStakeModifierHeight, int64_t& nStakeModifierTime, bool fPrintProofOfStake);

unsigned int getIntervalVersion(bool fTestNet)
{
    if (fTestNet)
        return MODIFIER_INTERVAL_TESTNET;
    else
        return MODIFIER_INTERVAL;
}


// Get selection interval section (in seconds)
static int64_t GetStakeModifierSelectionIntervalSection(int nSection)
{
    assert(nSection >= 0 && nSection < 64);
    int64_t a = getIntervalVersion(fTestNet) * 63 / (63 + ((63 - nSection) * (MODIFIER_INTERVAL_RATIO - 1)));
    return a;
}

// Get stake modifier selection interval (in seconds)
static int64_t GetStakeModifierSelectionInterval()
{
    int64_t nSelectionInterval = 0;
    for (int nSection = 0; nSection < 64; nSection++) {
        nSelectionInterval += GetStakeModifierSelectionIntervalSection(nSection);
    }
    return nSelectionInterval;
}



// The stake modifier used to hash for a stake kernel is chosen as the stake
// modifier about a selection interval later than the coin generating the kernel
bool GetKernelStakeModifier(uint256 hashBlockFrom, uint64_t& nStakeModifier, int& nStakeModifierHeight, int64_t& nStakeModifierTime, bool fPrintProofOfStake)
{
    nStakeModifier = 0;
    if (!mapBlockIndex.count(hashBlockFrom))
        return error("GetKernelStakeModifier() : block not indexed");
    const CBlockIndex* pindexFrom = mapBlockIndex[hashBlockFrom];
    nStakeModifierHeight = pindexFrom->nHeight;
    nStakeModifierTime = pindexFrom->GetBlockTime();
    int64_t nStakeModifierSelectionInterval = GetStakeModifierSelectionInterval();
    const CBlockIndex* pindex = pindexFrom;
    CBlockIndex* pindexNext = chainActive[pindexFrom->nHeight + 1];

    // loop to find the stake modifier later by a selection interval
    while (nStakeModifierTime < pindexFrom->GetBlockTime() + nStakeModifierSelectionInterval) {
        if (!pindexNext) {
            // Should never happen
            return error("Null pindexNext\n");
        }

        pindex = pindexNext;
        pindexNext = chainActive[pindexNext->nHeight + 1];
        if (pindex->GeneratedStakeModifier()) {
            nStakeModifierHeight = pindex->nHeight;
            nStakeModifierTime = pindex->GetBlockTime();
        }
    }
    nStakeModifier = pindex->nStakeModifier;
    return true;
}

// k.1 kernel ]
// CTheGCCStake [

bool CTheGCCStake::SetInput(CTransaction txPrev, unsigned int n)
{
    this->txFrom = CMutableTransaction(txPrev);
    this->nPosition = n;
    return true;
}

bool CTheGCCStake::GetTxFrom(CTransactionRef& tx)
{
    tx = std::make_shared<const CTransaction>(CTransaction(txFrom));
    return true;
}

bool CTheGCCStake::CreateTxIn(CWallet* pwallet, CTxIn& txIn, uint256 hashTxOut)
{
    txIn = CTxIn(txFrom.GetHash(), nPosition);
    return true;
}

CAmount CTheGCCStake::GetValue()
{
    return txFrom.vout[nPosition].nValue;
}

// q.1 nStakeSplitThreshold [

int64_t nStakeSplitThreshold = 2000;

// q.1 nStakeSplitThreshold ]

bool CTheGCCStake::CreateTxOuts(CWallet* pwallet, vector<CTxOut>& vout, CAmount nTotal)
{
    vector<valtype> vSolutions;
    txnouttype whichType;
    CScript scriptPubKeyKernel = txFrom.vout[nPosition].scriptPubKey;
    if (!Solver(scriptPubKeyKernel, whichType, vSolutions)) {
        LogPrintf("CreateCoinStake : failed to parse kernel\n");
        return false;
    }

    if (whichType != TX_PUBKEY && whichType != TX_PUBKEYHASH)
        return false; // only support pay to public key and pay to address

    CScript scriptPubKey;
    if (whichType == TX_PUBKEYHASH) // pay to address type
    {
        //convert to pay to public key type
        CKey key;
        if (!pwallet->GetKey(uint160(vSolutions[0]), key))
            return false;

        CPubKey pubkey = key.GetPubKey();

        std::vector<unsigned char> vchPubKey(pubkey.begin(), pubkey.end());

        scriptPubKey << vchPubKey << OP_CHECKSIG;
//        scriptPubKey << key.GetPubKey() << OP_CHECKSIG;
    } else
        scriptPubKey = scriptPubKeyKernel;

    vout.emplace_back(CTxOut(0, scriptPubKey));

    // Calculate if we need to split the output
//    if (nTotal / 2 > (CAmount)(pwallet->nStakeSplitThreshold * COIN))
    if (nTotal / 2 > (CAmount)(nStakeSplitThreshold * COIN))
        vout.emplace_back(CTxOut(0, scriptPubKey));

    return true;
}

bool CTheGCCStake::GetModifier(uint64_t& nStakeModifier)
{
    int nStakeModifierHeight = 0;
    int64_t nStakeModifierTime = 0;
    GetIndexFrom();
    if (!pindexFrom)
        return error("%s: failed to get index from", __func__);

    if (!GetKernelStakeModifier(pindexFrom->GetBlockHash(), nStakeModifier, nStakeModifierHeight, nStakeModifierTime, false))
        return error("CheckStakeKernelHash(): failed to get kernel stake modifier \n");

    return true;
}

CDataStream CTheGCCStake::GetUniqueness()
{
    //The unique identifier for a PIV stake is the outpoint
    CDataStream ss(SER_NETWORK, 0);
    ss << nPosition << txFrom.GetHash();
    return ss;
}

//The block that the UTXO was added to the chain
CBlockIndex* CTheGCCStake::GetIndexFrom()
{
    uint256 hashBlock;
    CTransactionRef tx;
    if (GetTransaction(txFrom.GetHash(), tx, Params().GetConsensus(), hashBlock, true)) {
        // If the index is in the chain, then set it as the "index from"
        if (mapBlockIndex.count(hashBlock)) {
            CBlockIndex* pindex = mapBlockIndex.at(hashBlock);
            if (chainActive.Contains(pindex))
                pindexFrom = pindex;
        }
    } else {
        LogPrintf("%s : failed to find tx %s\n", __func__, txFrom.GetHash().GetHex());
    }

    return pindexFrom;
}

// CTheGCCStake ]
