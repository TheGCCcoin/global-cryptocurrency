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
    int64 nValueIn = txPrev.vout[prevout.n].nValue;

    // v0.3 protocol kernel hash weight starts from 0 at the min age
    // this change increases active coins participating the hash and helps
    // to secure the network when proof-of-stake difficulty is low
    int64  nTimeWeight = min((int64)nTimeTx - txPrev.nTime, (int64)GetStakeMaxAge(nTimeTx) + GetStakeMinAge(nTimeTx)) - GetStakeMinAge(nTimeTx);
    if (nTimeTx > VERSION2_SWITCH_TIME)
        nTimeWeight = min((int64)nTimeTx - txPrev.nTime - GetStakeMinAge(nTimeTx), (int64)GetStakeMaxAge(nTimeTx));
    else
        nTimeWeight = min((int64)nTimeTx - txPrev.nTime, (int64)GetStakeMaxAge(nTimeTx) + GetStakeMinAge(nTimeTx)) - GetStakeMinAge(nTimeTx);

    CBigNum bnCoinDayWeight = CBigNum(nValueIn) * nTimeWeight / COIN / (24 * 60 * 60);

    // printf(">>> CheckStakeKernelHash: nTimeWeight = %"PRI64d"\n", nTimeWeight);
    // Calculate hash
    CDataStream ss(SER_GETHASH, 0);
    uint64 nStakeModifier = 0;
    int nStakeModifierHeight = 0;
    int64 nStakeModifierTime = 0;

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
        printf("CheckStakeKernelHash() : using modifier 0x%016"PRI64x" at height=%d timestamp=%s for block from height=%d timestamp=%s\n",
               nStakeModifier, nStakeModifierHeight,
               DateTimeStrFormat(nStakeModifierTime).c_str(),
               mapBlockIndex[blockFrom.GetHash()]->nHeight,
               DateTimeStrFormat(blockFrom.GetBlockTime()).c_str());
        printf("CheckStakeKernelHash() : check protocol=%s modifier=0x%016"PRI64x" nTimeBlockFrom=%u nTxPrevOffset=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProof=%s\n",
               "0.3",
               nStakeModifier,
               nTimeBlockFrom, nTxPrevOffset, txPrev.nTime, prevout.n, nTimeTx,
               hashProofOfStake.ToString().c_str());
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


    if (CBigNum(hashProofOfStake) > bnProduct)
    {
        if(fDebug)
        {
            printf(">>> bnCoinDayWeight = %s, bnTargetPerCoinDay=%s\n>>> too small:<bnProduct=%s>\n",
                   bnCoinDayWeight.ToString().c_str(),
                   bnTargetPerCoinDay.ToString().c_str(),
                   bnProduct.ToString().c_str());
            printf(">>> CheckStakeKernelHash - hashProofOfStake too much\n");
            printf(">>> hashProofOfStake too much: %s\n", CBigNum(hashProofOfStake).ToString().c_str());
        }
        return false;
    }


    if (fDebug && !fPrintProofOfStake)
    {
        printf("CheckStakeKernelHash() : using modifier 0x%016"PRI64x" at height=%d timestamp=%s for block from height=%d timestamp=%s\n",
               nStakeModifier, nStakeModifierHeight,
               DateTimeStrFormat(nStakeModifierTime).c_str(),
               mapBlockIndex[blockFrom.GetHash()]->nHeight,
               DateTimeStrFormat(blockFrom.GetBlockTime()).c_str());
        printf("CheckStakeKernelHash() : pass protocol=%s modifier=0x%016"PRI64x" nTimeBlockFrom=%u nTxPrevOffset=%u nTimeTxPrev=%u nPrevout=%u nTimeTx=%u hashProof=%s\n",
               "0.3",
               nStakeModifier,
               nTimeBlockFrom, nTxPrevOffset, txPrev.nTime, prevout.n, nTimeTx,
               hashProofOfStake.ToString().c_str());
    }
    return true;
}

// CheckStakeKernelHash ]
// CheckProofOfStake [

// Check kernel hash target and coinstake signature
bool CheckProofOfStake(const CTransaction& tx, unsigned int nBits, uint256& hashProofOfStake,
                       bool fIsInitialDownload)
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
    CTxDB txdb("r");
    CTransaction txPrev;
    CTxIndex txindex;
    if (!txPrev.ReadFromDisk(txdb, txin.prevout, txindex))
        // previous transaction not in main chain, may occur during initial download
        return tx.DoS(1, error("CheckProofOfStake() : INFO: read txPrev failed"));
    //txdb.Close();

    // Verify signature
    if (!VerifySignature(txPrev, tx, 0, true, 0))
        return tx.DoS(100, error("CheckProofOfStake() : VerifySignature failed on coinstake %s", tx.GetHash().ToString().c_str()));

    // Read block header
    CBlock block;
    if (!block.ReadFromDisk(txindex.pos.nFile, txindex.pos.nBlockPos, false))
        return fDebug? error("CheckProofOfStake() : read block failed") : false; // unable to read block of previous transaction

    if (!CheckStakeKernelHash(nBits, block, txindex.pos.nTxPos - txindex.pos.nBlockPos, txPrev, txin.prevout, tx.nTime, hashProofOfStake, fDebug))
        return tx.DoS(1, error("CheckProofOfStake() : INFO: check kernel failed on coinstake %s, hashProof=%s", tx.GetHash().ToString().c_str(), hashProofOfStake.ToString().c_str())); // may occur during initial download or if behind on block chain sync

    return true;
}

// CheckProofOfStake ]
