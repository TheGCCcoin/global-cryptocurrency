// Copyright (c) 2012-2014 The Bitcoin Core developers
// Copyright (c) 2014-2017 The GCC Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_VERSION_H
#define BITCOIN_VERSION_H

// network protocol versioning [

/**
 * network protocol versioning
 */


static const int PROTOCOL_VERSION = 70210;

// u.1.5 INIT_PROTO_VERSION thegcc-old = 63012 [

//! initial proto version, to be increased after version/verack negotiation
static const int INIT_PROTO_VERSION = 63012; // thegcc-old, thegcc-new=70210, dash=209;

// u.1.5 INIT_PROTO_VERSION thegcc-old = 63012 ]

//! In this version, 'getheaders' was introduced.
static const int GETHEADERS_VERSION = 70077;

// p1.1 - MIN_PEER_PROTO_VERSION = 63010 [

//! disconnect from peers older than this proto version
//static const int MIN_PEER_PROTO_VERSION = 70208;
static const int MIN_PEER_PROTO_VERSION = 63010;

// p1.1 - MIN_PEER_PROTO_VERSION = 63010 ]
// u.1.4 - CADDR_TIME_VERSION = 61001 [

//! nTime field added to CAddress, starting with this version;
//! if possible, avoid requesting addresses nodes older than this
static const int CADDR_TIME_VERSION = 61001; // thegcc-old, dash=31402;

// u.1.4 - CADDR_TIME_VERSION = 61001 ]

//! BIP 0031, pong message, is enabled for all versions AFTER this one
static const int BIP0031_VERSION = 60000;

//! "mempool" command, enhanced "getdata" behavior starts with this version
static const int MEMPOOL_GD_VERSION = 60002;

//! "filter*" commands are disabled without NODE_BLOOM after and including this version
static const int NO_BLOOM_VERSION = 70201;

//! "sendheaders" command and announcing blocks with headers starts with this version
static const int SENDHEADERS_VERSION = 70201;

//! DIP0001 was activated in this version
static const int DIP0001_PROTOCOL_VERSION = 70208;

//! short-id-based block download starts with this version
static const int SHORT_IDS_BLOCKS_VERSION = 70209;

// o.1.1 old gcc OLD_PROTO_HEADERS_VERSION [

static const int OLD_PROTO_HEADERS_VERSION = 70000;

// o.1.1 old gcc OLD_PROTO_HEADERS_VERSION ]
// o.1.2 old gcc OLD_CMD_HEADERS_VERSION [

static const int OLD_CMD_HEADERS_VERSION = 69999;

// o.1.2 old gcc OLD_CMD_HEADERS_VERSION ]

// f.5.2 skip old blocks THEGCC_V1_BLOCK_VERSION [

static const int THEGCC_V1_BLOCK_VERSION = 6;

// f.5.2 skip old blocks THEGCC_V1_BLOCK_VERSION ]

// network protocol versioning ]

#endif // BITCOIN_VERSION_H
