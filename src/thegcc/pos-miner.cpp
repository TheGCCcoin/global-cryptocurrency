// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2018 The TheGCCcoin developers
// Copyright (c) 2018 The GCC Coin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// @ [
/*
#include "miner.h"

#include "amount.h"
#include "hash.h"
#include "masternode-sync.h"
#include "net.h"
#include "pow.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "timedata.h"
#include "util.h"
#include "utilmoneystr.h"

#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

#include "validationinterface.h"
#include "masternode-payments.h"
#include "spork.h"

#include "consensus/validation.h"

#include "arith_uint256.h"

#include "thegcc/blocksignature.h"

#include "policy/policy.h"

#include "masternode-payments.h"

#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>

#include "undo.h"

#include "thegcc/stakeinput.h"

#include "init.h"

#include "script/sign.h"
*/
#include "thegcc/imports.h"

using namespace std;

// @ ]
// vars [

#define CBigNum uint256

//unsigned int nStakeMinAge = 60 * 60;
extern unsigned int nStakeMinAge;

int nMaturity = 100;


/** The maximum allowed size for a serialized block, in bytes (network rule) */
static const unsigned int MAX_BLOCK_SIZE_CURRENT = 2000000;
static const unsigned int MAX_BLOCK_SIZE_LEGACY = 1000000;

/** Default for -blockmaxsize and -blockminsize, which control the range of sizes the mining code will create **/
//static const unsigned int DEFAULT_BLOCK_MAX_SIZE = 750000;
static const unsigned int DEFAULT_BLOCK_MIN_SIZE = 0;

/** The maximum allowed number of signature check operations in a block (network rule) */
static const unsigned int MAX_BLOCK_SIGOPS_CURRENT = MAX_BLOCK_SIZE_CURRENT / 50;
static const unsigned int MAX_BLOCK_SIGOPS_LEGACY = MAX_BLOCK_SIZE_LEGACY / 50;

#if 0
/** Default for -blockprioritysize, maximum space for zero/low-fee transactions **/
static const unsigned int DEFAULT_BLOCK_PRIORITY_SIZE = 50000;
/** Default for accepting alerts from the P2P network. */
static const bool DEFAULT_ALERTS = true;
/** The maximum size for transactions we're willing to relay/mine */
static const unsigned int MAX_STANDARD_TX_SIZE = 100000;
static const unsigned int MAX_ZEROCOIN_TX_SIZE = 150000;
/** Maximum number of signature check operations in an IsStandard() P2SH script */
static const unsigned int MAX_P2SH_SIGOPS = 15;
/** The maximum number of sigops we're willing to relay/mine in a single tx */
static const unsigned int MAX_TX_SIGOPS_CURRENT = MAX_BLOCK_SIGOPS_CURRENT / 5;
static const unsigned int MAX_TX_SIGOPS_LEGACY = MAX_BLOCK_SIGOPS_LEGACY / 5;
/** Default for -maxorphantx, maximum number of orphan transactions kept in memory */
static const unsigned int DEFAULT_MAX_ORPHAN_TRANSACTIONS = 100;
/** The maximum size of a blk?????.dat file (since 0.8) */
static const unsigned int MAX_BLOCKFILE_SIZE = 0x8000000; // 128 MiB
/** The pre-allocation chunk size for blk?????.dat files (since 0.8) */
static const unsigned int BLOCKFILE_CHUNK_SIZE = 0x1000000; // 16 MiB
/** The pre-allocation chunk size for rev?????.dat files (since 0.8) */
static const unsigned int UNDOFILE_CHUNK_SIZE = 0x100000; // 1 MiB

/** Threshold for nLockTime: below this value it is interpreted as block number, otherwise as UNIX timestamp. */
static const unsigned int LOCKTIME_THRESHOLD = 500000000; // Tue Nov  5 00:53:20 1985 UTC
/** Maximum number of script-checking threads allowed */
static const int MAX_SCRIPTCHECK_THREADS = 16;
/** -par default (number of script-checking threads, 0 = auto) */
static const int DEFAULT_SCRIPTCHECK_THREADS = 0;
/** Number of blocks that can be requested at any given time from a single peer. */
static const int MAX_BLOCKS_IN_TRANSIT_PER_PEER = 16;
/** Timeout in seconds during which a peer must stall block download progress before being disconnected. */
static const unsigned int BLOCK_STALLING_TIMEOUT = 2;
/** Number of headers sent in one getheaders result. We rely on the assumption that if a peer sends
 *  less than this number, we reached their tip. Changing this value is a protocol upgrade. */
static const unsigned int MAX_HEADERS_RESULTS = 2000;
/** Size of the "block download window": how far ahead of our current height do we fetch?
 *  Larger windows tolerate larger download speed differences between peer, but increase the potential
 *  degree of disordering of blocks on disk (which make reindexing and in the future perhaps pruning
 *  harder). We'll probably want to make this a per-peer adaptive value at some point. */
static const unsigned int BLOCK_DOWNLOAD_WINDOW = 1024;
/** Time to wait (in seconds) between writing blockchain state to disk. */
static const unsigned int DATABASE_WRITE_INTERVAL = 3600;
/** Maximum length of reject messages. */
static const unsigned int MAX_REJECT_MESSAGE_LENGTH = 111;

/** Enable bloom filter */
static const bool DEFAULT_PEERBLOOMFILTERS = true;

#endif

extern std::vector<CNode*> vNodes;
extern CCriticalSection cs_vNodes;

extern int CUTOFF_POW_BLOCK;
extern unsigned int CUTOFF_POW_TIME;


extern uint64_t nLastBlockTx;
extern uint64_t nLastBlockSize;
//extern int64_t nLastCoinStakeSearchInterval;
//uint64_t nLastBlockTx = 0;
//uint64_t nLastBlockSize = 0;
int64_t nLastCoinStakeSearchInterval = 0;

double dHashesPerSec = 0.0;
int64_t nHPSTimerStart = 0;



// vars ]
// util [

// COrphan [

class COrphan
{
public:
    const CTransaction* ptx;
    set<uint256> setDependsOn;
    CFeeRate feeRate;
    double dPriority;

    COrphan(const CTransaction* ptxIn) : ptx(ptxIn), feeRate(0), dPriority(0)
    {
    }
};

// COrphan ]
// TxPriorityCompare [

// We want to sort transactions by priority and fee rate, so:
typedef boost::tuple<double, CFeeRate, const CTransaction*> TxPriority;
class TxPriorityCompare
{
    bool byFee;

public:
    TxPriorityCompare(bool _byFee) : byFee(_byFee) {}

    bool operator()(const TxPriority& a, const TxPriority& b)
    {
        if (byFee) {
            if (a.get<1>() == b.get<1>())
                return a.get<0>() < b.get<0>();
            return a.get<1>() < b.get<1>();
        } else {
            if (a.get<0>() == b.get<0>())
                return a.get<1>() < b.get<1>();
            return a.get<0>() < b.get<0>();
        }
    }
};

// TxPriorityCompare ]
// SetThreadPriority [

#include <sys/resource.h>

void SetThreadPriority(int nPriority);

void SetThreadPriority(int nPriority)
{
#ifdef WIN32
    SetThreadPriority(GetCurrentThread(), nPriority);
#else // WIN32
#ifdef PRIO_THREAD
    setpriority(PRIO_THREAD, 0, nPriority);
#else  // PRIO_THREAD
    setpriority(PRIO_PROCESS, 0, nPriority);
#endif // PRIO_THREAD
#endif // WIN32
}

// SetThreadPriority ]
// util ]
// m.4.1 SelectStakeCoins "-thegccstake" [

bool CWallet::SelectStakeCoins(std::list<std::unique_ptr<CStakeInput> >& listInputs, CAmount nTargetAmount)
{

    LOCK(cs_main);
    //Add PIV
    vector<COutput> vCoins;
    AvailableCoins(vCoins, true, NULL, false, STAKABLE_COINS);
    CAmount nAmountSelected = 0;
    if (GetBoolArg("-thegccstake", true)) {
        for (const COutput &out : vCoins) {
            //make sure not to outrun target amount
            if (nAmountSelected + out.tx->tx->vout[out.i].nValue > nTargetAmount)
                continue;

            int64_t nTxTime = out.tx->GetTxTime();

            //check for min age
            if (GetAdjustedTime() - nTxTime < nStakeMinAge)
                continue;

            // b.X! CMerkleTx [

            //check that it is matured
//            if (out.nDepth < (out.tx->IsCoinStake() ? Params().COINBASE_MATURITY() : 10))
//                continue;
            if (out.nDepth < (out.tx->tx->IsCoinStake() ? COINBASE_MATURITY : 10))
                continue;

            // b.X! CMerkleTx ]

            //add to our stake set
            nAmountSelected += out.tx->tx->vout[out.i].nValue;

            std::unique_ptr<CTheGCCStake> input(new CTheGCCStake());
            input->SetInput((CTransaction) *out.tx, out.i);
            listInputs.emplace_back(std::move(input));
        }
    }

    return true;
}

// m.4.1 SelectStakeCoins "-thegccstake" ]
// k.2.1 kernel Stake [

bool stakeTargetHit(uint256 hashProofOfStake, int64_t nValueIn, uint256 bnTargetPerCoinDay);
bool CheckStake(const CDataStream& ssUniqueID, CAmount nValueIn, const uint64_t nStakeModifier, const uint256& bnTarget,
                unsigned int nTimeBlockFrom, unsigned int& nTimeTx, uint256& hashProofOfStake);

//test hash vs target
bool stakeTargetHit(uint256 hashProofOfStake, int64_t nValueIn, uint256 bnTargetPerCoinDay)
{
    //get the stake weight - weight is equal to coin amount
    arith_uint256 bnCoinDayWeight = arith_uint256(nValueIn) / 100;

    // Now check if proof-of-stake hash meets target protocol
    return UintToArith256(hashProofOfStake) < (bnCoinDayWeight * UintToArith256(bnTargetPerCoinDay));
}

bool CheckStake(const CDataStream& ssUniqueID, CAmount nValueIn, const uint64_t nStakeModifier, const uint256& bnTarget,
                unsigned int nTimeBlockFrom, unsigned int& nTimeTx, uint256& hashProofOfStake)
{
    CDataStream ss(SER_GETHASH, 0);
    ss << nStakeModifier << nTimeBlockFrom << ssUniqueID << nTimeTx;
    hashProofOfStake = Hash(ss.begin(), ss.end());
    //LogPrintf("%s: modifier:%d nTimeBlockFrom:%d nTimeTx:%d hash:%s\n", __func__, nStakeModifier, nTimeBlockFrom, nTimeTx, hashProofOfStake.GetHex());

    return stakeTargetHit(hashProofOfStake, nValueIn, bnTarget);
}


bool Stake(CStakeInput* stakeInput, unsigned int nBits, unsigned int nTimeBlockFrom, unsigned int& nTimeTx, uint256& hashProofOfStake)
{
    if (nTimeTx < nTimeBlockFrom)
        return error("CheckStakeKernelHash() : nTime violation");

    if (nTimeBlockFrom + nStakeMinAge > nTimeTx) // Min age requirement
        return error("CheckStakeKernelHash() : min age violation - nTimeBlockFrom=%d nStakeMinAge=%d nTimeTx=%d",
                     nTimeBlockFrom, nStakeMinAge, nTimeTx);

    //grab difficulty
    arith_uint256 bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);

    //grab stake modifier
    uint64_t nStakeModifier = 0;
    if (!stakeInput->GetModifier(nStakeModifier))
        return error("failed to get kernel stake modifier");

    bool fSuccess = false;
    unsigned int nTryTime = 0;
    int nHeightStart = chainActive.Height();
    int nHashDrift = 30;
    CDataStream ssUniqueID = stakeInput->GetUniqueness();
    CAmount nValueIn = stakeInput->GetValue();
    for (int i = 0; i < nHashDrift; i++) //iterate the hashing
    {
        //new block came in, move on
        if (chainActive.Height() != nHeightStart)
            break;

        //hash this iteration
        nTryTime = nTimeTx + nHashDrift - i;

        // if stake hash does not meet the target then continue to next iteration
        if (!CheckStake(ssUniqueID, nValueIn, nStakeModifier, ArithToUint256(bnTargetPerCoinDay), nTimeBlockFrom, nTryTime, hashProofOfStake))
            continue;

        fSuccess = true; // if we make it this far then we have successfully created a stake hash
        //LogPrintf("%s: hashproof=%s\n", __func__, hashProofOfStake.GetHex());
        nTimeTx = nTryTime;
        break;
    }
// b.XI! mapHashedBlocks [
    /*
    mapHashedBlocks.clear();
    mapHashedBlocks[chainActive.Tip()->nHeight] = GetTime(); //store a time stamp of when we last hashed on this block
     */
// b.XI! mapHashedBlocks ]
    return fSuccess;
}

// k.2.1 kernel Stake ]

#if 01

// CreateNewBlock [

//typedef CTxMemPool::indexed_transaction_set::nth_index<0>::type::iterator txiter;

void UpdateCoins(const CTransaction& tx, CCoinsViewCache& inputs, CTxUndo &txundo, int nHeight);

std::pair<int, std::pair<uint256, uint256> > pCheckpointCache;
CBlockTemplate* CreateNewBlock(const CScript& scriptPubKeyIn, CWallet* pwallet, bool fProofOfStake)
{
    // reservekey [

    CReserveKey reservekey(pwallet);

    // reservekey ]
    // block [

    // Create new block
    unique_ptr<CBlockTemplate> pblocktemplate(new CBlockTemplate());
    if (!pblocktemplate.get())
        return NULL;
    CBlock* pblock = &pblocktemplate->block; // pointer for convenience

    // block ]
    // -blockversion [

    // -regtest only: allow overriding block.nVersion with
    // -blockversion=N to test forking scenarios
    if (Params().MineBlocksOnDemand())
        pblock->nVersion = GetArg("-blockversion", pblock->nVersion);

    // -blockversion ]
    // g.7 !!! block version = 3 [

    bool fActive = true; //GetAdjustedTime() >= Params().TheGCCcoin_StartTime();
    if (!fActive)
        pblock->nVersion = 2;
    else
        pblock->nVersion = 3;

    // g.7 !!! block version = 3 ]
    // Create coinbase tx [

    // Create coinbase tx
    CMutableTransaction txNew;
    txNew.vin.resize(1);
    txNew.vin[0].prevout.SetNull();
    txNew.vout.resize(1);
    txNew.vout[0].scriptPubKey = scriptPubKeyIn;
    pblock->vtx.push_back(std::make_shared<const CTransaction>(CTransaction(txNew)));
    pblocktemplate->vTxFees.push_back(-1);   // updated at end
    pblocktemplate->vTxSigOps.push_back(-1); // updated at end

    // Create coinbase tx ]
    // nLastCoinStakeSearchTime [

    // ppcoin: if coinstake available add coinstake tx
    static int64_t nLastCoinStakeSearchTime = GetAdjustedTime(); // only initialized at startup

    // nLastCoinStakeSearchTime ]
    // if (fProofOfStake) [

    if (fProofOfStake) {
        boost::this_thread::interruption_point();
        pblock->nTime = GetAdjustedTime();
        CBlockIndex* pindexPrev = chainActive.Tip();
        pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, Params().GetConsensus(), fProofOfStake);
        CMutableTransaction txCoinStake;
        int64_t nSearchTime = pblock->nTime; // search to current time
        bool fStakeFound = false;
        if (nSearchTime >= nLastCoinStakeSearchTime) {
            unsigned int nTxNewTime = 0;
            if (pwallet->CreateCoinStake(*pwallet, pblock->nBits, nSearchTime - nLastCoinStakeSearchTime, txCoinStake, nTxNewTime)) {
                pblock->nTime = nTxNewTime;

//                pblock->vtx[0].vout[0].SetEmpty();
                CMutableTransaction vtx0(*pblock->vtx[0]);
                vtx0.vout[0].SetEmpty();
                pblock->vtx[0] = std::make_shared<const CTransaction>(CTransaction(vtx0));

//                pblock->vtx.push_back(CTransaction(txCoinStake));
                pblock->vtx.push_back(std::make_shared<const CTransaction>(CTransaction(txCoinStake)));
                fStakeFound = true;
            }
            nLastCoinStakeSearchInterval = nSearchTime - nLastCoinStakeSearchTime;
            nLastCoinStakeSearchTime = nSearchTime;
        }

        if (!fStakeFound)
            return NULL;
    }

    // if (fProofOfStake) ]
    // -blockmaxsize [

    // Largest block you're willing to create:
    unsigned int nBlockMaxSize = GetArg("-blockmaxsize", DEFAULT_BLOCK_MAX_SIZE);
    // Limit to betweeen 1K and MAX_BLOCK_SIZE-1K for sanity:
    unsigned int nBlockMaxSizeNetwork = MAX_BLOCK_SIZE_CURRENT;
    nBlockMaxSize = std::max((unsigned int)1000, std::min((nBlockMaxSizeNetwork - 1000), nBlockMaxSize));

    // -blockmaxsize ]
    // -blockprioritysize [

    // How much of the block should be dedicated to high-priority transactions,
    // included regardless of the fees they pay
    unsigned int nBlockPrioritySize = GetArg("-blockprioritysize", DEFAULT_BLOCK_PRIORITY_SIZE);
    nBlockPrioritySize = std::min(nBlockMaxSize, nBlockPrioritySize);

    // -blockprioritysize ]
    // -blockminsize [

    // Minimum block size you want to create; block will be filled with free transactions
    // until there are no more or the block reaches this size:
    unsigned int nBlockMinSize = GetArg("-blockminsize", DEFAULT_BLOCK_MIN_SIZE);
    nBlockMinSize = std::min(nBlockMaxSize, nBlockMinSize);

    // -blockminsize ]
    // Collect memory pool transactions into the block [

    // Collect memory pool transactions into the block
    CAmount nFees = 0;

    {
        LOCK2(cs_main, mempool.cs);

        CBlockIndex* pindexPrev = chainActive.Tip();
        const int nHeight = pindexPrev->nHeight + 1;
        CCoinsViewCache view(pcoinsTip);

        // Priority order to process transactions
        list<COrphan> vOrphan; // list memory doesn't move
        map<uint256, vector<COrphan*> > mapDependers;
        bool fPrintPriority = GetBoolArg("-printpriority", false);

        int64_t nLockTimeCutoff = pblock->GetBlockTime();

        // This vector will be sorted into a priority queue:
        vector<TxPriority> vecPriority;
        vecPriority.reserve(mempool.mapTx.size());
        for (CTxMemPool::txiter mi = mempool.mapTx.begin();
//        for (map<uint256, CTxMemPoolEntry>::iterator mi = mempool.mapTx.begin();
             mi != mempool.mapTx.end(); ++mi) {
            const CTransaction& tx = mi->GetTx(); //mi->second.GetTx();
            if (tx.IsCoinBase() || tx.IsCoinStake() || !IsFinalTx(tx, nHeight, nLockTimeCutoff)){
                continue;
            }

            COrphan* porphan = NULL;
            double dPriority = 0;
            CAmount nTotalIn = 0;
            bool fMissingInputs = false;
            uint256 txid = tx.GetHash();
            for (const CTxIn& txin : tx.vin) {
                // Read prev transaction
                if (!view.HaveCoin(txin.prevout)) {
//                    if (!view.HaveCoins(txin.prevout.hash)) {
                    // This should never happen; all transactions in the memory
                    // pool should connect to either transactions in the chain
                    // or other transactions in the memory pool.
                    if (!mempool.mapTx.count(txin.prevout.hash)) {
                        LogPrintf("ERROR: mempool transaction missing input\n");
                        if (fDebug) assert("mempool transaction missing input" == 0);
                        fMissingInputs = true;
                        if (porphan)
                            vOrphan.pop_back();
                        break;
                    }

                    // Has to wait for dependencies
                    if (!porphan) {
                        // Use list for automatic deletion
                        vOrphan.push_back(COrphan(&tx));
                        porphan = &vOrphan.back();
                    }
                    mapDependers[txin.prevout.hash].push_back(porphan);
                    porphan->setDependsOn.insert(txin.prevout.hash);
//                    nTotalIn += mempool.mapTx.find(txin.prevout.hash).GetTx().vout[txin.prevout.n].nValue;
                    nTotalIn += mempool.mapTx.find(txin.prevout.hash)->GetTx().vout[txin.prevout.n].nValue;
                    continue;
                }
#if 0
                //Check for invalid/fraudulent inputs. They shouldn't make it through mempool, but check anyways.
                if (invalid_out::ContainsOutPoint(txin.prevout)) {
                    LogPrintf("%s : found invalid input %s in tx %s", __func__, txin.prevout.ToString(), tx.GetHash().ToString());
                    fMissingInputs = true;
                    break;
                }
#endif
//                const CCoins* coins = view.AccessCoins(txin.prevout.hash);
//                assert(coins);
                const Coin &coins = view.AccessCoin(txin.prevout);

//                CAmount nValueIn = coins->vout[txin.prevout.n].nValue;
                CAmount nValueIn = coins.out.nValue;
                nTotalIn += nValueIn;

//                int nConf = nHeight - coins->nHeight;

                // zPIV spends can have very large priority, use non-overflowing safe functions
//                dPriority = double_safe_addition(dPriority, ((double)nValueIn * nConf));

            }
            if (fMissingInputs) continue;

            // Priority is sum(valuein * age) / modified_txsize
            unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
            dPriority = tx.ComputePriority(dPriority, nTxSize);

            uint256 hash = tx.GetHash();
            mempool.ApplyDeltas(hash, dPriority, nTotalIn);

            CFeeRate feeRate(nTotalIn - tx.GetValueOut(), nTxSize);

            if (porphan) {
                porphan->dPriority = dPriority;
                porphan->feeRate = feeRate;
            } else
                vecPriority.push_back(TxPriority(dPriority, feeRate, &mi->GetTx()));
//                vecPriority.push_back(TxPriority(dPriority, feeRate, &mi->second.GetTx()));
        }

        // Collect memory pool transactions into the block ]
        // Collect transactions into block [

        // Collect transactions into block
        uint64_t nBlockSize = 1000;
        uint64_t nBlockTx = 0;
        int nBlockSigOps = 100;
        bool fSortedByFee = (nBlockPrioritySize <= 0);

        TxPriorityCompare comparer(fSortedByFee);
        std::make_heap(vecPriority.begin(), vecPriority.end(), comparer);

        vector<CBigNum> vBlockSerials;
        vector<CBigNum> vTxSerials;
        while (!vecPriority.empty()) {
            // Take highest priority transaction off the priority queue:
            double dPriority = vecPriority.front().get<0>();
            CFeeRate feeRate = vecPriority.front().get<1>();
            const CTransaction& tx = *(vecPriority.front().get<2>());

            std::pop_heap(vecPriority.begin(), vecPriority.end(), comparer);
            vecPriority.pop_back();

            // Size limits
            unsigned int nTxSize = ::GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
            if (nBlockSize + nTxSize >= nBlockMaxSize)
                continue;

            // Legacy limits on sigOps:
            unsigned int nMaxBlockSigOps = MAX_BLOCK_SIGOPS_CURRENT;
            unsigned int nTxSigOps = GetLegacySigOpCount(tx);
            if (nBlockSigOps + nTxSigOps >= nMaxBlockSigOps)
                continue;

            // Skip free transactions if we're past the minimum block size:
            const uint256& hash = tx.GetHash();
            double dPriorityDelta = 0;
            CAmount nFeeDelta = 0;
            mempool.ApplyDeltas(hash, dPriorityDelta, nFeeDelta);
            if (fSortedByFee && (dPriorityDelta <= 0) && (nFeeDelta <= 0) && (feeRate < ::minRelayTxFee) && (nBlockSize + nTxSize >= nBlockMinSize))
                continue;

            // Prioritise by fee once past the priority size or we run out of high-priority
            // transactions:
            if (!fSortedByFee &&
                ((nBlockSize + nTxSize >= nBlockPrioritySize) || !AllowFree(dPriority))) {
                fSortedByFee = true;
                comparer = TxPriorityCompare(fSortedByFee);
                std::make_heap(vecPriority.begin(), vecPriority.end(), comparer);
            }

            if (!view.HaveInputs(tx))
                continue;

            CAmount nTxFees = view.GetValueIn(tx) - tx.GetValueOut();

            nTxSigOps += GetP2SHSigOpCount(tx, view);
            if (nBlockSigOps + nTxSigOps >= nMaxBlockSigOps)
                continue;

            // Note that flags: we don't want to set mempool/IsStandard()
            // policy here, but we still have to ensure that the block we
            // create only contains transactions that are valid in new blocks.
            CValidationState state;
            if (!CheckInputs(tx, state, view, true, MANDATORY_SCRIPT_VERIFY_FLAGS, true))
                continue;

            CTxUndo txundo;
//            UpdateCoins(tx, state, view, txundo, nHeight);
            UpdateCoins(tx, view, txundo, nHeight);
//            UpdateCoins(tx, view, nHeight);

            // Added
//            pblock->vtx.push_back(tx);
            pblock->vtx.push_back(std::make_shared<const CTransaction>(tx));
            pblocktemplate->vTxFees.push_back(nTxFees);
            pblocktemplate->vTxSigOps.push_back(nTxSigOps);
            nBlockSize += nTxSize;
            ++nBlockTx;
            nBlockSigOps += nTxSigOps;
            nFees += nTxFees;

            for (const CBigNum bnSerial : vTxSerials)
                vBlockSerials.emplace_back(bnSerial);

            if (fPrintPriority) {
                LogPrintf("priority %.1f fee %s txid %s\n",
                          dPriority, feeRate.ToString(), tx.GetHash().ToString());
            }

            // Add transactions that depend on this one to the priority queue
            if (mapDependers.count(hash)) {
                BOOST_FOREACH (COrphan* porphan, mapDependers[hash]) {
                    if (!porphan->setDependsOn.empty()) {
                        porphan->setDependsOn.erase(hash);
                        if (porphan->setDependsOn.empty()) {
                            vecPriority.push_back(TxPriority(porphan->dPriority, porphan->feeRate, porphan->ptx));
                            std::push_heap(vecPriority.begin(), vecPriority.end(), comparer);
                        }
                    }
                }
            }
        }

        // Collect transactions into block ]
        // !fProofOfStake ?-> Masternode and general budget payments [

        // j.1.1 pivx PoW reward [
#if 0
        if (!fProofOfStake) {
            //Masternode and general budget payments
            FillBlockPayee(txNew, nFees, fProofOfStake);

            //Make payee
/*            if (txNew.vout.size() > 1) {
                pblock->payee = txNew.vout[1].scriptPubKey;
            }*/
        }
#endif
        // j.1.1 pivx PoW reward ]

        // j.1.2 old-gcc PoW reward [

        if (pblock->IsProofOfWork()) {
            txNew.vout[0].nValue = GetProofOfWorkReward(((unsigned int) pindexPrev->nHeight) + 1,
                                         nFees, pindexPrev->GetBlockHash());
            pblock->vtx[0] = std::make_shared<const CTransaction>(txNew);

            pblocktemplate->vTxFees[0] = -nFees;
        }

        // j.1.2 old-gcc PoW reward ]


        nLastBlockTx = nBlockTx;
        nLastBlockSize = nBlockSize;
        LogPrintf("CreateNewBlock(): total size %u\n", nBlockSize);

        // !fProofOfStake ?-> Masternode and general budget payments ]
        // set vin0.scriptSig - Compute final coinbase transaction [

        // Compute final coinbase transaction.
        CMutableTransaction vtx0(*pblock->vtx[0]);
        vtx0.vin[0].scriptSig = CScript() << nHeight << OP_0;
        pblock->vtx[0] = std::make_shared<const CTransaction>(CTransaction(vtx0));

        // set vin0.scriptSig - Compute final coinbase transaction ]
        // Fill in header [

        // Fill in header
        pblock->hashPrevBlock = pindexPrev->GetBlockHash();
        if (!fProofOfStake)
            UpdateTime(pblock, Params().GetConsensus(), pindexPrev);
        pblock->nBits = GetNextWorkRequired(pindexPrev, pblock, Params().GetConsensus(), fProofOfStake);
        pblock->nNonce = 0;

        // Fill in header ]
        // Calculate the accumulator checkpoint only if the previous cached checkpoint need to be updated [
#if 0
        //Calculate the accumulator checkpoint only if the previous cached checkpoint need to be updated
        uint256 nCheckpoint;
        uint256 hashBlockLastAccumulated = chainActive[nHeight - (nHeight % 10) - 10]->GetBlockHash();
        if (nHeight >= pCheckpointCache.first || pCheckpointCache.second.first != hashBlockLastAccumulated) {
            pCheckpointCache.second.second = pindexPrev->nAccumulatorCheckpoint;
        }

        // Calculate the accumulator checkpoint only if the previous cached checkpoint need to be updated ]
        // nAccumulatorCheckpoint vTxSigOps [

        pblock->nAccumulatorCheckpoint = pCheckpointCache.second.second;
        pblocktemplate->vTxSigOps[0] = GetLegacySigOpCount(pblock->vtx[0]);

        // nAccumulatorCheckpoint vTxSigOps ]
#endif
        // TestBlockValidity [

        CValidationState state;
        if (!TestBlockValidity(state, Params(), *pblock, pindexPrev, false, false)) {
            LogPrintf("CreateNewBlock() : TestBlockValidity failed\n");
            mempool.clear();
            return NULL;
        }

        // TestBlockValidity ]
    }

    return pblocktemplate.release();
}

#endif

// CreateNewBlock ]
// IncrementExtraNonce [

void IncrementExtraNonce(CBlock* pblock, CBlockIndex* pindexPrev, unsigned int& nExtraNonce)
{
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    if (hashPrevBlock != pblock->hashPrevBlock) {
        nExtraNonce = 0;
        hashPrevBlock = pblock->hashPrevBlock;
    }
    ++nExtraNonce;
    unsigned int nHeight = pindexPrev->nHeight + 1; // Height first in coinbase required for block.version=2
    CMutableTransaction txCoinbase(*pblock->vtx[0].get());
    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(nExtraNonce)) + COINBASE_FLAGS;
    assert(txCoinbase.vin[0].scriptSig.size() <= 100);

    CTransaction tx(txCoinbase);
    pblock->vtx[0] = std::make_shared<const CTransaction>(tx);
//    pblock->vtx[0] = std::make_shared<const CTransactionRef>(CTransaction(txCoinbase));
//    pblock->vtx[0] = txCoinbase;
    pblock->hashMerkleRoot = pblock->BuildMerkleTree();
}

// IncrementExtraNonce ]


// ENABLE_WALLET.1 [

#ifdef ENABLE_WALLET

// CreateNewBlockWithKey [

CBlockTemplate* CreateNewBlockWithKey(CReserveKey& reservekey, CWallet* pwallet, bool fProofOfStake)
{
    // g.5 review GetScriptForMining [
    // a [

    CPubKey pubkey;
    if (!reservekey.GetReservedKey(pubkey, false))
        return NULL;

    CScript scriptPubKey = CScript() << ToByteVector(pubkey) << OP_CHECKSIG;

    // a ]
    // b [

    //CScript scriptPubKey = GetScriptForMining(reservekey);

    // b ]
    // g.5 review GetScriptForMining ]

    return CreateNewBlock(scriptPubKey, pwallet, fProofOfStake);
}

// CreateNewBlockWithKey ]
// ProcessBlockFound [

bool ProcessBlockFound(CBlock* pblock, CWallet& wallet, CReserveKey& reservekey)
{
    LogPrintf("%s\n", pblock->ToString());
    LogPrintf("generated %s\n", FormatMoney(pblock->vtx[0].get()->vout[0].nValue));

    // Found a solution
    {
        LOCK(cs_main);
        if (pblock->hashPrevBlock != chainActive.Tip()->GetBlockHash())
            return error("TheGCCcoinMiner : generated block is stale");
    }

    // Remove key from key pool
    reservekey.KeepKey();

    // Track how many getdata requests this block gets
    {
        LOCK(wallet.cs_wallet);
        wallet.mapRequestCount[pblock->GetHash()] = 0;
    }

    // Inform about the new block
    GetMainSignals().BlockFound(pblock->GetHash());

    // Process this block the same as if we had received it from another node
    CValidationState state;

    // g.6.1.d1 ProcessNewBlock [

    std::shared_ptr<const CBlock> shared_pblock = std::make_shared<const CBlock>(*pblock);
    if (!ProcessNewBlock(Params(), shared_pblock, true, NULL)) {
//    if (!ProcessNewBlock(state, NULL, pblock)) {
        return error("TheGCCcoinMiner : ProcessNewBlock, block not accepted");
    }

    // g.6.1.d1 ProcessNewBlock ]

    {
        LOCK(cs_vNodes);
        for (CNode* node : vNodes) {
            node->PushInventory(CInv(MSG_BLOCK, pblock->GetHash()));
        }
    }

    return true;
}

// ProcessBlockFound ]

#endif // ENABLE_WALLET

// ENABLE_WALLET.1 ]

