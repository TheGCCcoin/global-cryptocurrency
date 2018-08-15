#ifndef THEGCC_BLOCK_H
#define THEGCC_BLOCK_H

bool AcceptBlock(const std::shared_ptr<const CBlock>& pblock, CValidationState& state, const CChainParams& chainparams, CBlockIndex** ppindex, bool fRequested, const CDiskBlockPos* dbp, bool* fNewBlock);

bool ConnectBlock(const CBlock& block, CValidationState& state, CBlockIndex* pindex,
                  CCoinsViewCache& view, const CChainParams& chainparams, bool fJustCheck = false);

CBlockIndex* AddToBlockIndex(const CBlockHeader& block);

bool ReceivedBlockTransactions(const CBlock &block, CValidationState& state, CBlockIndex *pindexNew, const CDiskBlockPos& pos);

void blockIndexSetDirty(CBlockIndex *index, const wchar_t *name);
void blockIndexUnsetDirty(CBlockIndex *index, const wchar_t *name);
void llog_blockIndexUpdateTip(CBlockIndex *pindexNew, const CChainParams& chainParams);

#endif //THEGCC_BLOCK_H
