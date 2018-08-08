// Copyright (c) 2018 The GCC Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef THEGCC_POS_H
#define THEGCC_POS_H

// thegcc: pos common header [

// g.3 pos work [

#include "consensus/params.h"

class CBlockIndex;
class CBlockHeader;

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params, bool fProofOfStake);

unsigned int GetStakeMinAge(unsigned int nTime);

// g.3 pos work ]
// g.2 pos miner [

class CBlock;
class CReserveKey;
class CScript;
class CWallet;

struct CBlockTemplate;

/** Run the miner threads */
void GenerateBitcoins(bool fGenerate, CWallet* pwallet, int nThreads);
/** Generate a new block, without valid proof-of-work */
CBlockTemplate* CreateNewBlock(const CScript& scriptPubKeyIn, CWallet* pwallet, bool fProofOfStake);
CBlockTemplate* CreateNewBlockWithKey(CReserveKey& reservekey, CWallet* pwallet, bool fProofOfStake);
/** Modify the extranonce in a block */
void IncrementExtraNonce(CBlock* pblock, CBlockIndex* pindexPrev, unsigned int& nExtraNonce);
/** Check mined block */
//void UpdateTime(CBlockHeader* block, const Consensus::Params& consensusParams, const CBlockIndex* pindexPrev);

void BitcoinMiner(CWallet* pwallet, bool fProofOfStake);

extern double dHashesPerSec;
extern int64_t nHPSTimerStart;

// g.2 pos miner ]

// thegcc: pos common header ]

#endif //THEGCC_POS_H
