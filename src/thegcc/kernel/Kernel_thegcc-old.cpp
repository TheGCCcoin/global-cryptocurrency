// thegcc-old [
// @ [

#include <boost/assign/list_of.hpp>

using namespace std;

//extern int nStakeCapAge; // 9 days
//extern int nStakeMaxAge;

extern int nStakeTargetSpacing;

extern unsigned int VERSION2_SWITCH_TIME;

extern bool fTestNet;

// MODIFIER_INTERVAL [

// Modifier interval: time to elapse before new modifier is computed
// Set to 3-hour for production network and 20-minute for test network

static const unsigned int MODIFIER_INTERVAL = 5 * 60;// * 60; // 20 minutes

unsigned int nModifierInterval = MODIFIER_INTERVAL;

// MODIFIER_INTERVAL ]


// Hard checkpoints of stake modifiers to ensure they are deterministic
static std::map<int, unsigned int> mapStakeModifierCheckpoints =
        boost::assign::map_list_of
                (        0, 0xfd11f4e7u)
;

#define CBigNum arith_uint256

unsigned int GetStakeMaxAge(unsigned int nTime);
unsigned int GetStakeMinAge(unsigned int nTime);
bool GetKernelStakeModifier(uint256 hashBlockFrom, uint64_t& nStakeModifier, int& nStakeModifierHeight, int64_t& nStakeModifierTime, bool fPrintProofOfStake);


// @ ]
// CheckStakeKernelHash [

// ppcoin kernel protocol
// coinstake must meet hash target according to the protocol:
// kernel (input 0) must meet the formula or something similar

//     hash(nStakeModifier + txPrev.block.nTime +
//          txPrev.offset + txPrev.nTime + txPrev.vout.n + nTime)
//     < bnTarget * nCoinDayWeight * nTargetMultiplier
// (the nTargetMultiplier is 10 for the short staking times of XST (72 hr --> 144 hr full wt))
// this ensures that the chance of getting a coinstake is proportional to the
// amount of coin age one owns.
// The reason this hash is chosen is the following:
//   nStakeModifier:
//       (v0.3) scrambles computation to make it very difficult to precompute
//              future proof-of-stake at the time of the coin's confirmation
//       (v0.2) nBits (deprecated): encodes all past block timestamps
//   txPrev.block.nTime: prevent nodes from guessing a good timestamp to
//                       generate transaction for future advantage
//   txPrev.offset: offset of txPrev inside block, to reduce the chance of
//                  nodes generating coinstake at the same time
//   txPrev.nTime: reduce the chance of nodes generating coinstake at the same
//                 time
//   txPrev.vout.n: output number of txPrev, to reduce the chance of nodes
//                  generating coinstake at the same time
//   block/tx hash should not be used here as they can be generated in vast
//   quantities so as to generate blocks faster, degrading the system back into
//   a proof-of-work situation.
//
bool CheckStakeKernelHash(unsigned int nBits, const CBlock& blockFrom, unsigned int nTxPrevOffset, const CTransaction& txPrev, const COutPoint& prevout, unsigned int nTimeTx, uint256& hashProofOfStake, bool fPrintProofOfStake)
{

    unsigned int nTargetMultiplier = 10;

    if (nTimeTx < txPrev.nTime)  // Transaction timestamp violation
        return error("CheckStakeKernelHash() : nTime violation");

    unsigned int nTimeBlockFrom = blockFrom.GetBlockTime();
    if (nTimeBlockFrom + GetStakeMinAge(nTimeBlockFrom) > nTimeTx) // Min age requirement
        return error("CheckStakeKernelHash() : min age violation");

    CBigNum bnTargetPerCoinDay;
    bnTargetPerCoinDay.SetCompact(nBits);
    int64_t nValueIn = txPrev.vout[prevout.n].nValue;

    // v0.3 protocol kernel hash weight starts from 0 at the min age
    // this change increases active coins participating the hash and helps
    // to secure the network when proof-of-stake difficulty is low
    int64_t  nTimeWeight = min((int64_t)nTimeTx - txPrev.nTime, (int64_t)GetStakeMaxAge(nTimeTx) + GetStakeMinAge(nTimeTx)) - GetStakeMinAge(nTimeTx);
    if (nTimeTx > VERSION2_SWITCH_TIME)
        nTimeWeight = min((int64_t)nTimeTx - txPrev.nTime - GetStakeMinAge(nTimeTx), (int64_t)GetStakeMaxAge(nTimeTx));
    else
        nTimeWeight = min((int64_t)nTimeTx - txPrev.nTime, (int64_t)GetStakeMaxAge(nTimeTx) + GetStakeMinAge(nTimeTx)) - GetStakeMinAge(nTimeTx);

    CBigNum bnCoinDayWeight = CBigNum(nValueIn) * nTimeWeight / COIN / (24 * 60 * 60);

    // printf(">>> CheckStakeKernelHash: nTimeWeight = %"PRI64d"\n", nTimeWeight);
    // Calculate hash
    CDataStream ss(SER_GETHASH, 0);
    uint64_t nStakeModifier = 0;
    int nStakeModifierHeight = 0;
    int64_t nStakeModifierTime = 0;

    if (!GetKernelStakeModifier(blockFrom.GetHash(), nStakeModifier, nStakeModifierHeight, nStakeModifierTime, fPrintProofOfStake))
    {
        if(fDebug)
            printf(">>> CheckStakeKernelHash: GetKernelStakeModifier return false\n");

        return false;
    }

    // if(fDebug) {
    //     printf(">>> CheckStakeKernelHash: passed GetKernelStakeModifier\n");
    // }
    ss << nStakeModifier;

    ss << nTimeBlockFrom << nTxPrevOffset << txPrev.nTime << prevout.n << nTimeTx;
    hashProofOfStake = Hash(ss.begin(), ss.end());
    if (fPrintProofOfStake)
    {
/*        printf("CheckStakeKernelHash() : using modifier 0x%016"PRI64x" at height=%d timestamp=%s for block from height=%d timestamp=%s\n",
               nStakeModifier, nStakeModifierHeight,
               DateTimeStrFormat(nStakeModifierTime).c_str(),
               mapBlockIndex[blockFrom.GetHash()]->nHeight,
               DateTimeStrFormat(blockFrom.GetBlockTime()).c_str());
        printf("CheckStakeKernelHash() : check protocol=%s modifier=0x%016"PRI64x" nTimeBlockFrom=%u nTxPrevOffset=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProof=%s\n",
               "0.3",
               nStakeModifier,
               nTimeBlockFrom, nTxPrevOffset, txPrev.nTime, prevout.n, nTimeTx,
               hashProofOfStake.ToString().c_str());
               */
    }

    // Now check if proof-of-stake hash meets target protocol
    // The nTargetMultiplier of 10 is a calibration for stealth
    CBigNum bnProduct = bnCoinDayWeight * bnTargetPerCoinDay * nTargetMultiplier;



    // someone minted with beta
    /*
      if (nTimeBlockFrom <= KERNEL_MODIFIER_TIME_01) {
      printf("\n>>> dividing by 256\n");
      hashProofOfStake = hashProofOfStake >> 8;
    }
    */


    if (UintToArith256(hashProofOfStake) > bnProduct)
    {
        if(fDebug)
        {
            printf(">>> bnCoinDayWeight = %s, bnTargetPerCoinDay=%s\n>>> too small:<bnProduct=%s>\n",
                   bnCoinDayWeight.ToString().c_str(),
                   bnTargetPerCoinDay.ToString().c_str(),
                   bnProduct.ToString().c_str());
            printf(">>> CheckStakeKernelHash - hashProofOfStake too much\n");
            printf(">>> hashProofOfStake too much: %s\n", hashProofOfStake.ToString().c_str());
        }
        return false;
    }


    if (fDebug && !fPrintProofOfStake)
    {
/*        printf("CheckStakeKernelHash() : using modifier 0x%016"PRI64x" at height=%d timestamp=%s for block from height=%d timestamp=%s\n",
               nStakeModifier, nStakeModifierHeight,
               DateTimeStrFormat(nStakeModifierTime).c_str(),
               mapBlockIndex[blockFrom.GetHash()]->nHeight,
               DateTimeStrFormat(blockFrom.GetBlockTime()).c_str());
        printf("CheckStakeKernelHash() : pass protocol=%s modifier=0x%016"PRI64x" nTimeBlockFrom=%u nTxPrevOffset=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProof=%s\n",
               "0.3",
               nStakeModifier,
               nTimeBlockFrom, nTxPrevOffset, txPrev.nTime, prevout.n, nTimeTx,
               hashProofOfStake.ToString().c_str());*/
    }
    return true;
}

// CheckStakeKernelHash ]
// VerifySignature [

bool VerifySignature(const CTransaction& txFrom, const CTransaction& txTo, unsigned int nIn, bool fValidatePayToScriptHash, int nHashType);

bool VerifySignature(const CTransaction& txFrom, const CTransaction& txTo, unsigned int nIn, bool fValidatePayToScriptHash, int nHashType)
{
    assert(nIn < txTo.vin.size());
    const CTxIn& txin = txTo.vin[nIn];
    if (txin.prevout.n >= txFrom.vout.size())
        return false;
    const CTxOut& txout = txFrom.vout[txin.prevout.n];

    if (txin.prevout.hash != txFrom.GetHash())
        return false;

//    return VerifyScript(txin.scriptSig, txout.scriptPubKey, txTo, nIn, fValidatePayToScriptHash, nHashType);

    return !VerifyScript(txin.scriptSig, txFrom.vout[txin.prevout.n].scriptPubKey, STANDARD_SCRIPT_VERIFY_FLAGS, TransactionSignatureChecker(&txTo, 0));
}

// VerifySignature ]
// CheckProofOfStake - yes [

// Check kernel hash target and coinstake signature
bool CheckProofOfStake(const CTransaction& tx, unsigned int nBits, std::unique_ptr<CStakeInput>& stake,
                       uint256& hashProofOfStake, bool fIsInitialDownload)
{
    if (!tx.IsCoinStake())
        return error("CheckProofOfStake() : called on non-coinstake %s", tx.GetHash().ToString().c_str());

    /*
    // give a pass to blocks already in the blockchain, duh (?).
    // this prevents many restarts just to load a chain, which has the same effect
    if (fIsInitialDownload) {
      return true;
    }
    */

    // Kernel (input 0) must match the stake hash target per coin age (nBits)
    const CTxIn& txin = tx.vin[0];

    // First try finding the previous transaction in database
/*    CTxDB txdb("r");
    CTransaction txPrev;
    CTxIndex txindex;
    if (!txPrev.ReadFromDisk(txdb, txin.prevout, txindex))
        // previous transaction not in main chain, may occur during initial download
        return tx.DoS(1, error("CheckProofOfStake() : INFO: read txPrev failed"));
    //txdb.Close();
*/
    // First try finding the previous transaction in database
    uint256 hashBlock;
    CTransactionRef txPrev;
    if (!GetTransaction(txin.prevout.hash, txPrev, Params().GetConsensus(), hashBlock, true))
        return error("CheckProofOfStake() : INFO: read txPrev failed");

    // Verify signature
//    if (!VerifySignature(*txPrev, tx, 0, true, 0))
//        return DoS(100, error("CheckProofOfStake() : VerifySignature failed on coinstake %s", tx.GetHash().ToString().c_str()));

/*    // Read block header
    CBlock block;
    if (!block.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos, false))
        return fDebug? error("CheckProofOfStake() : read block failed") : false; // unable to read block of previous transaction*/

    CTheGCCStake* stakeInput = new CTheGCCStake();
    stakeInput->SetInput(*txPrev, txin.prevout.n);
    stake = std::unique_ptr<CStakeInput>(stakeInput);

    CBlockIndex* pindex = stake->GetIndexFrom();
    // Read block header
    CBlock blockprev;

    if (!ReadBlockFromDisk(blockprev, pindex->GetBlockPos(), Params().GetConsensus())) {

        // l.16 llog ReadBlockFromDisk error [

        llogLog(L"Kernel/CheckProofOfStake/ReadBlockFromDisk-error", L"blockprev", blockprev);
        llogLog(L"Kernel/CheckProofOfStake/ReadBlockFromDisk-error", L"pindex", *pindex);

        // l.16 llog ReadBlockFromDisk error ]

        return error("CheckProofOfStake(): INFO: failed to find block");
    }



//    if (!CheckStakeKernelHash(nBits, block, txindex.pos.nTxPos - txindex.pos.nBlockPos, txPrev, txin.prevout, tx.nTime, hashProofOfStake, fDebug))
//        return state.DoS(1, error("CheckProofOfStake() : INFO: check kernel failed on coinstake %s, hashProof=%s", tx.GetHash().ToString().c_str(), hashProofOfStake.ToString().c_str())); // may occur during initial download or if behind on block chain sync

    return true;
}

// CheckProofOfStake - yes ]
// ComputeNextStakeModifier [

// Stake Modifier (hash modifier of proof-of-stake):
// The purpose of stake modifier is to prevent a txout (coin) owner from
// computing future proof-of-stake generated by this txout at the time
// of transaction confirmation. To meet kernel protocol, the txout
// must hash with a future stake modifier to generate the proof.
// Stake modifier consists of bits each of which is contributed from a
// selected block of a given block group in the past.
// The selection of a block is based on a hash of the block's proof-hash and
// the previous stake modifier.
// Stake modifier is recomputed at a fixed time interval instead of every
// block. This is to make it difficult for an attacker to gain control of
// additional bits in the stake modifier, even after generating a chain of
// blocks.
bool ComputeNextStakeModifier(const CBlockIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier)
{
    nStakeModifier = 0;
    fGeneratedStakeModifier = false;
    if (!pindexPrev)
    {
        fGeneratedStakeModifier = true;
        return true;  // genesis block's modifier is 0
    }

    // p.11 pivx pos fix fGeneratedStakeModifier [

    if (pindexPrev->nHeight == 0) {
        //Give a stake modifier to the first block
        fGeneratedStakeModifier = true;
        nStakeModifier = uint64_t("stakemodifier");
        return true;
    }

    // p.11 pivx pos fix fGeneratedStakeModifier ]

    // First find current stake modifier and its generation block time
    // if it's not old enough, return the same stake modifier
    int64_t nModifierTime = 0;
    if (!GetLastStakeModifier(pindexPrev, nStakeModifier, nModifierTime))
        return error("ComputeNextStakeModifier: unable to get last modifier");
    if (fDebug)
    {
//        printf("ComputeNextStakeModifier: prev modifier=0x%016"PRI64x" time=%s\n", nStakeModifier, DateTimeStrFormat(nModifierTime).c_str());
    }
    if (nModifierTime / nModifierInterval >= pindexPrev->GetBlockTime() / nModifierInterval)
        return true;

    // Sort candidate blocks by timestamp
    vector<pair<int64_t, uint256> > vSortedByTimestamp;
    vSortedByTimestamp.reserve(64 * nModifierInterval / nStakeTargetSpacing);
    int64_t nSelectionInterval = GetStakeModifierSelectionInterval();
    int64_t nSelectionIntervalStart = (pindexPrev->GetBlockTime() / nModifierInterval) * nModifierInterval - nSelectionInterval;
    const CBlockIndex* pindex = pindexPrev;
    while (pindex && pindex->GetBlockTime() >= nSelectionIntervalStart)
    {
        vSortedByTimestamp.push_back(make_pair(pindex->GetBlockTime(), pindex->GetBlockHash()));
        pindex = pindex->pprev;
    }
    int nHeightFirstCandidate = pindex ? (pindex->nHeight + 1) : 0;
    reverse(vSortedByTimestamp.begin(), vSortedByTimestamp.end());
    sort(vSortedByTimestamp.begin(), vSortedByTimestamp.end());

    // Select 64 blocks from candidate blocks to generate stake modifier
    uint64_t nStakeModifierNew = 0;
    int64_t nSelectionIntervalStop = nSelectionIntervalStart;
    map<uint256, const CBlockIndex*> mapSelectedBlocks;
    for (int nRound=0; nRound<min(64, (int)vSortedByTimestamp.size()); nRound++)
    {
        // add an interval section to the current selection round
        nSelectionIntervalStop += GetStakeModifierSelectionIntervalSection(nRound);
        // select a block from the candidates of current round
        if (!SelectBlockFromCandidates(vSortedByTimestamp, mapSelectedBlocks, nSelectionIntervalStop, nStakeModifier, &pindex))
            return error("ComputeNextStakeModifier: unable to select block at round %d", nRound);
        // write the entropy bit of the selected block
        nStakeModifierNew |= (((uint64_t)pindex->GetStakeEntropyBit()) << nRound);
        // add the selected block from candidates to selected list
        mapSelectedBlocks.insert(make_pair(pindex->GetBlockHash(), pindex));
//        if (fDebug && GetBoolArg("-printstakemodifier", false))
//            printf("ComputeNextStakeModifier: selected round %d stop=%s height=%d bit=%d\n",
//                   nRound, DateTimeStrFormat(nSelectionIntervalStop).c_str(), pindex->nHeight, pindex->GetStakeEntropyBit());
    }

    // Print selection map for visualization of the selected blocks
    if (fDebug && GetBoolArg("-printstakemodifier", false))
    {
        string strSelectionMap = "";
        // '-' indicates proof-of-work blocks not selected
        strSelectionMap.insert(0, pindexPrev->nHeight - nHeightFirstCandidate + 1, '-');
        pindex = pindexPrev;
        while (pindex && pindex->nHeight >= nHeightFirstCandidate)
        {
            // '=' indicates proof-of-stake blocks not selected
            if (pindex->IsProofOfStake())
                strSelectionMap.replace(pindex->nHeight - nHeightFirstCandidate, 1, "=");
            pindex = pindex->pprev;
        }
        BOOST_FOREACH(const PAIRTYPE(uint256, const CBlockIndex*)& item, mapSelectedBlocks)
        {
            // 'S' indicates selected proof-of-stake blocks
            // 'W' indicates selected proof-of-work blocks
            strSelectionMap.replace(item.second->nHeight - nHeightFirstCandidate, 1, item.second->IsProofOfStake()? "S" : "W");
        }
        printf("ComputeNextStakeModifier: selection height [%d, %d] map %s\n", nHeightFirstCandidate, pindexPrev->nHeight, strSelectionMap.c_str());
    }
    if (fDebug)
    {
//        printf("ComputeNextStakeModifier: new modifier=0x%016"PRI64x" time=%s\n", nStakeModifierNew, DateTimeStrFormat(pindexPrev->GetBlockTime()).c_str());
    }

    nStakeModifier = nStakeModifierNew;
    fGeneratedStakeModifier = true;
    return true;
}

// ComputeNextStakeModifier ]
// p.9.1.1 thegcc-old GetStakeModifierChecksum - no [
#if 0
// Get stake modifier checksum
unsigned int GetStakeModifierChecksum(const CBlockIndex* pindex)
{
    assert (pindex->pprev || pindex->GetBlockHash() == (!fTestNet ? hashGenesisBlock : hashGenesisBlockTestNet));
    // Hash previous checksum with flags, hashProofOfStake and nStakeModifier
    CDataStream ss(SER_GETHASH, 0);
    if (pindex->pprev)
        ss << pindex->pprev->nStakeModifierChecksum;
    ss << pindex->nFlags << pindex->hashProofOfStake << pindex->nStakeModifier;
    uint256 hashChecksum = Hash(ss.begin(), ss.end());
    hashChecksum >>= (256 - 32);
    if(fDebug)
        printf("stake checksum: 0x%016"PRI64x"", hashChecksum.Get64());
    return hashChecksum.Get64();
}
#endif
// p.9.1.1 thegcc-old GetStakeModifierChecksum - no ]
// CheckStakeModifierCheckpoints ==pivx [

// Check stake modifier hard checkpoints
bool CheckStakeModifierCheckpoints(int nHeight, unsigned int nStakeModifierChecksum)
{
    if (fTestNet) return true; // Testnet has no checkpoints
    if (mapStakeModifierCheckpoints.count(nHeight))
    {
        return nStakeModifierChecksum == mapStakeModifierCheckpoints[nHeight];
    }
    return true;
}

// CheckStakeModifierCheckpoints ==pivx ]
// GetLastStakeModifier - ==pivx [

// Get the last stake modifier and its generation time from a given block
bool GetLastStakeModifier(const CBlockIndex* pindex, uint64_t& nStakeModifier, int64_t& nModifierTime)
{
    if (!pindex)
        return error("GetLastStakeModifier: null pindex");
    while (pindex && pindex->pprev && !pindex->GeneratedStakeModifier())
        pindex = pindex->pprev;
    if (!pindex->GeneratedStakeModifier())
        return error("GetLastStakeModifier: no generation at genesis block");
    nStakeModifier = pindex->nStakeModifier;
    nModifierTime = pindex->GetBlockTime();
    return true;
}

// GetLastStakeModifier - ==pivx ]
// p.9.2.1 SelectBlockFromCandidates - yes [

// select a block from the candidate blocks in vSortedByTimestamp, excluding
// already selected blocks in vSelectedBlocks, and with timestamp up to
// nSelectionIntervalStop.
bool SelectBlockFromCandidates(
        std::vector<std::pair<int64_t, uint256> >& vSortedByTimestamp,
        std::map<uint256, const CBlockIndex*>& mapSelectedBlocks,
        int64_t nSelectionIntervalStop, uint64_t nStakeModifierPrev,
        const CBlockIndex** pindexSelected)
{
    bool fSelected = false;
    arith_uint256 hashBest = 0;
    *pindexSelected = (const CBlockIndex*) 0;
    BOOST_FOREACH(const PAIRTYPE(int64_t, uint256)& item, vSortedByTimestamp)
    {
        if (!mapBlockIndex.count(item.second))
            return error("SelectBlockFromCandidates: failed to find block index for candidate block %s", item.second.ToString().c_str());
        const CBlockIndex* pindex = mapBlockIndex[item.second];
        if (fSelected && pindex->GetBlockTime() > nSelectionIntervalStop)
            break;
        if (mapSelectedBlocks.count(pindex->GetBlockHash()) > 0)
            continue;
        // compute the selection hash by hashing its proof-hash and the
        // previous proof-of-stake modifier
        uint256 hashProof = pindex->IsProofOfStake2()? pindex->hashProofOfStake : pindex->GetBlockHash();
        CDataStream ss(SER_GETHASH, 0);
        ss << hashProof << nStakeModifierPrev;
//        arith_uint256 hashSelection = ProofHash(ss.begin(), ss.end());
        arith_uint256 hashSelection = UintToArith256(Hash(ss.begin(), ss.end()));
        // the selection hash is divided by 2**32 so that proof-of-stake block
        // is always favored over proof-of-work block. this is to preserve
        // the energy efficiency property
        if (pindex->IsProofOfStake2())
            hashSelection >>= 32;
        if (fSelected && hashSelection < hashBest)
        {
            hashBest = hashSelection;
            *pindexSelected = (const CBlockIndex*) pindex;
        }
        else if (!fSelected)
        {
            fSelected = true;
            hashBest = hashSelection;
            *pindexSelected = (const CBlockIndex*) pindex;
        }
    }
    if (fDebug && GetBoolArg("-printstakemodifier", false))
        printf("SelectBlockFromCandidates: selection hash=%s\n", hashBest.ToString().c_str());
    return fSelected;
}

// p.9.2.1 SelectBlockFromCandidates - no]
// thegcc-old ]
