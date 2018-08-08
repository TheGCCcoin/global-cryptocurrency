// pivx [
// GetDepthInMainChainINTERNAL [

int CMerkleTx::GetDepthInMainChainINTERNAL(const CBlockIndex*& pindexRet) const
{
    if (hashBlock == 0 || nIndex == -1)
        return 0;
    AssertLockHeld(cs_main);

    // Find the block it claims to be in
    BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
    if (mi == mapBlockIndex.end())
        return 0;
    CBlockIndex* pindex = (*mi).second;
    if (!pindex || !chainActive.Contains(pindex))
        return 0;

    // Make sure the merkle branch connects to this block
    if (!fMerkleVerified) {
        if (CBlock::CheckMerkleBranch(GetHash(), vMerkleBranch, nIndex) != pindex->hashMerkleRoot)
            return 0;
        fMerkleVerified = true;
    }

    pindexRet = pindex;
    return chainActive.Height() - pindex->nHeight + 1;
}

// GetDepthInMainChainINTERNAL ]
// p.5.2.2 pivx GetDepthInMainChain [

int CMerkleTx::GetDepthInMainChain(const CBlockIndex*& pindexRet, bool enableIX) const
{
    AssertLockHeld(cs_main);
    int nResult = GetDepthInMainChainINTERNAL(pindexRet);
    if (nResult == 0 && !mempool.exists(GetHash()))
        return -1; // Not in chain, not in mempool

    if (enableIX) {
        if (nResult < 6) {
            int signatures = GetTransactionLockSignatures();
            if (signatures >= SWIFTTX_SIGNATURES_REQUIRED) {
                return nSwiftTXDepth + nResult;
            }
        }
    }

    return nResult;
}

// p.5.2.2 pivx GetDepthInMainChain ]
// p.5.1.2 pivx GetBlocksToMaturity - no, pivx==dash [

int CMerkleTx::GetBlocksToMaturity() const
{
    LOCK(cs_main);
    if (!(IsCoinBase() || IsCoinStake()))
        return 0;
    return max(0, (Params().COINBASE_MATURITY() + 1) - GetDepthInMainChain());
}

// p.5.1.2 pivx GetBlocksToMaturity - no, pivx==dash ]
// pivx ]

