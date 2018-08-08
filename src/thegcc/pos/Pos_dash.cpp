// dash [
// p.5.1.1 dash GetBlocksToMaturity - no [
#if 0
int CMerkleTx::GetBlocksToMaturity() const
{
    if (!IsCoinBase())
        return 0;
    return std::max(0, (COINBASE_MATURITY+1) - GetDepthInMainChain());
}
#endif
// p.5.1.1 dash GetBlocksToMaturity - no ]
// p.5.2.1 dash GetDepthInMainChain - yes [

int CMerkleTx::GetDepthInMainChain(const CBlockIndex* &pindexRet, bool enableIX) const
{
    int nResult;

    if (hashUnset())
        nResult = 0;
    else {
        AssertLockHeld(cs_main);

        // Find the block it claims to be in
        BlockMap::iterator mi = mapBlockIndex.find(hashBlock);
        if (mi == mapBlockIndex.end())
            nResult = 0;
        else {
            CBlockIndex* pindex = (*mi).second;
            if (!pindex || !chainActive.Contains(pindex))
                nResult = 0;
            else {
                pindexRet = pindex;
                nResult = ((nIndex == -1) ? (-1) : 1) * (chainActive.Height() - pindex->nHeight + 1);

                if (nResult == 0 && !mempool.exists(GetHash()))
                    return -1; // Not in chain, not in mempool
            }
        }
    }

    if(enableIX && nResult < 6 && instantsend.IsLockedInstantSendTransaction(GetHash()))
        return nInstantSendDepth + nResult;

    return nResult;
}

// p.5.2.1 dash GetDepthInMainChain - yes ]
// dash ]
