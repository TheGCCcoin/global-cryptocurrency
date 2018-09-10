// Copyright (c) 2018 The GCC Core developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#include "pos.h"

#include "arith_uint256.h"
#include "chain.h"
#include "chainparams.h"
#include "primitives/block.h"
#include "uint256.h"

#define CBigNum uint256

#define int64 int64_t

// c.1.0 dash backup [

//static const CAmount COIN = 100000000;
//static const CAmount CENT = 1000000;

//static const CAmount MAX_MONEY = 21000000 * COIN;

// c.1.0 dash backup ]
// c.1.0.0 remove [

//static const int64 COIN = 1000000;
//static const int64 CENT = 10000;

//static const int64 MAX_MONEY = COIN * 125000000000;  // 124 Billion -> 125 bumped to be safe

//CBigNum bnProofOfStakeLimit(~uint256(0) >> 2);
//CBigNum bnProofOfStakeLimitV2(~uint256(0) >> 8);

//static CScriptNum bnProofOfWorkLimit(~uint256(0) >> 20);

//extern uint32_t bnProofOfWorkLimit;

// c.1.0.0 remove ]
// c.2.1 thegcc old params [


// f.6.1 genesis.nBits = bnProofOfWorkLimit(~uint256(0) >> 20) = 0x1e0fffff - todo: remove after review [

// arith_uint256 bnProofOfWorkLimit = ((arith_uint256() - 1) >> 20);

//static uint256 bnProofOfWorkLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
//static CScriptNum bnProofOfWorkLimit(~uint256(0) >> 20);
//static uint32_t bnProofOfWorkLimit = 0x1e0fffff;

// f.6.1 genesis.nBits = bnProofOfWorkLimit(~uint256(0) >> 20) = 0x1e0fffff - todo: remove after review ]
// f.6.x1 - todo: remove after review [

//uint256 bnProofOfStakeLimit = uint256S("3fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
//uint256 bnProofOfStakeLimitV2 = uint256S("00ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

// f.6.x1 - todo: remove after review ]

int64_t nReserveBalance = 0;

unsigned int nStakeMinAge = 60 * 60 * 24 * 3;	//minimum age for coin age:  15 day
unsigned int nStakeMaxAge = 60 * 60 * 24 * 10;	//stake age of full weight:  30 day
unsigned int nStakeTargetSpacing = 60;			// 60  sec block spacing

unsigned int nStakeMinAgeV2 = 60 * 24;	//minimum age for coin age:  15 day
unsigned int nStakeMaxAgeV2 = -1;	//stake age of full weight:  unlimited

int64 nChainStartTime =1488585034 ;

static const unsigned int MAX_BLOCK_SIZE = 1000000;
static const unsigned int MAX_BLOCK_SIZE_GEN = MAX_BLOCK_SIZE/2;
static const unsigned int MAX_BLOCK_SIGOPS = MAX_BLOCK_SIZE/50;
static const unsigned int MAX_ORPHAN_TRANSACTIONS = MAX_BLOCK_SIZE/100;
static const unsigned int MAX_INV_SZ = 50000;
static const int64 MIN_TX_FEE = 0.0001 * CENT;
static const int64 MIN_RELAY_TX_FEE = 0.0001 * CENT;
// MAX_MONEY is for consistency checking
// The actual PoW coins is about 23,300,000
// This number will go up over time.
// Code Reviewers: Don't look here for the money supply limit!!!!!!
// Go look at GetProofOfWorkReward and do the math yourself
// Don't quote the value of MAX_MONEY as an indicator
// of the true money supply or you will look foolish!!!!!!
static const int64 CIRCULATION_MONEY = MAX_MONEY;
static const double TAX_PERCENTAGE = 0.00; //no tax
// 20% annual interest
static const int64 MAX_STEALTH_PROOF_OF_STAKE = 0.012 * COIN;
static const int64 MAX_STEALTH_PROOF_OF_STAKE_V2 = 20 * CENT; // 20%
static const int64 MAX_STEALTH_PROOF_OF_STAKE_V3 = 18 * CENT; // 18%
static const int64 MAX_STEALTH_PROOF_OF_STAKE_V4 = 16 * CENT; // 16%
static const int64 MAX_STEALTH_PROOF_OF_STAKE_V5 = 14 * CENT; // 14%
static const int64 MAX_STEALTH_PROOF_OF_STAKE_V6 = 12 * CENT; // 12%
static const int64 MAX_STEALTH_PROOF_OF_STAKE_V7 = 2 * CENT; // 2%

// too many problems with PoW+PoS and big hashes
// static const unsigned int CUTOFF_POW_BLOCK = 20421;
int CUTOFF_POW_BLOCK = 6222800;
unsigned int CUTOFF_POW_TIME = 1905125188;
// Thu Oct  9 00:00:00 2014 MST
static const unsigned int STEALTH_ADDR_KICK_IN = 1912834400;

unsigned int VERSION2_SWITCH_TIME = 1500336002; // GMT: Tuesday, 18 July 2017  20%
static const unsigned int POS_REWARDS_SWITCH_TIME3 = 1523318400; // 10.04.2018 - 18%
static const unsigned int POS_REWARDS_SWITCH_TIME4 = 1554854400; // 10.04.2019 - 16%
static const unsigned int POS_REWARDS_SWITCH_TIME5 = 1586476800; // 10.04-2020 - 14%
static const unsigned int POS_REWARDS_SWITCH_TIME6 = 1618012800; // 10.04-2021 - 12%
static const unsigned int POS_REWARDS_SWITCH_TIME7 = 1649548800; // 10.04.2022 - 2%

static const int64 nTargetTimespan = 60 * 30; // 30 blocks
static const int64 nTargetSpacingWorkMax = 3 * nStakeTargetSpacing;

//COINBASE_MATURITY

// c.2.1 thegcc old params ]

// thegcc [
// FutureDrift [

int64_t FutureDrift(int64_t nTime)
{
    if (nTime > VERSION2_SWITCH_TIME)
        return nTime + 15 * 60;  // up to 15 minutes from the future
    else
        return nTime + 2 * 60 * 60;  // up to 120 minutes from the future
}

// FutureDrift ]
// CheckCoinStakeTimestamp [

// Check whether the coinstake timestamp meets protocol
bool CheckCoinStakeTimestamp(int64_t nTimeBlock, int64_t nTimeTx)
{
    // v0.3 protocol
    return (nTimeBlock == nTimeTx);
}

// CheckCoinStakeTimestamp ]
// thegcc ]

// c.3 thegcc old pos functions [

// pos: GetStakeMinAge [

unsigned int GetStakeMinAge(unsigned int nTime)
{
    if (nTime > VERSION2_SWITCH_TIME)
        return nStakeMinAgeV2;
    else
        return nStakeMinAge;
}

// pos: GetStakeMinAge ]
// pos: GetStakeMaxAge [

unsigned int GetStakeMaxAge(unsigned int nTime)
{
    if (nTime > VERSION2_SWITCH_TIME)
        return nStakeMaxAgeV2;
    else
        return nStakeMaxAge;
}

// pos: GetStakeMaxAge ]
// pos: GetProofOfStakeLimit [

static CBigNum GetProofOfStakeLimit(unsigned int nTime)
{
    // f.6.x2 bnProofOfStakeLimit bnProofOfStakeLimitV2  - todo: remove after review [
    /*
    if (nTime > VERSION2_SWITCH_TIME)
        return bnProofOfStakeLimitV2;
    else
        return bnProofOfStakeLimit;
    */
    // f.6.x2 bnProofOfStakeLimit bnProofOfStakeLimitV2  - todo: remove after review ]

    if (nTime > VERSION2_SWITCH_TIME)
        return Params().GetConsensus().posLimitV2;
    else
        return Params().GetConsensus().posLimit;
}

// pos: GetProofOfStakeLimit ]
// pos: GetLastBlockIndex [

// ppcoin: find last block index up to pindex
const CBlockIndex* GetLastBlockIndex(const CBlockIndex* pindex, bool fProofOfStake)
{
    while (pindex && pindex->pprev && (pindex->IsProofOfStake2() != fProofOfStake))
        pindex = pindex->pprev;
    return pindex;
}

// pos: GetLastBlockIndex ]
// pos: GetNextWorkRequired [
// old.GetNextTargetRequired => new.GetNextWorkRequired

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params, bool fProofOfStake)
{
    arith_uint256 bnTargetLimit = UintToArith256(Params().GetConsensus().powLimit); //bnProofOfWorkLimit;

    if(fProofOfStake)
    {
        // Proof-of-Stake blocks has own target limit since nVersion=3 supermajority on mainNet and always on testNet
        bnTargetLimit = UintToArith256(GetProofOfStakeLimit(pindexLast->nTime));
    }

    if (pindexLast == NULL)
        return  bnTargetLimit.GetCompact(); // genesis block

    const CBlockIndex* pindexPrev = GetLastBlockIndex(pindexLast, fProofOfStake);
    if (pindexPrev->pprev == NULL)
        return bnTargetLimit.GetCompact(); // first block
    const CBlockIndex* pindexPrevPrev = GetLastBlockIndex(pindexPrev->pprev, fProofOfStake);
    if (pindexPrevPrev->pprev == NULL)
        return bnTargetLimit.GetCompact(); // second block

    int64 nActualSpacing = pindexPrev->GetBlockTime() - pindexPrevPrev->GetBlockTime();
    if(nActualSpacing < 0)
    {
        // printf(">> nActualSpacing = %"PRI64d" corrected to 1.\n", nActualSpacing);
        nActualSpacing = 1;
    }
    else if(nActualSpacing > nTargetTimespan)
    {
        // printf(">> nActualSpacing = %"PRI64d" corrected to nTargetTimespan (900).\n", nActualSpacing);
        nActualSpacing = nTargetTimespan;
    }

    // ppcoin: target change every block
    // ppcoin: retarget with exponential moving toward target spacing
//    CBigNum bnNew;
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexPrev->nBits);

    int64 nTargetSpacing = fProofOfStake? nStakeTargetSpacing : std::min(nTargetSpacingWorkMax, (int64) nStakeTargetSpacing * (1 + pindexLast->nHeight - pindexPrev->nHeight));
    int64 nInterval = nTargetTimespan / nTargetSpacing;
    bnNew *= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
    bnNew /= ((nInterval + 1) * nTargetSpacing);

    /*
    printf(">> Height = %d, fProofOfStake = %d, nInterval = %"PRI64d", nTargetSpacing = %"PRI64d", nActualSpacing = %"PRI64d"\n",
        pindexPrev->nHeight, fProofOfStake, nInterval, nTargetSpacing, nActualSpacing);
    printf(">> pindexPrev->GetBlockTime() = %"PRI64d", pindexPrev->nHeight = %d, pindexPrevPrev->GetBlockTime() = %"PRI64d", pindexPrevPrev->nHeight = %d\n",
        pindexPrev->GetBlockTime(), pindexPrev->nHeight, pindexPrevPrev->GetBlockTime(), pindexPrevPrev->nHeight);
    */

    if (bnNew > bnTargetLimit)
        bnNew = bnTargetLimit;

    return bnNew.GetCompact();
}

// pos: GetNextWorkRequired ]
// c.3 thegcc old pos functions ]

// b.1 blk pos [

#if 0
// GetNextTargetRequired [
unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, bool fProofOfStake, const Consensus::Params& params)
{
    unsigned int nTargetLimit = UintToArith256(params.powLimit).GetCompact();

    // Genesis block
    if (pindexLast == NULL)
        return nTargetLimit;

    const CBlockIndex* pindexPrev = GetLastBlockIndex(pindexLast, fProofOfStake);
    if (pindexPrev->pprev == NULL)
        return nTargetLimit; // first block
    const CBlockIndex* pindexPrevPrev = GetLastBlockIndex(pindexPrev->pprev, fProofOfStake);
    if (pindexPrevPrev->pprev == NULL)
        return nTargetLimit; // second block

    return CalculateNextTargetRequired(pindexPrev, pindexPrevPrev->GetBlockTime(), params);
}

// GetNextTargetRequired ]
// CalculateNextTargetRequired [

unsigned int CalculateNextTargetRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    int64_t nActualSpacing = pindexLast->GetBlockTime() - nFirstBlockTime;
    int64_t nTargetSpacing = params.IsProtocolV2(pindexLast->GetBlockTime()) ? params.nTargetSpacing : params.nTargetSpacingV1;

    // Limit adjustment step
    if (pindexLast->GetBlockTime() > params.nProtocolV1RetargetingFixedTime && nActualSpacing < 0)
        nActualSpacing = nTargetSpacing;
    if (pindexLast->GetBlockTime() > params.nProtocolV3Time && nActualSpacing > nTargetSpacing*10)
        nActualSpacing = nTargetSpacing*10;

    // retarget with exponential moving toward target spacing
    const arith_uint256 bnTargetLimit = GetTargetLimit(pindexLast->GetBlockTime(), pindexLast->IsProofOfStake(), params);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    int64_t nInterval = params.nTargetTimespan / nTargetSpacing;
    bnNew *= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
    bnNew /= ((nInterval + 1) * nTargetSpacing);

    if (bnNew <= 0 || bnNew > bnTargetLimit)
        bnNew = bnTargetLimit;

    return bnNew.GetCompact();
}
// CalculateNextTargetRequired ]
#endif

// b.1 blk pos ]

