// Copyright (c) 2017-2018 The PIVX developers
// Copyright (c) 2018 TheGCCcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef THEGCC_STAKEINPUT_H
#define THEGCC_STAKEINPUT_H

#include <string>
#include <vector>

#include "amount.h"
#include "primitives/transaction.h"

#include "uint256.h"


class CKeyStore;
class CWallet;
class CWalletTx;
class CBlockIndex;
class CTxIn;
class CTransaction;
class CTxOut;
class CDataStream;

class CStakeInput
{
protected:
    CBlockIndex* pindexFrom;

public:
    virtual ~CStakeInput(){};
    virtual CBlockIndex* GetIndexFrom() = 0;
    virtual bool CreateTxIn(CWallet* pwallet, CTxIn& txIn, uint256 hashTxOut = uint256()) = 0;
    virtual bool GetTxFrom(CTransactionRef& tx) = 0;
    virtual CAmount GetValue() = 0;
    virtual bool CreateTxOuts(CWallet* pwallet, std::vector<CTxOut>& vout, CAmount nTotal) = 0;
    virtual bool GetModifier(uint64_t& nStakeModifier) = 0;
    virtual CDataStream GetUniqueness() = 0;
};

class CTheGCCStake : public CStakeInput
{
private:
    CMutableTransaction txFrom;
    unsigned int nPosition;
public:
    CTheGCCStake()
    {
        this->pindexFrom = nullptr;
    }

    bool SetInput(CTransaction txPrev, unsigned int n);

    CBlockIndex* GetIndexFrom() override;
    bool GetTxFrom(CTransactionRef& tx) override;
    CAmount GetValue() override;
    bool GetModifier(uint64_t& nStakeModifier) override;
    CDataStream GetUniqueness() override;
    bool CreateTxIn(CWallet* pwallet, CTxIn& txIn, uint256 hashTxOut = uint256()) override;
    bool CreateTxOuts(CWallet* pwallet, std::vector<CTxOut>& vout, CAmount nTotal) override;
};

int64_t GetStakeModifierSelectionInterval();
int64_t GetStakeModifierSelectionIntervalSection(int nSection);

#endif //THEGCC_STAKEINPUT_H
