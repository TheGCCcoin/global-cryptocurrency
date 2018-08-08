// @ [

#include "thegcc/imports.h"

// @ ]
// thegcc-old [

// b.5.1 COINBASE_MATURITY = oldgcc [

// dash/pivx  = 100
// thegcc-old = 40

int COINBASE_MATURITY = 40;

//oldgcc: int nCoinbaseMaturity = 40;

/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
//pivx: static const int COINBASE_MATURITY = 100;

//int COINBASE_MATURITY = nMaturity;

// b.5.1 COINBASE_MATURITY = oldgcc ]

#include "Pos_dash.cpp"
#include "Pos_thegcc-old.cpp"

// p.3 GetBlockValue - stake coins by height ! fix thegcc [


int64_t GetBlockValue(int nHeight)
{
    int64_t nSubsidy = 0;
#if 0
    if (Params().NetworkID() == CBaseChainParams::TESTNET) {
        if (nHeight < 200 && nHeight > 0)
            return 250000 * COIN;
    }

    int64_t nSubsidy = 0;
    if (nHeight == 0) {
        nSubsidy = 60001 * COIN;
    } else if (nHeight < 86400 && nHeight > 0) {
        nSubsidy = 250 * COIN;
    } else if (nHeight < (Params().NetworkID() == CBaseChainParams::TESTNET ? 145000 : 151200) && nHeight >= 86400) {
        nSubsidy = 225 * COIN;
    } else if (nHeight <= Params().LAST_POW_BLOCK() && nHeight >= 151200) {
        nSubsidy = 45 * COIN;
    } else if (nHeight <= 302399 && nHeight > Params().LAST_POW_BLOCK()) {
        nSubsidy = 45 * COIN;
    } else if (nHeight <= 345599 && nHeight >= 302400) {
        nSubsidy = 40.5 * COIN;
    } else if (nHeight <= 388799 && nHeight >= 345600) {
        nSubsidy = 36 * COIN;
    } else if (nHeight <= 431999 && nHeight >= 388800) {
        nSubsidy = 31.5 * COIN;
    } else if (nHeight <= 475199 && nHeight >= 432000) {
        nSubsidy = 27 * COIN;
    } else if (nHeight <= 518399 && nHeight >= 475200) {
        nSubsidy = 22.5 * COIN;
    } else if (nHeight <= 561599 && nHeight >= 518400) {
        nSubsidy = 18 * COIN;
    } else if (nHeight <= 604799 && nHeight >= 561600) {
        nSubsidy = 13.5 * COIN;
    } else if (nHeight <= 647999 && nHeight >= 604800) {
        nSubsidy = 9 * COIN;
    } else if (nHeight < Params().Zerocoin_Block_V2_Start()) {
        nSubsidy = 4.5 * COIN;
    } else {
        nSubsidy = 5 * COIN;
    }
#endif
    return nSubsidy;
}


// p.3 GetBlockValue - stake coins by height ! fix thegcc ]

#if 0

// p.4.2 AvailableCoins [

// populate vCoins with vector of spendable COutputs
void CWallet::AvailableCoins(vector<COutput>& vCoins, bool fOnlyConfirmed, const CCoinControl *coinControl) const
{
    vCoins.clear();

    {
        LOCK(cs_wallet);
        for (map<uint256, CWalletTx>::const_iterator it = mapWallet.begin(); it != mapWallet.end(); ++it)
        {
            const CWalletTx* pcoin = &(*it).second;

            if (!pcoin->IsFinal())
                continue;

            if (fOnlyConfirmed && !pcoin->IsConfirmed())
                continue;

            if (pcoin->IsCoinBase() && pcoin->GetBlocksToMaturity() > 0)
                continue;

            if(pcoin->IsCoinStake() && pcoin->GetBlocksToMaturity() > 0)
                continue;
            int nDepth = pcoin->GetDepthInMainChain();
            if(nDepth < 0)
                continue;
            for (unsigned int i = 0; i < pcoin->vout.size(); i++)
                if (!(pcoin->IsSpent(i)) && IsMine(pcoin->vout[i]) && pcoin->vout[i].nValue > 0 &&
                 (!coinControl || !coinControl->HasSelected() || coinControl->IsSelected((*it).first, i)))
                    vCoins.push_back(COutput(pcoin, i, nDepth));
        }
    }
}

// p.4.2 AvailableCoins ]

#endif

// thegcc-old ]
