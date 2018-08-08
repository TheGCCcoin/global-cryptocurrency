// @ [

#include "thegcc/imports.h"

// @ ]
// pivx [
// CheckBlockHeader - ok [

bool CheckBlockHeader(const CBlockHeader& block, CValidationState& state, bool fCheckPOW)
{
    // Check proof of work matches claimed amount
    if (fCheckPOW && !CheckProofOfWork(block.GetHash(), block.nBits))
        return state.DoS(50, error("CheckBlockHeader() : proof of work failed"),
                         REJECT_INVALID, "high-hash");
/*
    // Version 4 header must be used after Params().Zerocoin_StartHeight(). And never before.
    if (block.GetBlockTime() > Params().Zerocoin_StartTime()) {
        if(block.nVersion < Params().Zerocoin_HeaderVersion())
            return state.DoS(50, error("CheckBlockHeader() : block version must be above 4 after ZerocoinStartHeight"),
                             REJECT_INVALID, "block-version");
    } else {
        if (block.nVersion >= Params().Zerocoin_HeaderVersion())
            return state.DoS(50, error("CheckBlockHeader() : block version must be below 4 before ZerocoinStartHeight"),
                             REJECT_INVALID, "block-version");
    }
*/
    return true;
}

// CheckBlockHeader - ok ]
// CheckBlock [

bool CheckBlock(const CBlock& block, CValidationState& state, bool fCheckPOW, bool fCheckMerkleRoot, bool fCheckSig)
{
    // These are checks that are independent of context.

    // check pow - CheckBlockHeader [

    // Check that the header is valid (particularly PoW).  This is mostly
    // redundant with the call in AcceptBlockHeader.
    if (!CheckBlockHeader(block, state, block.IsProofOfWork()))
        return state.DoS(100, error("CheckBlock() : CheckBlockHeader failed"),
                         REJECT_INVALID, "bad-header", true);

    // check pow - CheckBlockHeader ]
    // pivx: check timestamp pow/pos [

    // Check timestamp
    LogPrint("debug", "%s: block=%s  is proof of stake=%d\n", __func__, block.GetHash().ToString().c_str(), block.IsProofOfStake());
    if (block.GetBlockTime() > GetAdjustedTime() + (block.IsProofOfStake() ? 180 : 7200)) // 3 minute future drift for PoS
        return state.Invalid(error("CheckBlock() : block timestamp too far in the future"),
                             REJECT_INVALID, "time-too-new");

    // pivx: check timestamp pow/pos ]
    // merkle root [

    // Check the merkle root.
    if (fCheckMerkleRoot) {
        bool mutated;
        uint256 hashMerkleRoot2 = block.BuildMerkleTree(&mutated);
        if (block.hashMerkleRoot != hashMerkleRoot2)
            return state.DoS(100, error("CheckBlock() : hashMerkleRoot mismatch"),
                             REJECT_INVALID, "bad-txnmrklroot", true);

        // Check for merkle tree malleability (CVE-2012-2459): repeating sequences
        // of transactions in a block without affecting the merkle root of a block,
        // while still invalidating it.
        if (mutated)
            return state.DoS(100, error("CheckBlock() : duplicate transaction"),
                             REJECT_INVALID, "bad-txns-duplicate", true);
    }

    // merkle root ]
    // size limits [

    // All potential-corruption validation must be done before we do any
    // transaction validation, as otherwise we may mark the header as invalid
    // because we receive the wrong transactions for it.

    // Size limits
    unsigned int nMaxBlockSize = MAX_BLOCK_SIZE_CURRENT;
    if (block.vtx.empty() || block.vtx.size() > nMaxBlockSize || ::GetSerializeSize(block, SER_NETWORK, PROTOCOL_VERSION) > nMaxBlockSize)
        return state.DoS(100, error("CheckBlock() : size limits failed"),
                         REJECT_INVALID, "bad-blk-length");

    // size limits ]
    // check coinbase transaction [

    // First transaction must be coinbase, the rest must not be
    if (block.vtx.empty() || !block.vtx[0].IsCoinBase())
        return state.DoS(100, error("CheckBlock() : first tx is not coinbase"),
                         REJECT_INVALID, "bad-cb-missing");
    for (unsigned int i = 1; i < block.vtx.size(); i++)
        if (block.vtx[i].IsCoinBase())
            return state.DoS(100, error("CheckBlock() : more than one coinbase"),
                             REJECT_INVALID, "bad-cb-multiple");

    // check coinbase transaction ]
    // pivx: pos/coinbase [

    if (block.IsProofOfStake()) {
        // Coinbase output should be empty if proof-of-stake block
        if (block.vtx[0].vout.size() != 1 || !block.vtx[0].vout[0].IsEmpty())
            return state.DoS(100, error("CheckBlock() : coinbase output not empty for proof-of-stake block"));

        // Second transaction must be coinstake, the rest must not be
        if (block.vtx.empty() || !block.vtx[1].IsCoinStake())
            return state.DoS(100, error("CheckBlock() : second tx is not coinstake"));
        for (unsigned int i = 2; i < block.vtx.size(); i++)
            if (block.vtx[i].IsCoinStake())
                return state.DoS(100, error("CheckBlock() : more than one coinstake"));
    }

    // pivx: pos/coinbase ]
    // pivx: SPORK_3_SWIFTTX_BLOCK_FILTERING [

    // ----------- swiftTX transaction scanning -----------
    if (IsSporkActive(SPORK_3_SWIFTTX_BLOCK_FILTERING)) {
        BOOST_FOREACH (const CTransaction& tx, block.vtx) {
            if (!tx.IsCoinBase()) {
                //only reject blocks when it's based on complete consensus
                BOOST_FOREACH (const CTxIn& in, tx.vin) {
                    if (mapLockedInputs.count(in.prevout)) {
                        if (mapLockedInputs[in.prevout] != tx.GetHash()) {
                            mapRejectedBlocks.insert(make_pair(block.GetHash(), GetTime()));
                            LogPrintf("CheckBlock() : found conflicting transaction with transaction lock %s %s\n", mapLockedInputs[in.prevout].ToString(), tx.GetHash().ToString());
                            return state.DoS(0, error("CheckBlock() : found conflicting transaction with transaction lock"),
                                             REJECT_INVALID, "conflicting-tx-ix");
                        }
                    }
                }
            }
        }
    } else {
        LogPrintf("CheckBlock() : skipping transaction locking checks\n");
    }

    // pivx: SPORK_3_SWIFTTX_BLOCK_FILTERING ]
    // pivx: check masternode payments / budgets [

    // masternode payments / budgets
    CBlockIndex* pindexPrev = chainActive.Tip();
    int nHeight = 0;
    if (pindexPrev != NULL) {
        if (pindexPrev->GetBlockHash() == block.hashPrevBlock) {
            nHeight = pindexPrev->nHeight + 1;
        } else { //out of order
            BlockMap::iterator mi = mapBlockIndex.find(block.hashPrevBlock);
            if (mi != mapBlockIndex.end() && (*mi).second)
                nHeight = (*mi).second->nHeight + 1;
        }

        // PIVX
        // It is entierly possible that we don't have enough data and this could fail
        // (i.e. the block could indeed be valid). Store the block for later consideration
        // but issue an initial reject message.
        // The case also exists that the sending peer could not have enough data to see
        // that this block is invalid, so don't issue an outright ban.
        if (nHeight != 0 && !IsInitialBlockDownload()) {
            if (!IsBlockPayeeValid(block, nHeight)) {
                mapRejectedBlocks.insert(make_pair(block.GetHash(), GetTime()));
                return state.DoS(0, error("CheckBlock() : Couldn't find masternode/budget payment"),
                                 REJECT_INVALID, "bad-cb-payee");
            }
        } else {
            if (fDebug)
                LogPrintf("CheckBlock(): Masternode payment check skipped on sync - skipping IsBlockPayeeValid()\n");
        }
    }

    // pivx: check masternode payments / budgets ]
    // check transactions [

    // Check transactions
    bool fZerocoinActive = block.GetBlockTime() > Params().Zerocoin_StartTime();
    vector<CBigNum> vBlockSerials;
    for (const CTransaction& tx : block.vtx) {
        if (!CheckTransaction(tx, fZerocoinActive, chainActive.Height() + 1 >= Params().Zerocoin_Block_EnforceSerialRange(), state))
            return error("CheckBlock() : CheckTransaction failed");

        // double check that there are no double spent zPIV spends in this block
        if (tx.IsZerocoinSpend()) {
            for (const CTxIn txIn : tx.vin) {
                if (txIn.scriptSig.IsZerocoinSpend()) {
                    libzerocoin::CoinSpend spend = TxInToZerocoinSpend(txIn);
                    if (count(vBlockSerials.begin(), vBlockSerials.end(), spend.getCoinSerialNumber()))
                        return state.DoS(100, error("%s : Double spending of zPIV serial %s in block\n Block: %s",
                                                    __func__, spend.getCoinSerialNumber().GetHex(), block.ToString()));
                    vBlockSerials.emplace_back(spend.getCoinSerialNumber());
                }
            }
        }
    }

    unsigned int nSigOps = 0;
    BOOST_FOREACH (const CTransaction& tx, block.vtx) {
        nSigOps += GetLegacySigOpCount(tx);
    }
    unsigned int nMaxBlockSigOps = fZerocoinActive ? MAX_BLOCK_SIGOPS_CURRENT : MAX_BLOCK_SIGOPS_LEGACY;
    if (nSigOps > nMaxBlockSigOps)
        return state.DoS(100, error("CheckBlock() : out-of-bounds SigOpCount"),
                         REJECT_INVALID, "bad-blk-sigops", true);

    // check transactions ]

    return true;
}

// CheckBlock ]
// ConnectBlock [

bool ConnectBlock(const CBlock& block, CValidationState& state, CBlockIndex* pindex, CCoinsViewCache& view, bool fJustCheck, bool fAlreadyChecked)
{
    AssertLockHeld(cs_main);
    // Check it again in case a previous version let a bad block in
    if (!fAlreadyChecked && !CheckBlock(block, state, !fJustCheck, !fJustCheck))
        return false;

    // verify that the view's current state corresponds to the previous block
    uint256 hashPrevBlock = pindex->pprev == NULL ? uint256(0) : pindex->pprev->GetBlockHash();
    if (hashPrevBlock != view.GetBestBlock())
        LogPrintf("%s: hashPrev=%s view=%s\n", __func__, hashPrevBlock.ToString().c_str(), view.GetBestBlock().ToString().c_str());
    assert(hashPrevBlock == view.GetBestBlock());

    // Special case for the genesis block, skipping connection of its transactions
    // (its coinbase is unspendable)
    if (block.GetHash() == Params().HashGenesisBlock()) {
        view.SetBestBlock(pindex->GetBlockHash());
        return true;
    }

    if (pindex->nHeight <= Params().LAST_POW_BLOCK() && block.IsProofOfStake())
        return state.DoS(100, error("ConnectBlock() : PoS period not active"),
                         REJECT_INVALID, "PoS-early");

    if (pindex->nHeight > Params().LAST_POW_BLOCK() && block.IsProofOfWork())
        return state.DoS(100, error("ConnectBlock() : PoW period ended"),
                         REJECT_INVALID, "PoW-ended");

    // D.1.pivx fScriptChecks = pindex->nHeight >= Checkpoints::GetTotalBlocksEstimate() - diff dash/pivx/oldgcc [

    bool fScriptChecks = pindex->nHeight >= Checkpoints::GetTotalBlocksEstimate();

    // D.1.pivx fScriptChecks = pindex->nHeight >= Checkpoints::GetTotalBlocksEstimate() - diff dash/pivx/oldgcc ]

    // Do not allow blocks that contain transactions which 'overwrite' older transactions,
    // unless those are already completely spent.
    // If such overwrites are allowed, coinbases and transactions depending upon those
    // can be duplicated to remove the ability to spend the first instance -- even after
    // being sent to another address.
    // See BIP30 and http://r6.ca/blog/20120206T005236Z.html for more information.
    // This logic is not necessary for memory pool transactions, as AcceptToMemoryPool
    // already refuses previously-known transaction ids entirely.
    // This rule was originally applied all blocks whose timestamp was after March 15, 2012, 0:00 UTC.
    // Now that the whole chain is irreversibly beyond that time it is applied to all blocks except the
    // two in the chain that violate it. This prevents exploiting the issue against nodes in their
    // initial block download.
    bool fEnforceBIP30 = (!pindex->phashBlock) || // Enforce on CreateNewBlock invocations which don't have a hash.
                         !((pindex->nHeight == 91842 && pindex->GetBlockHash() == uint256("0x00000000000a4d0a398161ffc163c503763b1f4360639393e0e4c8e300e0caec")) ||
                           (pindex->nHeight == 91880 && pindex->GetBlockHash() == uint256("0x00000000000743f190a18c5577a3c2d2a1f610ae9601ac046a38084ccb7cd721")));
    if (fEnforceBIP30) {
        BOOST_FOREACH (const CTransaction& tx, block.vtx) {
            const CCoins* coins = view.AccessCoins(tx.GetHash());
            if (coins && !coins->IsPruned())
                return state.DoS(100, error("ConnectBlock() : tried to overwrite transaction"),
                                 REJECT_INVALID, "bad-txns-BIP30");
        }
    }

    CCheckQueueControl<CScriptCheck> control(fScriptChecks && nScriptCheckThreads ? &scriptcheckqueue : NULL);

    int64_t nTimeStart = GetTimeMicros();
    CAmount nFees = 0;
    int nInputs = 0;
    unsigned int nSigOps = 0;
    CDiskTxPos pos(pindex->GetBlockPos(), GetSizeOfCompactSize(block.vtx.size()));
    std::vector<std::pair<uint256, CDiskTxPos> > vPos;
    std::vector<pair<CoinSpend, uint256> > vSpends;
    vector<pair<PublicCoin, uint256> > vMints;
    vPos.reserve(block.vtx.size());
    CBlockUndo blockundo;
    blockundo.vtxundo.reserve(block.vtx.size() - 1);
    CAmount nValueOut = 0;
    CAmount nValueIn = 0;
    unsigned int nMaxBlockSigOps = MAX_BLOCK_SIGOPS_CURRENT;
    vector<uint256> vSpendsInBlock;
    uint256 hashBlock = block.GetHash();
    for (unsigned int i = 0; i < block.vtx.size(); i++) {
        const CTransaction& tx = block.vtx[i];

        nInputs += tx.vin.size();
        nSigOps += GetLegacySigOpCount(tx);
        if (nSigOps > nMaxBlockSigOps)
            return state.DoS(100, error("ConnectBlock() : too many sigops"), REJECT_INVALID, "bad-blk-sigops");

        //Temporarily disable zerocoin transactions for maintenance
        if (block.nTime > GetSporkValue(SPORK_16_ZEROCOIN_MAINTENANCE_MODE) && !IsInitialBlockDownload() && tx.ContainsZerocoins()) {
            return state.DoS(100, error("ConnectBlock() : zerocoin transactions are currently in maintenance mode"));
        }

        if (tx.IsZerocoinSpend()) {
            int nHeightTx = 0;
            uint256 txid = tx.GetHash();
            vSpendsInBlock.emplace_back(txid);
            if (IsTransactionInChain(txid, nHeightTx)) {
                //when verifying blocks on init, the blocks are scanned without being disconnected - prevent that from causing an error
                if (!fVerifyingBlocks || (fVerifyingBlocks && pindex->nHeight > nHeightTx))
                    return state.DoS(100, error("%s : txid %s already exists in block %d , trying to include it again in block %d", __func__,
                                                tx.GetHash().GetHex(), nHeightTx, pindex->nHeight),
                                     REJECT_INVALID, "bad-txns-inputs-missingorspent");
            }

            //Check for double spending of serial #'s
            set<CBigNum> setSerials;
            for (const CTxIn& txIn : tx.vin) {
                if (!txIn.scriptSig.IsZerocoinSpend())
                    continue;
                CoinSpend spend = TxInToZerocoinSpend(txIn);
                nValueIn += spend.getDenomination() * COIN;

                //queue for db write after the 'justcheck' section has concluded
                vSpends.emplace_back(make_pair(spend, tx.GetHash()));
                if (!ContextualCheckZerocoinSpend(tx, spend, pindex, hashBlock))
                    return state.DoS(100, error("%s: failed to add block %s with invalid zerocoinspend", __func__, tx.GetHash().GetHex()), REJECT_INVALID);
            }

            // Check that zPIV mints are not already known
            if (tx.IsZerocoinMint()) {
                for (auto& out : tx.vout) {
                    if (!out.IsZerocoinMint())
                        continue;

                    PublicCoin coin(Params().Zerocoin_Params(false));
                    if (!TxOutToPublicCoin(out, coin, state))
                        return state.DoS(100, error("%s: failed final check of zerocoinmint for tx %s", __func__, tx.GetHash().GetHex()));

                    if (!ContextualCheckZerocoinMint(tx, coin, pindex))
                        return state.DoS(100, error("%s: zerocoin mint failed contextual check", __func__));

                    vMints.emplace_back(make_pair(coin, tx.GetHash()));
                }
            }
        } else if (!tx.IsCoinBase()) {
            if (!view.HaveInputs(tx))
                return state.DoS(100, error("ConnectBlock() : inputs missing/spent"),
                                 REJECT_INVALID, "bad-txns-inputs-missingorspent");

            // Check that the inputs are not marked as invalid/fraudulent
            for (CTxIn in : tx.vin) {
                if (!ValidOutPoint(in.prevout, pindex->nHeight)) {
                    return state.DoS(100, error("%s : tried to spend invalid input %s in tx %s", __func__, in.prevout.ToString(),
                                                tx.GetHash().GetHex()), REJECT_INVALID, "bad-txns-invalid-inputs");
                }
            }

            // Check that zPIV mints are not already known
            if (tx.IsZerocoinMint()) {
                for (auto& out : tx.vout) {
                    if (!out.IsZerocoinMint())
                        continue;

                    PublicCoin coin(Params().Zerocoin_Params(false));
                    if (!TxOutToPublicCoin(out, coin, state))
                        return state.DoS(100, error("%s: failed final check of zerocoinmint for tx %s", __func__, tx.GetHash().GetHex()));

                    if (!ContextualCheckZerocoinMint(tx, coin, pindex))
                        return state.DoS(100, error("%s: zerocoin mint failed contextual check", __func__));

                    vMints.emplace_back(make_pair(coin, tx.GetHash()));
                }
            }

            // Add in sigops done by pay-to-script-hash inputs;
            // this is to prevent a "rogue miner" from creating
            // an incredibly-expensive-to-validate block.
            nSigOps += GetP2SHSigOpCount(tx, view);
            if (nSigOps > nMaxBlockSigOps)
                return state.DoS(100, error("ConnectBlock() : too many sigops"), REJECT_INVALID, "bad-blk-sigops");

            if (!tx.IsCoinStake())
                nFees += view.GetValueIn(tx) - tx.GetValueOut();
            nValueIn += view.GetValueIn(tx);

            std::vector<CScriptCheck> vChecks;
            unsigned int flags = SCRIPT_VERIFY_P2SH | SCRIPT_VERIFY_DERSIG;
            if (!CheckInputs(tx, state, view, fScriptChecks, flags, false, nScriptCheckThreads ? &vChecks : NULL))
                return false;
            control.Add(vChecks);
        }
        nValueOut += tx.GetValueOut();

        CTxUndo undoDummy;
        if (i > 0) {
            blockundo.vtxundo.push_back(CTxUndo());
        }
        UpdateCoins(tx, state, view, i == 0 ? undoDummy : blockundo.vtxundo.back(), pindex->nHeight);

        vPos.push_back(std::make_pair(tx.GetHash(), pos));
        pos.nTxOffset += ::GetSerializeSize(tx, SER_DISK, CLIENT_VERSION);
    }

    //A one-time event where money supply counts were off and recalculated on a certain block.
    if (pindex->nHeight == Params().Zerocoin_Block_RecalculateAccumulators() + 1) {
        RecalculateZPIVMinted();
        RecalculateZPIVSpent();
        RecalculatePIVSupply(Params().Zerocoin_StartHeight());
    }

    //Track zPIV money supply in the block index
    if (!UpdateZPIVSupply(block, pindex))
        return state.DoS(100, error("%s: Failed to calculate new zPIV supply for block=%s height=%d", __func__,
                                    block.GetHash().GetHex(), pindex->nHeight), REJECT_INVALID);

    // track money supply and mint amount info
    CAmount nMoneySupplyPrev = pindex->pprev ? pindex->pprev->nMoneySupply : 0;
    pindex->nMoneySupply = nMoneySupplyPrev + nValueOut - nValueIn;
    pindex->nMint = pindex->nMoneySupply - nMoneySupplyPrev + nFees;

//    LogPrintf("XX69----------> ConnectBlock(): nValueOut: %s, nValueIn: %s, nFees: %s, nMint: %s zPivSpent: %s\n",
//              FormatMoney(nValueOut), FormatMoney(nValueIn),
//              FormatMoney(nFees), FormatMoney(pindex->nMint), FormatMoney(nAmountZerocoinSpent));

    int64_t nTime1 = GetTimeMicros();
    nTimeConnect += nTime1 - nTimeStart;
    LogPrint("bench", "      - Connect %u transactions: %.2fms (%.3fms/tx, %.3fms/txin) [%.2fs]\n", (unsigned)block.vtx.size(), 0.001 * (nTime1 - nTimeStart), 0.001 * (nTime1 - nTimeStart) / block.vtx.size(), nInputs <= 1 ? 0 : 0.001 * (nTime1 - nTimeStart) / (nInputs - 1), nTimeConnect * 0.000001);

    //PoW phase redistributed fees to miner. PoS stage destroys fees.
    CAmount nExpectedMint = GetBlockValue(pindex->pprev->nHeight);
    if (block.IsProofOfWork())
        nExpectedMint += nFees;

    //Check that the block does not overmint
    if (!IsBlockValueValid(block, nExpectedMint, pindex->nMint)) {
        return state.DoS(100, error("ConnectBlock() : reward pays too much (actual=%s vs limit=%s)",
                                    FormatMoney(pindex->nMint), FormatMoney(nExpectedMint)),
                         REJECT_INVALID, "bad-cb-amount");
    }

    // Ensure that accumulator checkpoints are valid and in the same state as this instance of the chain
    AccumulatorMap mapAccumulators(Params().Zerocoin_Params(pindex->nHeight < Params().Zerocoin_Block_V2_Start()));
    if (!ValidateAccumulatorCheckpoint(block, pindex, mapAccumulators))
        return state.DoS(100, error("%s: Failed to validate accumulator checkpoint for block=%s height=%d", __func__,
                                    block.GetHash().GetHex(), pindex->nHeight), REJECT_INVALID, "bad-acc-checkpoint");

    if (!control.Wait())
        return state.DoS(100, false);
    int64_t nTime2 = GetTimeMicros();
    nTimeVerify += nTime2 - nTimeStart;
    LogPrint("bench", "    - Verify %u txins: %.2fms (%.3fms/txin) [%.2fs]\n", nInputs - 1, 0.001 * (nTime2 - nTimeStart), nInputs <= 1 ? 0 : 0.001 * (nTime2 - nTimeStart) / (nInputs - 1), nTimeVerify * 0.000001);

    //IMPORTANT NOTE: Nothing before this point should actually store to disk (or even memory)
    if (fJustCheck)
        return true;

    // Write undo information to disk
    if (pindex->GetUndoPos().IsNull() || !pindex->IsValid(BLOCK_VALID_SCRIPTS)) {
        if (pindex->GetUndoPos().IsNull()) {
            CDiskBlockPos pos;
            if (!FindUndoPos(state, pindex->nFile, pos, ::GetSerializeSize(blockundo, SER_DISK, CLIENT_VERSION) + 40))
                return error("ConnectBlock() : FindUndoPos failed");
            if (!blockundo.WriteToDisk(pos, pindex->pprev->GetBlockHash()))
                return state.Abort("Failed to write undo data");

            // update nUndoPos in block index
            pindex->nUndoPos = pos.nPos;
            pindex->nStatus |= BLOCK_HAVE_UNDO;
        }

        pindex->RaiseValidity(BLOCK_VALID_SCRIPTS);
        setDirtyBlockIndex.insert(pindex);
    }

    //Record zPIV serials
    set<uint256> setAddedTx;
    for (pair<CoinSpend, uint256> pSpend : vSpends) {
        //record spend to database
        if (!zerocoinDB->WriteCoinSpend(pSpend.first.getCoinSerialNumber(), pSpend.second))
            return state.Abort(("Failed to record coin serial to database"));

        // Send signal to wallet if this is ours
        if (pwalletMain) {
            if (pwalletMain->IsMyZerocoinSpend(pSpend.first.getCoinSerialNumber())) {
                LogPrintf("%s: %s detected zerocoinspend in transaction %s \n", __func__, pSpend.first.getCoinSerialNumber().GetHex(), pSpend.second.GetHex());
                pwalletMain->NotifyZerocoinChanged(pwalletMain, pSpend.first.getCoinSerialNumber().GetHex(), "Used", CT_UPDATED);

                //Don't add the same tx multiple times
                if (setAddedTx.count(pSpend.second))
                    continue;

                //Search block for matching tx, turn into wtx, set merkle branch, add to wallet
                for (CTransaction tx : block.vtx) {
                    if (tx.GetHash() == pSpend.second) {
                        CWalletTx wtx(pwalletMain, tx);
                        wtx.nTimeReceived = pindex->GetBlockTime();
                        wtx.SetMerkleBranch(block);
                        pwalletMain->AddToWallet(wtx);
                        setAddedTx.insert(pSpend.second);
                    }
                }
            }
        }
    }

    //Record mints to db
    for (pair<PublicCoin, uint256> pMint : vMints) {
        if (!zerocoinDB->WriteCoinMint(pMint.first, pMint.second))
            return state.Abort(("Failed to record new mint to database"));
    }

    //Record accumulator checksums
    DatabaseChecksums(mapAccumulators);

    if (fTxIndex)
        if (!pblocktree->WriteTxIndex(vPos))
            return state.Abort("Failed to write transaction index");

    // add this block to the view's block chain
    view.SetBestBlock(pindex->GetBlockHash());

    int64_t nTime3 = GetTimeMicros();
    nTimeIndex += nTime3 - nTime2;
    LogPrint("bench", "    - Index writing: %.2fms [%.2fs]\n", 0.001 * (nTime3 - nTime2), nTimeIndex * 0.000001);

    // Watch for changes to the previous coinbase transaction.
    static uint256 hashPrevBestCoinBase;
    GetMainSignals().UpdatedTransaction(hashPrevBestCoinBase);
    hashPrevBestCoinBase = block.vtx[0].GetHash();

    int64_t nTime4 = GetTimeMicros();
    nTimeCallbacks += nTime4 - nTime3;
    LogPrint("bench", "    - Callbacks: %.2fms [%.2fs]\n", 0.001 * (nTime4 - nTime3), nTimeCallbacks * 0.000001);

    //Continue tracking possible movement of fraudulent funds until they are completely frozen
    if (pindex->nHeight >= Params().Zerocoin_Block_FirstFraudulent() && pindex->nHeight <= Params().Zerocoin_Block_RecalculateAccumulators() + 1)
        AddInvalidSpendsToMap(block);

    //Remove zerocoinspends from the pending map
    for (const uint256& txid : vSpendsInBlock) {
        auto it = mapZerocoinspends.find(txid);
        if (it != mapZerocoinspends.end())
            mapZerocoinspends.erase(it);
    }

    return true;
}

// ConnectBlock ]
// pivx ]
