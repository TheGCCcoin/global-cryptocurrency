// Copyright (c) 2017 The PIVX developers
// Copyright (c) 2018 TheGCCcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef THEGCC_BLOCKSIGNATURE_H
#define THEGCC_BLOCKSIGNATURE_H

// blocksignature header [

#include "key.h"
#include "primitives/block.h"
#include "keystore.h"

bool SignBlockWithKey(CBlock& block, const CKey& key);
bool SignBlock(CBlock& block, const CKeyStore& keystore);
bool CheckBlockSignature(const CBlock& block);

// blocksignature header ]

#endif //THEGCC_BLOCKSIGNATURE_H
