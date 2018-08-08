// Copyright (c) 2017-2018 The PIVX developers
// Copyright (c) 2018 TheGCCcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// i.1 @ - review [

#include "miner.h"

#include "amount.h"
#include "hash.h"
#include "masternode-sync.h"
#include "net.h"
#include "pow.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "timedata.h"
#include "util.h"
#include "utilmoneystr.h"

#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

#include "validationinterface.h"
#include "masternode-payments.h"
#include "spork.h"

#include "consensus/validation.h"

#include "arith_uint256.h"

#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>

#include "blocksignature.h"

// i.1 @ - review ]

// blocksignature [
// vars [

typedef std::vector<unsigned char> valtype;

// vars ]
// SignBlockWithKey [

bool SignBlockWithKey(CBlock& block, const CKey& key)
{
    if (!key.Sign(block.GetHash(), block.vchBlockSig))
        return error("%s: failed to sign block hash with key", __func__);

    return true;
}

// SignBlockWithKey ]
// GetKeyIDFromUTXO [

bool GetKeyIDFromUTXO(const CTxOut& txout, CKeyID& keyID)
{
    std::vector<valtype> vSolutions;
    txnouttype whichType;
    if (!Solver(txout.scriptPubKey, whichType, vSolutions))
        return false;
    if (whichType == TX_PUBKEY) {
        keyID = CPubKey(vSolutions[0]).GetID();
    } else if (whichType == TX_PUBKEYHASH) {
        keyID = CKeyID(uint160(vSolutions[0]));
    }

    return true;
}

// GetKeyIDFromUTXO ]
// SignBlock [

bool SignBlock(CBlock& block, const CKeyStore& keystore)
{
    CKeyID keyID;
    if (block.IsProofOfWork()) {
        bool fFoundID = false;
        for (const CTxOut& txout :block.vtx[0]->vout) {
            if (!GetKeyIDFromUTXO(txout, keyID))
                continue;
            fFoundID = true;
            break;
        }
        if (!fFoundID)
            return error("%s: failed to find key for PoW", __func__);
    } else {
        if (!GetKeyIDFromUTXO(block.vtx[1]->vout[1], keyID))
            return error("%s: failed to find key for PoS", __func__);
    }

    CKey key;
    if (!keystore.GetKey(keyID, key))
        return error("%s: failed to get key from keystore", __func__);

    return SignBlockWithKey(block, key);
}

// SignBlock ]
// pivx: CheckBlockSignature [

bool CheckBlockSignature(const CBlock& block)
{
    if (block.IsProofOfWork())
        return block.vchBlockSig.empty();

    if (block.vchBlockSig.empty())
        return error("%s: vchBlockSig is empty!", __func__);

    CPubKey pubkey;
    txnouttype whichType;
    std::vector<valtype> vSolutions;
    const CTxOut& txout = block.vtx[1]->vout[1];
    if (!Solver(txout.scriptPubKey, whichType, vSolutions))
        return false;
    if (whichType == TX_PUBKEY || whichType == TX_PUBKEYHASH) {
        valtype& vchPubKey = vSolutions[0];
        pubkey = CPubKey(vchPubKey);
    }

    if (!pubkey.IsValid())
        return error("%s: invalid pubkey %s", __func__, pubkey.GetHex());

    return pubkey.Verify(block.GetHash(), block.vchBlockSig);
}

// pivx: CheckBlockSignature ]
// blocksignature ]
