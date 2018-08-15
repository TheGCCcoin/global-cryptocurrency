#ifndef THEGCC_IMPORTS_H
#define THEGCC_IMPORTS_H

// 0 [

#include "validation.h"

#include "alert.h"
#include "arith_uint256.h"
#include "blockencodings.h"
#include "chainparams.h"
#include "checkpoints.h"
#include "checkqueue.h"
#include "consensus/consensus.h"
#include "consensus/merkle.h"
#include "consensus/validation.h"
#include "hash.h"
#include "init.h"
#include "policy/policy.h"
#include "pow.h"
#include "primitives/block.h"
#include "primitives/transaction.h"
#include "script/script.h"
#include "script/sigcache.h"
#include "script/standard.h"
#include "timedata.h"
#include "tinyformat.h"
#include "txdb.h"
#include "txmempool.h"
#include "ui_interface.h"
#include "undo.h"
#include "util.h"
#include "spork.h"
#include "utilmoneystr.h"
#include "utilstrencodings.h"
#include "validationinterface.h"
#include "versionbits.h"
#include "warnings.h"

#include "instantx.h"
#include "masternodeman.h"
#include "masternode-payments.h"

#include <atomic>
#include <sstream>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/math/distributions/poisson.hpp>
#include <boost/thread.hpp>

#if defined(NDEBUG)
# error "GCC Core cannot be compiled without assertions."
#endif

#ifdef ENABLE_WALLET
#include "wallet/wallet.h"
#endif

// 0 ]
// pos [

#include "thegcc/blocksignature.h"
#include "thegcc/stakeinput.h"

// pos ]
// thegcc [

#include "thegcc/wallet/Wallet.h"
#include "thegcc/pos/Pos.h"
#include "thegcc/masternode/Masternode.h"
#include "thegcc/validation/Block.h"
#include "thegcc/kernel/Kernel.h"

// thegcc ]
// 1 [

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

#include "thegcc/blocksignature.h"

#include "policy/policy.h"

#include "masternode-payments.h"

#include <boost/thread.hpp>
#include <boost/tuple/tuple.hpp>

#include "undo.h"

#include "thegcc/stakeinput.h"

#include "init.h"

#include "script/sign.h"

// 1 ]
// @ [

extern int64_t nReserveBalance;

// @ ]


#endif //THEGCC_IMPORTS_H
