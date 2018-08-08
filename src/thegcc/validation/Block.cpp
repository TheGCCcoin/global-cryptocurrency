// @ [

#include "thegcc/imports.h"

#include "thegcc/validation/Block.h"


bool AcceptBlock(const std::shared_ptr<const CBlock>& pblock, CValidationState& state, const CChainParams& chainparams, CBlockIndex** ppindex, bool fRequested, const CDiskBlockPos* dbp, bool* fNewBlock);
void CheckBlockIndex(const Consensus::Params& consensusParams);
void NotifyHeaderTip();

int64_t FutureDrift(int64_t nTime);
bool CheckCoinStakeTimestamp(int64_t nTimeBlock, int64_t nTimeTx);

extern unsigned int CUTOFF_POW_TIME;

// benchmark [

static int64_t nTimeCheck = 0;
static int64_t nTimeForks = 0;
static int64_t nTimeVerify = 0;
static int64_t nTimeConnect = 0;
static int64_t nTimeIndex = 0;
static int64_t nTimeCallbacks = 0;
static int64_t nTimeTotal = 0;

// benchmark ]
// extern validation.cpp [

void UpdateCoins(const CTransaction& tx, CCoinsViewCache& inputs, CTxUndo &txundo, int nHeight);
bool FindUndoPos(CValidationState &state, int nFile, CDiskBlockPos &pos, unsigned int nAddSize);

//namespace {
    bool UndoWriteToDisk(const CBlockUndo &blockundo, CDiskBlockPos &pos, const uint256 &hashBlock,
                         const CMessageHeader::MessageStartChars &messageStart);
    bool AbortNode(const std::string &strMessage, const std::string &userMessage = "");
    bool AbortNode(CValidationState &state, const std::string &strMessage, const std::string &userMessage = "");
//}

extern bool fAddressIndex;
extern bool fSpentIndex;
extern bool fTimestampIndex;

extern CCheckQueue<CScriptCheck> scriptcheckqueue;

namespace {
    std::set<CBlockIndex*> setDirtyBlockIndex;
}

// extern validation.cpp ]
// @ ]
// dash [
// CheckBlockHeader - ok [

bool CheckBlockHeader(const CBlockHeader& block, CValidationState& state, const Consensus::Params& consensusParams, bool fCheckPOW)
{
//    llogLog(L"validation.cpp/Hash", L"hash", block.GetHash().GetHex());

    // check PoW [

    // Check proof of work matches claimed amount
    if (fCheckPOW && !CheckProofOfWork(block.GetHash(), block.nBits, consensusParams)) {

        // f.5.1 skip error if: 1. old blockchain, 2. until checkpoint 3. SPORK disable old blocks - todo: audit [

        if (block.nVersion == THEGCC_V1_BLOCK_VERSION)
            return true;

        // f.5.2 skip error if: 1. old blockchain, 2. until checkpoint 3. SPORK disable old blocks - todo: audit ]

        /*
        if (pblock->IsProofOfStake())
        {
            uint256 hashProofOfStake = 0;
            if (!CheckProofOfStake(pblock->vtx[1], pblock->nBits, hashProofOfStake, IsInitialBlockDownload()))
            {
                printf("WARNING: ProcessBlock(): check proof-of-stake failed for block %s\n", hash.ToString().c_str());
                return false; // do not error here as we expect this during initial block download
            }
            if (!mapProofOfStake.count(hash)) // add to mapProofOfStake
                mapProofOfStake.insert(make_pair(hash, hashProofOfStake));
        }
        */



        // l.7. llog error validation CheckBlockHeader [

        llogLog(L"validation.cpp/Errors", L"proof of work failed", block);
        llogLog(L"validation.cpp/Errors", L"CheckProofOfWork", (int)CheckProofOfWork(block.GetHash(), block.nBits, consensusParams));
        std::wostringstream ss;
        ss << "block.nTime == " << block.nTime << " ||\n";
        llogLog(L"validation.cpp/Errors1", L"block.nTime ==", (int)block.nTime);
        llogLog(L"validation.cpp/Errors1", ss.str());
//        badBlocks.push_back(block);
//        badBlocks[block.nTime] = block;
//        llogLog(L"validation.cpp/Errors1", L"bad block pow ", badBlocks.size(), true);

        return true;

        // l.7. llog error validation CheckBlockHeader ]

        return state.DoS(50, false, REJECT_INVALID, "high-hash", false, "proof of work failed");
    }

    // check PoW ]
    // check DevNet [

    // Check DevNet
    if (!consensusParams.hashDevnetGenesisBlock.IsNull() &&
        block.hashPrevBlock == consensusParams.hashGenesisBlock &&
        block.GetHash() != consensusParams.hashDevnetGenesisBlock) {
        return state.DoS(100, error("CheckBlockHeader(): wrong devnet genesis"),
                         REJECT_INVALID, "devnet-genesis");
    }

    // check DevNet ]

    return true;
}

// CheckBlockHeader - ok ]
// CheckBlock [

bool CheckBlock(const CBlock& block, CValidationState& state, const Consensus::Params& consensusParams, bool fCheckPOW, bool fCheckMerkleRoot, bool fCheckSig)
{
    // + is block.fChecked [

    // These are checks that are independent of context.

    if (block.fChecked)
        return true;

    // + is block.fChecked ]

    //x l.16 llog CheckBlock [

//    llogLog(L"Block.cpp/CheckBlock", L"block", block.GetHash().GetHex());
//    llogLog(L"Block.cpp/CheckBlock", L"block", block);

    //x l.16 llog CheckBlock ]

    // + check pow/dev - CheckBlockHeader [

    // Check that the header is valid (particularly PoW).  This is mostly
    // redundant with the call in AcceptBlockHeader.
    if (!CheckBlockHeader(block, state, consensusParams, fCheckPOW && block.IsProofOfWork()))
        return false;

    // + check pow/dev - CheckBlockHeader ]
    // + thegcc: check timestamp [

    // Check timestamp
    if (block.GetBlockTime() > FutureDrift(GetAdjustedTime()))
        return error("CheckBlock() : block timestamp too far in the future");

    // + thegcc: check timestamp ]
    // x pivx: check timestamp pow/pos [
/*
    // Check timestamp
    LogPrint("debug", "%s: block=%s  is proof of stake=%d\n", __func__, block.GetHash().ToString().c_str(), block.IsProofOfStake());
    if (block.GetBlockTime() > GetAdjustedTime() + (block.IsProofOfStake() ? 180 : 7200)) // 3 minute future drift for PoS
        return state.Invalid(error("CheckBlock() : block timestamp too far in the future"),
                             REJECT_INVALID, "time-too-new");
*/
    // x pivx: check timestamp pow/pos ]
    // + merkle root [

    // Check the merkle root.
    if (fCheckMerkleRoot) {
        bool mutated;
        uint256 hashMerkleRoot2 = BlockMerkleRoot(block, &mutated);
        if (block.hashMerkleRoot != hashMerkleRoot2)
            return state.DoS(100, false, REJECT_INVALID, "bad-txnmrklroot", true, "hashMerkleRoot mismatch");

        // Check for merkle tree malleability (CVE-2012-2459): repeating sequences
        // of transactions in a block without affecting the merkle root of a block,
        // while still invalidating it.
        if (mutated)
            return state.DoS(100, false, REJECT_INVALID, "bad-txns-duplicate", true, "duplicate transaction");
    }

    // + merkle root ]
    // +? size limits [

    // All potential-corruption validation must be done before we do any
    // transaction validation, as otherwise we may mark the header as invalid
    // because we receive the wrong transactions for it.

    // Size limits (relaxed)

    // d.5 dash fDIP0001Active [

    bool fDIP0001Active = true; // 2MB max
    //MAX_BLOCK_SIZE_CURRENT // pivx 2MB also

    // d.5 dash fDIP0001Active ]

    unsigned int nMaxBlockSize = MaxBlockSize(fDIP0001Active);
    if (block.vtx.empty() || block.vtx.size() > nMaxBlockSize || ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION) > nMaxBlockSize)
        return state.DoS(100, false, REJECT_INVALID, "bad-blk-length", false, "size limits failed");

    // +? size limits ]
    // + check coinbase transaction [

    // First transaction must be coinbase, the rest must not be
    if (block.vtx.empty() || !block.vtx[0]->IsCoinBase())
        return state.DoS(100, false, REJECT_INVALID, "bad-cb-missing", false, "first tx is not coinbase");
    for (unsigned int i = 1; i < block.vtx.size(); i++)
        if (block.vtx[i]->IsCoinBase())
            return state.DoS(100, false, REJECT_INVALID, "bad-cb-multiple", false, "more than one coinbase");

    // + check coinbase transaction ]
    // thegcc: check coinbase timestamp [

    // Check coinbase timestamp
    if (block.GetBlockTime() > FutureDrift(block.vtx[0]->nTime))
        return state.DoS(50, error("CheckBlock() : coinbase timestamp is too early"));

    // thegcc: check coinbase timestamp ]
    // thegcc: pos [

    if (block.IsProofOfStake())
    {
        if (block.vtx[0]->vout.size() != 1 || !block.vtx[0]->vout[0].IsEmpty())
            return state.DoS(100, error("CheckBlock() : coinbase output not empty for proof-of-stake block"));

        // Second transaction must be coinstake, the rest must not be
        if (block.vtx.empty() || !block.vtx[1]->IsCoinStake())
            return state.DoS(100, error("CheckBlock() : second tx is not coinstake"));

        for (unsigned int i = 2; i < block.vtx.size(); i++)
            if (block.vtx[i]->IsCoinStake())
                return state.DoS(100, error("CheckBlock() : more than one coinstake"));

        // Check coinstake timestamp
        if (!CheckCoinStakeTimestamp(block.GetBlockTime(), block.vtx[1]->nTime))
            return state.DoS(50, error("CheckBlock() : coinstake timestamp violation nTimeBlock=%lu nTimeTx=%u", block.GetBlockTime(), block.vtx[1]->nTime));

        if ((block.GetBlockTime() >= CUTOFF_POW_TIME) && (block.IsProofOfWork()))
            return state.DoS(100, error("CheckBlock() : Proof of work (%f ATC) at t=%d on or after %d.\n",
                                  ((double) block.vtx[0]->GetValueOut() / (double) COIN),
                                  (int) block.GetBlockTime(),
                                  (int) CUTOFF_POW_TIME));

//thegcc        if (false || fCheckSig & !block.CheckBlockSignature())
        if (fCheckSig & !CheckBlockSignature(block)) //pivx
            return state.DoS(100, error("CheckBlock() : bad proof-of-stake block signature"));
    }

    // thegcc: pos ]
    // dash: check SPORK_3_INSTANTSEND_BLOCK_FILTERING [

    // TheGCCcoin : CHECK TRANSACTIONS FOR INSTANTSEND

    if(sporkManager.IsSporkActive(SPORK_3_INSTANTSEND_BLOCK_FILTERING)) {
        // We should never accept block which conflicts with completed transaction lock,
        // that's why this is in CheckBlock unlike coinbase payee/amount.
        // Require other nodes to comply, send them some data in case they are missing it.
        for(const auto& tx : block.vtx) {
            // skip coinbase, it has no inputs
            if (tx->IsCoinBase()) continue;
            // LOOK FOR TRANSACTION LOCK IN OUR MAP OF OUTPOINTS
            for (const auto& txin : tx->vin) {
                uint256 hashLocked;
                if(instantsend.GetLockedOutPointTxHash(txin.prevout, hashLocked) && hashLocked != tx->GetHash()) {
                    // The node which relayed this will have to switch later,
                    // relaying instantsend data won't help it.
                    LOCK(cs_main);
                    mapRejectedBlocks.insert(std::make_pair(block.GetHash(), GetTime()));
                    return state.DoS(100, false, REJECT_INVALID, "conflict-tx-lock", false,
                                     strprintf("transaction %s conflicts with transaction lock %s", tx->GetHash().ToString(), hashLocked.ToString()));
                }
            }
        }
    } else {
        LogPrintf("CheckBlock(TheGCCcoin): spork is off, skipping transaction locking checks\n");
    }

    // END TheGCCcoin
    // dash: check SPORK_3_INSTANTSEND_BLOCK_FILTERING ]
    // check transactions [

    // Check transactions
    for (const auto& tx : block.vtx)
        if (!CheckTransaction(*tx, state, false))
            return state.Invalid(false, state.GetRejectCode(), state.GetRejectReason(),
                                 strprintf("Transaction check failed (tx hash %s) %s", tx->GetHash().ToString(), state.GetDebugMessage()));

    unsigned int nSigOps = 0;
    for (const auto& tx : block.vtx)
    {
        nSigOps += GetLegacySigOpCount(*tx);
    }
    // sigops limits (relaxed)
    if (nSigOps > MaxBlockSigOps(true))
        return state.DoS(100, false, REJECT_INVALID, "bad-blk-sigops", false, "out-of-bounds SigOpCount");

    // check transactions ]

    if (fCheckPOW && fCheckMerkleRoot)
        block.fChecked = true;

    return true;
}

// CheckBlock ]
// ProcessNewBlock [

bool ProcessNewBlock(const CChainParams& chainparams, const std::shared_ptr<const CBlock> pblock, bool fForceProcessing, bool *fNewBlock)
{
    {
        //x l.12 llog ProcessNewBlock [

//        llogLog(L"validation.cpp/ProcessNewBlock", L"new block", *pblock);

        //x l.12 llog ProcessNewBlock ]

        CBlockIndex *pindex = NULL;
        if (fNewBlock) *fNewBlock = false;
        CValidationState state;
        // Ensure that CheckBlock() passes before calling AcceptBlock, as
        // belt-and-suspenders.
        bool ret = CheckBlock(*pblock, state, chainparams.GetConsensus());

        LOCK(cs_main);

        if (ret) {
            // Store to disk
            ret = AcceptBlock(pblock, state, chainparams, &pindex, fForceProcessing, NULL, fNewBlock);
        }

        // l.15.1 llog ProcessNewBlock height [

        if (pindex && pindex->nHeight % 1000 == 0)
            llogLog(L"ProcessNewBlock/Height", L"nHeight", pindex->nHeight, true);

        // l.15.1 llog ProcessNewBlock height ]
        // l.15.2 llog ProcessNewBlock err [

        if (!ret) {
//            llogLog(L"ProcessNewBlock/error AcceptBlock", L"nHeight", pindex->nHeight);
            llogLog(L"ProcessNewBlock/error AcceptBlock", L"", *pblock);
            ret = AcceptBlock(pblock, state, chainparams, &pindex, fForceProcessing, NULL, fNewBlock);
        }

        // l.15.2 llog ProcessNewBlock err ]

        CheckBlockIndex(chainparams.GetConsensus());
        if (!ret) {
            GetMainSignals().BlockChecked(*pblock, state);
            return error("%s: AcceptBlock FAILED", __func__);
        }
    }

    NotifyHeaderTip();

    CValidationState state; // Only used to report errors, not invalidity - ignore it
    if (!ActivateBestChain(state, chainparams, pblock))
        return error("%s: ActivateBestChain failed", __func__);

    LogPrintf("%s : ACCEPTED\n", __func__);
    return true;
}

// ProcessNewBlock ]
// ConnectBlock - *refactor [

/** Apply the effects of this block (with given index) on the UTXO set represented by coins.
 *  Validity checks that depend on the UTXO set are also done; ConnectBlock()
 *  can fail if those validity checks fail (among other reasons). */
bool ConnectBlock(const CBlock& block, CValidationState& state, CBlockIndex* pindex,
                         CCoinsViewCache& view, const CChainParams& chainparams, bool fJustCheck)
{
    AssertLockHeld(cs_main);

    int64_t nTimeStart = GetTimeMicros();

    // l.15 llog ConnectBlock height [

    if (pindex->nHeight % 1000 == 0)
        llogLog(L"ConnectBlock/Height", L"nHeight", pindex->nHeight, true);

    // l.15 llog ConnectBlock height ]

    // CheckBlock [

    // Check it again in case a previous version let a bad block in
    if (!CheckBlock(block, state, chainparams.GetConsensus(), !fJustCheck, !fJustCheck))
        return error("%s: Consensus::CheckBlock: %s", __func__, FormatStateMessage(state));

    // CheckBlock ]
    // hashPrevBlock == view.GetBestBlock ? [

    // verify that the view's current state corresponds to the previous block
    uint256 hashPrevBlock = pindex->pprev == NULL ? uint256() : pindex->pprev->GetBlockHash();
    assert(hashPrevBlock == view.GetBestBlock());

    // hashPrevBlock == view.GetBestBlock ? ]
    // block == genesis ? -> true [

    // Special case for the genesis block, skipping connection of its transactions
    // (its coinbase is unspendable)
    if (block.GetHash() == chainparams.GetConsensus().hashGenesisBlock) {
        if (!fJustCheck)
            view.SetBestBlock(pindex->GetBlockHash());
        return true;
    }

    // block == genesis ? -> true ]
    // hashAssumeValid ? [

    bool fScriptChecks = true;
    if (!hashAssumeValid.IsNull()) {
        // We've been configured with the hash of a block which has been externally verified to have a valid history.
        // A suitable default value is included with the software and updated from time to time.  Because validity
        //  relative to a piece of software is an objective fact these defaults can be easily reviewed.
        // This setting doesn't force the selection of any particular chain but makes validating some faster by
        //  effectively caching the result of part of the verification.
        BlockMap::const_iterator  it = mapBlockIndex.find(hashAssumeValid);
        if (it != mapBlockIndex.end()) {
            if (it->second->GetAncestor(pindex->nHeight) == pindex &&
                pindexBestHeader->GetAncestor(pindex->nHeight) == pindex &&
                pindexBestHeader->nChainWork >= UintToArith256(chainparams.GetConsensus().nMinimumChainWork)) {
                // This block is a member of the assumed verified chain and an ancestor of the best header.
                // The equivalent time check discourages hashpower from extorting the network via DOS attack
                //  into accepting an invalid block through telling users they must manually set assumevalid.
                //  Requiring a software change or burying the invalid block, regardless of the setting, makes
                //  it hard to hide the implication of the demand.  This also avoids having release candidates
                //  that are hardly doing any signature verification at all in testing without having to
                //  artificially set the default assumed verified block further back.
                // The test against nMinimumChainWork prevents the skipping when denied access to any chain at
                //  least as good as the expected chain.
                fScriptChecks = (GetBlockProofEquivalentTime(*pindexBestHeader, *pindex, *pindexBestHeader, chainparams.GetConsensus()) <= 60 * 60 * 24 * 7 * 2);
            }
        }
    }

    // hashAssumeValid ? ]
    // D.1 fScriptChecks &= pindex->nHeight >= Checkpoints::GetTotalBlocksEstimate() - disable checks ]

    int64_t nTime1 = GetTimeMicros(); nTimeCheck += nTime1 - nTimeStart;
    LogPrint("bench", "    - Sanity checks: %.2fms [%.2fs]\n", 0.001 * (nTime1 - nTimeStart), nTimeCheck * 0.000001);

    // d.2 fEnforceBIP30 dash [

    // Do not allow blocks that contain transactions which 'overwrite' older transactions,
    // unless those are already completely spent.
    // If such overwrites are allowed, coinbases and transactions depending upon those
    // can be duplicated to remove the ability to spend the first instance -- even after
    // being sent to another address.
    // See BIP30 and http://r6.ca/blog/20120206T005236Z.html for more information.
    // This logic is not necessary for memory pool transactions, as AcceptToMemoryPool
    // already refuses previously-known transaction ids entirely.
    // This rule was originally applied to all blocks with a timestamp after March 15, 2012, 0:00 UTC.
    // Now that the whole chain is irreversibly beyond that time it is applied to all blocks except the
    // two in the chain that violate it. This prevents exploiting the issue against nodes during their
    // initial block download.
    bool fEnforceBIP30 = (!pindex->phashBlock) || // Enforce on CreateNewBlock invocations which don't have a hash.
                         !((pindex->nHeight==91842 && pindex->GetBlockHash() == uint256S("0x00000000000a4d0a398161ffc163c503763b1f4360639393e0e4c8e300e0caec")) ||
                           (pindex->nHeight==91880 && pindex->GetBlockHash() == uint256S("0x00000000000743f190a18c5577a3c2d2a1f610ae9601ac046a38084ccb7cd721")));

    // Once BIP34 activated it was not possible to create new duplicate coinbases and thus other than starting
    // with the 2 existing duplicate coinbase pairs, not possible to create overwriting txs.  But by the
    // time BIP34 activated, in each of the existing pairs the duplicate coinbase had overwritten the first
    // before the first had been spent.  Since those coinbases are sufficiently buried its no longer possible to create further
    // duplicate transactions descending from the known pairs either.
    // If we're on the known chain at height greater than where BIP34 activated, we can save the db accesses needed for the BIP30 check.
    CBlockIndex *pindexBIP34height = pindex->pprev->GetAncestor(chainparams.GetConsensus().BIP34Height);
    //Only continue to enforce if we're below BIP34 activation height or the block hash at that height doesn't correspond.
    fEnforceBIP30 = fEnforceBIP30 && (!pindexBIP34height || !(pindexBIP34height->GetBlockHash() == chainparams.GetConsensus().BIP34Hash));

    if (fEnforceBIP30) {
        for (const auto& tx : block.vtx) {
            for (size_t o = 0; o < tx->vout.size(); o++) {
                if (view.HaveCoin(COutPoint(tx->GetHash(), o))) {
                    return state.DoS(100, error("ConnectBlock(): tried to overwrite transaction"),
                                     REJECT_INVALID, "bad-txns-BIP30");
                }
            }
        }
    }

    // d.2 fEnforceBIP30 dash ]
    // f.7 fix dash - disable Check superblock start [
/*
    /// TheGCCcoin: Check superblock start

    // make sure old budget is the real one
    if (pindex->nHeight == chainparams.GetConsensus().nSuperblockStartBlock &&
        chainparams.GetConsensus().nSuperblockStartHash != uint256() &&
        block.GetHash() != chainparams.GetConsensus().nSuperblockStartHash)
        return state.DoS(100, error("ConnectBlock(): invalid superblock start"),
                         REJECT_INVALID, "bad-sb-start");

    /// END TheGCCcoin
*/
    // f.7 fix dash - disable Check superblock start ]
    // BIP16 [

    // BIP16 didn't become active until Apr 1 2012
    int64_t nBIP16SwitchTime = 1333238400;
    bool fStrictPayToScriptHash = (pindex->GetBlockTime() >= nBIP16SwitchTime);

    // BIP16 ]
    // flags = (fStrictPayToScriptHash ? SCRIPT_VERIFY_P2SH : SCRIPT_VERIFY_NONE) | SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY... [

    unsigned int flags = fStrictPayToScriptHash ? SCRIPT_VERIFY_P2SH : SCRIPT_VERIFY_NONE;

    // Start enforcing the DERSIG (BIP66) rule
    if (pindex->nHeight >= chainparams.GetConsensus().BIP66Height) {
        flags |= SCRIPT_VERIFY_DERSIG;
    }

    // Start enforcing CHECKLOCKTIMEVERIFY (BIP65) rule
    if (pindex->nHeight >= chainparams.GetConsensus().BIP65Height) {
        flags |= SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY;
    }

    // Start enforcing BIP68 (sequence locks) and BIP112 (CHECKSEQUENCEVERIFY) using versionbits logic.
    int nLockTimeFlags = 0;
    if (VersionBitsState(pindex->pprev, chainparams.GetConsensus(), Consensus::DEPLOYMENT_CSV, versionbitscache) == THRESHOLD_ACTIVE) {
        flags |= SCRIPT_VERIFY_CHECKSEQUENCEVERIFY;
        nLockTimeFlags |= LOCKTIME_VERIFY_SEQUENCE;
    }

    if (VersionBitsState(pindex->pprev, chainparams.GetConsensus(), Consensus::DEPLOYMENT_BIP147, versionbitscache) == THRESHOLD_ACTIVE) {
        flags |= SCRIPT_VERIFY_NULLDUMMY;
    }

    // flags = (fStrictPayToScriptHash ? SCRIPT_VERIFY_P2SH : SCRIPT_VERIFY_NONE) | SCRIPT_VERIFY_CHECKLOCKTIMEVERIFY... ]

    int64_t nTime2 = GetTimeMicros(); nTimeForks += nTime2 - nTime1;
    LogPrint("bench", "    - Fork checks: %.2fms [%.2fs]\n", 0.001 * (nTime2 - nTime1), nTimeForks * 0.000001);

    // check vtx [

    CBlockUndo blockundo;

    CCheckQueueControl<CScriptCheck> control(fScriptChecks && nScriptCheckThreads ? &scriptcheckqueue : NULL);

    std::vector<int> prevheights;
    CAmount nFees = 0;
    int nInputs = 0;
    unsigned int nSigOps = 0;
    CDiskTxPos pos(pindex->GetBlockPos(), GetSizeOfCompactSize(block.vtx.size()));
    std::vector<std::pair<uint256, CDiskTxPos> > vPos;
    vPos.reserve(block.vtx.size());
    blockundo.vtxundo.reserve(block.vtx.size() - 1);
    std::vector<std::pair<CAddressIndexKey, CAmount> > addressIndex;
    std::vector<std::pair<CAddressUnspentKey, CAddressUnspentValue> > addressUnspentIndex;
    std::vector<std::pair<CSpentIndexKey, CSpentIndexValue> > spentIndex;

    // fDIP0001Active_context [

    bool fDIP0001Active_context = pindex->nHeight >= Params().GetConsensus().DIP0001Height;

    // fDIP0001Active_context ]

    for (unsigned int i = 0; i < block.vtx.size(); i++)
    {
        const CTransaction &tx = *(block.vtx[i]);
        const uint256 txhash = tx.GetHash();

        nInputs += tx.vin.size();
        nSigOps += GetLegacySigOpCount(tx);
        if (nSigOps > MaxBlockSigOps(fDIP0001Active_context))
            return state.DoS(100, error("ConnectBlock(): too many sigops"),
                             REJECT_INVALID, "bad-blk-sigops");

        if (!tx.IsCoinBase())
        {
            if (!view.HaveInputs(tx))
                return state.DoS(100, error("ConnectBlock(): inputs missing/spent"),
                                 REJECT_INVALID, "bad-txns-inputs-missingorspent");

            // Check that transaction is BIP68 final
            // BIP68 lock checks (as opposed to nLockTime checks) must
            // be in ConnectBlock because they require the UTXO set
            prevheights.resize(tx.vin.size());
            for (size_t j = 0; j < tx.vin.size(); j++) {
                prevheights[j] = view.AccessCoin(tx.vin[j].prevout).nHeight;
            }

            if (!SequenceLocks(tx, nLockTimeFlags, &prevheights, *pindex)) {
                return state.DoS(100, error("%s: contains a non-BIP68-final transaction", __func__),
                                 REJECT_INVALID, "bad-txns-nonfinal");
            }

            if (fAddressIndex || fSpentIndex)
            {
                for (size_t j = 0; j < tx.vin.size(); j++) {
                    const CTxIn input = tx.vin[j];
                    const Coin& coin = view.AccessCoin(tx.vin[j].prevout);
                    const CTxOut &prevout = coin.out;
                    uint160 hashBytes;
                    int addressType;

                    if (prevout.scriptPubKey.IsPayToScriptHash()) {
                        hashBytes = uint160(std::vector<unsigned char>(prevout.scriptPubKey.begin()+2, prevout.scriptPubKey.begin()+22));
                        addressType = 2;
                    } else if (prevout.scriptPubKey.IsPayToPublicKeyHash()) {
                        hashBytes = uint160(std::vector<unsigned char>(prevout.scriptPubKey.begin()+3, prevout.scriptPubKey.begin()+23));
                        addressType = 1;
                    } else if (prevout.scriptPubKey.IsPayToPublicKey()) {
                        hashBytes = Hash160(prevout.scriptPubKey.begin()+1, prevout.scriptPubKey.end()-1);
                        addressType = 1;
                    } else {
                        hashBytes.SetNull();
                        addressType = 0;
                    }

                    if (fAddressIndex && addressType > 0) {
                        // record spending activity
                        addressIndex.push_back(std::make_pair(CAddressIndexKey(addressType, hashBytes, pindex->nHeight, i, txhash, j, true), prevout.nValue * -1));

                        // remove address from unspent index
                        addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(addressType, hashBytes, input.prevout.hash, input.prevout.n), CAddressUnspentValue()));
                    }

                    if (fSpentIndex) {
                        // add the spent index to determine the txid and input that spent an output
                        // and to find the amount and address from an input
                        spentIndex.push_back(std::make_pair(CSpentIndexKey(input.prevout.hash, input.prevout.n), CSpentIndexValue(txhash, j, pindex->nHeight, prevout.nValue, addressType, hashBytes)));
                    }
                }

            }

            if (fStrictPayToScriptHash)
            {
                // Add in sigops done by pay-to-script-hash inputs;
                // this is to prevent a "rogue miner" from creating
                // an incredibly-expensive-to-validate block.
                nSigOps += GetP2SHSigOpCount(tx, view);
                if (nSigOps > MaxBlockSigOps(fDIP0001Active_context))
                    return state.DoS(100, error("ConnectBlock(): too many sigops"),
                                     REJECT_INVALID, "bad-blk-sigops");
            }

            nFees += view.GetValueIn(tx)-tx.GetValueOut();

            std::vector<CScriptCheck> vChecks;
            bool fCacheResults = fJustCheck; /* Don't cache results if we're actually connecting blocks (still consult the cache, though) */

            //x l.14 llog tx CheckInputs [
/*
            if (pindex->nHeight == 2036) {
                llogLog(L"ConnectBlock/tx", L"nHeight", pindex->nHeight);
                llogLog(L"ConnectBlock/tx", L"", block);
            }
*/
            //x l.14 llog tx CheckInputs ]

            //x b.4 - fix diff dash D.1 disable script checks old block height 260+ thegcc-old [

            //    fScriptChecks &= pindex->nHeight >= Checkpoints::GetTotalBlocksEstimate();

            //x b.4 - fix diff dash D.1 disable script checks old block height 260+ thegcc-old ]
            // X.1 !!! disable signature check - fScriptChecks=false !!! BUG !!! [

//            fScriptChecks = false;
            fScriptChecks &= block.IsProofOfStake();

            // X.1 !!! disable signature check - fScriptChecks=false !!! BUG !!! ]

            if (!CheckInputs(tx, state, view, fScriptChecks, flags, fCacheResults, nScriptCheckThreads ? &vChecks : NULL))
                return error("ConnectBlock(): CheckInputs on %s failed with %s",
                             tx.GetHash().ToString(), FormatStateMessage(state));
            control.Add(vChecks);
        }

        if (fAddressIndex) {
            for (unsigned int k = 0; k < tx.vout.size(); k++) {
                const CTxOut &out = tx.vout[k];

                if (out.scriptPubKey.IsPayToScriptHash()) {
                    std::vector<unsigned char> hashBytes(out.scriptPubKey.begin()+2, out.scriptPubKey.begin()+22);

                    // record receiving activity
                    addressIndex.push_back(std::make_pair(CAddressIndexKey(2, uint160(hashBytes), pindex->nHeight, i, txhash, k, false), out.nValue));

                    // record unspent output
                    addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(2, uint160(hashBytes), txhash, k), CAddressUnspentValue(out.nValue, out.scriptPubKey, pindex->nHeight)));

                } else if (out.scriptPubKey.IsPayToPublicKeyHash()) {
                    std::vector<unsigned char> hashBytes(out.scriptPubKey.begin()+3, out.scriptPubKey.begin()+23);

                    // record receiving activity
                    addressIndex.push_back(std::make_pair(CAddressIndexKey(1, uint160(hashBytes), pindex->nHeight, i, txhash, k, false), out.nValue));

                    // record unspent output
                    addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(1, uint160(hashBytes), txhash, k), CAddressUnspentValue(out.nValue, out.scriptPubKey, pindex->nHeight)));

                } else if (out.scriptPubKey.IsPayToPublicKey()) {
                    uint160 hashBytes(Hash160(out.scriptPubKey.begin()+1, out.scriptPubKey.end()-1));
                    addressIndex.push_back(std::make_pair(CAddressIndexKey(1, hashBytes, pindex->nHeight, i, txhash, k, false), out.nValue));
                    addressUnspentIndex.push_back(std::make_pair(CAddressUnspentKey(1, hashBytes, txhash, k), CAddressUnspentValue(out.nValue, out.scriptPubKey, pindex->nHeight)));
                } else {
                    continue;
                }

            }
        }

        CTxUndo undoDummy;
        if (i > 0) {
            blockundo.vtxundo.push_back(CTxUndo());
        }
        UpdateCoins(tx, view, i == 0 ? undoDummy : blockundo.vtxundo.back(), pindex->nHeight);

        vPos.push_back(std::make_pair(tx.GetHash(), pos));
        pos.nTxOffset += ::GetSerializeSize(tx, SER_DISK, CLIENT_VERSION);
    }
    int64_t nTime3 = GetTimeMicros(); nTimeConnect += nTime3 - nTime2;
    LogPrint("bench", "      - Connect %u transactions: %.2fms (%.3fms/tx, %.3fms/txin) [%.2fs]\n", (unsigned)block.vtx.size(), 0.001 * (nTime3 - nTime2), 0.001 * (nTime3 - nTime2) / block.vtx.size(), nInputs <= 1 ? 0 : 0.001 * (nTime3 - nTime2) / (nInputs-1), nTimeConnect * 0.000001);

    // check vtx ]
    // d.3 dash disable IsBlockValueValid MASTERNODE PAYMENTS AND SUPERBLOCKS [
    /*
    // TheGCCcoin : MODIFIED TO CHECK MASTERNODE PAYMENTS AND SUPERBLOCKS

    // It's possible that we simply don't have enough data and this could fail
    // (i.e. block itself could be a correct one and we need to store it),
    // that's why this is in ConnectBlock. Could be the other way around however -
    // the peer who sent us this block is missing some data and wasn't able
    // to recognize that block is actually invalid.
    // TODO: resync data (both ways?) and try to reprocess this block later.
    CAmount blockReward = nFees + GetBlockSubsidy(pindex->pprev->nBits, pindex->pprev->nHeight, chainparams.GetConsensus());
    std::string strError = "";
    if (!IsBlockValueValid(block, pindex->nHeight, blockReward, strError)) {
        return state.DoS(0, error("ConnectBlock(TheGCCcoin): %s", strError), REJECT_INVALID, "bad-cb-amount");
    }
    */
    // d.3 dash disable IsBlockValueValid MASTERNODE PAYMENTS AND SUPERBLOCKS ]
    // d.4 dash disable IsBlockPayeeValid [
    /*
    if (!IsBlockPayeeValid(*block.vtx[0], pindex->nHeight, blockReward)) {
        mapRejectedBlocks.insert(std::make_pair(block.GetHash(), GetTime()));
        return state.DoS(0, error("ConnectBlock(TheGCCcoin): couldn't find masternode or superblock payments"),
                                REJECT_INVALID, "bad-cb-payee");
    }
    */

    // END TheGCCcoin
    // d.4 dash disable IsBlockPayeeValid ]

    // control.Wait() || fJustCheck -> return true [

    if (!control.Wait())
        return state.DoS(100, false);
    int64_t nTime4 = GetTimeMicros(); nTimeVerify += nTime4 - nTime2;
    LogPrint("bench", "    - Verify %u txins: %.2fms (%.3fms/txin) [%.2fs]\n", nInputs - 1, 0.001 * (nTime4 - nTime2), nInputs <= 1 ? 0 : 0.001 * (nTime4 - nTime2) / (nInputs-1), nTimeVerify * 0.000001);

    if (fJustCheck)
        return true;

    // control.Wait() || fJustCheck -> return true ]
    // assert: Write undo information to disk [

    // Write undo information to disk
    if (pindex->GetUndoPos().IsNull() || !pindex->IsValid(BLOCK_VALID_SCRIPTS))
    {
        if (pindex->GetUndoPos().IsNull()) {
            CDiskBlockPos _pos;
            if (!FindUndoPos(state, pindex->nFile, _pos, ::GetSerializeSize(blockundo, SER_DISK, CLIENT_VERSION) + 40))
                return error("ConnectBlock(): FindUndoPos failed");
            if (!UndoWriteToDisk(blockundo, _pos, pindex->pprev->GetBlockHash(), chainparams.MessageStart()))
                return AbortNode(state, "Failed to write undo data");

            // update nUndoPos in block index
            pindex->nUndoPos = _pos.nPos;
            pindex->nStatus |= BLOCK_HAVE_UNDO;
        }

        pindex->RaiseValidity(BLOCK_VALID_SCRIPTS);
        setDirtyBlockIndex.insert(pindex);
    }

    if (fTxIndex)
        if (!pblocktree->WriteTxIndex(vPos))
            return AbortNode(state, "Failed to write transaction index");

    if (fAddressIndex) {
        if (!pblocktree->WriteAddressIndex(addressIndex)) {
            return AbortNode(state, "Failed to write address index");
        }

        if (!pblocktree->UpdateAddressUnspentIndex(addressUnspentIndex)) {
            return AbortNode(state, "Failed to write address unspent index");
        }
    }

    if (fSpentIndex)
        if (!pblocktree->UpdateSpentIndex(spentIndex))
            return AbortNode(state, "Failed to write transaction index");

    if (fTimestampIndex)
        if (!pblocktree->WriteTimestampIndex(CTimestampIndexKey(pindex->nTime, pindex->GetBlockHash())))
            return AbortNode(state, "Failed to write timestamp index");

    // assert: Write undo information to disk ]
    // add this block to the view's block chain [

    // add this block to the view's block chain
    view.SetBestBlock(pindex->GetBlockHash());

    int64_t nTime5 = GetTimeMicros(); nTimeIndex += nTime5 - nTime4;
    LogPrint("bench", "    - Index writing: %.2fms [%.2fs]\n", 0.001 * (nTime5 - nTime4), nTimeIndex * 0.000001);

    // add this block to the view's block chain ]
    // Watch for changes to the previous coinbase transaction [

    // Watch for changes to the previous coinbase transaction.
    static uint256 hashPrevBestCoinBase;
    GetMainSignals().UpdatedTransaction(hashPrevBestCoinBase);
    hashPrevBestCoinBase = block.vtx[0]->GetHash();


    int64_t nTime6 = GetTimeMicros(); nTimeCallbacks += nTime6 - nTime5;
    LogPrint("bench", "    - Callbacks: %.2fms [%.2fs]\n", 0.001 * (nTime6 - nTime5), nTimeCallbacks * 0.000001);

    // Watch for changes to the previous coinbase transaction ]

    return true;
}

// ConnectBlock - *refactor ]
// dash ]