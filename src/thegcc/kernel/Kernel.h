#ifndef THEGCC_KERNEL_H
#define THEGCC_KERNEL_H

#include "arith_uint256.h"

class CTransaction;
class CStakeInput;
class CBlockIndex;

bool CheckProofOfStake(const CTransaction& tx, unsigned int nBits, std::unique_ptr<CStakeInput>& stake,
                       uint256& hashProofOfStake, bool fIsInitialDownload);

bool GetLastStakeModifier(const CBlockIndex* pindex, uint64_t& nStakeModifier, int64_t& nModifierTime);
bool ComputeNextStakeModifier(const CBlockIndex* pindexPrev, uint64_t& nStakeModifier, bool& fGeneratedStakeModifier);
unsigned int GetStakeModifierChecksum(const CBlockIndex* pindex);
bool CheckStakeModifierCheckpoints(int nHeight, unsigned int nStakeModifierChecksum);

bool SelectBlockFromCandidates(
        std::vector<std::pair<int64_t, uint256> >& vSortedByTimestamp,
        std::map<uint256, const CBlockIndex*>& mapSelectedBlocks,
        int64_t nSelectionIntervalStop,
        uint64_t nStakeModifierPrev,
        const CBlockIndex** pindexSelected);


#endif //THEGCC_KERNEL_H
