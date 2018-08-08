#ifndef THEGCC_BLOCK_H
#define THEGCC_BLOCK_H

bool ConnectBlock(const CBlock& block, CValidationState& state, CBlockIndex* pindex,
                  CCoinsViewCache& view, const CChainParams& chainparams, bool fJustCheck = false);

#endif //THEGCC_BLOCK_H
