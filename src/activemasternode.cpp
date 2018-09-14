// Copyright (c) 2014-2017 The GCC Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// @ [

#include "activemasternode.h"
#include "masternode.h"
#include "masternode-sync.h"
#include "masternodeman.h"
#include "netbase.h"
#include "protocol.h"

// @ ]
// vars [

// Keep track of the active Masternode
CActiveMasternode activeMasternode;

// vars ]
// class CActiveMasternode [
// ManageState [

void CActiveMasternode::ManageState(CConnman& connman)
{
    LogPrint("masternode", "CActiveMasternode::ManageState -- Start\n");

    // !master -> exit [
    if(!fMasternodeMode) {
        LogPrint("masternode", "CActiveMasternode::ManageState -- Not a masternode, returning\n");
        return;
    }
    // !master -> exit ]

    // !sync + (main || dev) -> state=sync_in_progress [
    if(Params().NetworkIDString() != CBaseChainParams::REGTEST && !masternodeSync.IsBlockchainSynced()) {
        nState = ACTIVE_MASTERNODE_SYNC_IN_PROCESS;
        LogPrintf("CActiveMasternode::ManageState -- %s: %s\n", GetStateString(), GetStatus());
        return;
    }
    // !sync + (main || dev) -> state=sync_in_progress ]
    // regtest [

    if(nState == ACTIVE_MASTERNODE_SYNC_IN_PROCESS) {
        nState = ACTIVE_MASTERNODE_INITIAL;
    }

    LogPrint("masternode", "CActiveMasternode::ManageState -- status = %s, type = %s, pinger enabled = %d\n", GetStatus(), GetTypeString(), fPingerEnabled);

    if(eType == MASTERNODE_UNKNOWN) {
        ManageStateInitial(connman);
    }

    if(eType == MASTERNODE_REMOTE) {
        ManageStateRemote();
    }

    SendMasternodePing(connman);
    // regtest ]
}

// ManageState ]
// STR [
// GetStateString [
// -> state : string INITIAL, SYNC_IN_PROCESS, INPUT_TOO_NEW, NOT_CAPABLE, STARTED, UNKNOWN [

std::string CActiveMasternode::GetStateString() const
{
    switch (nState) {
        case ACTIVE_MASTERNODE_INITIAL:         return "INITIAL";
        case ACTIVE_MASTERNODE_SYNC_IN_PROCESS: return "SYNC_IN_PROCESS";
        case ACTIVE_MASTERNODE_INPUT_TOO_NEW:   return "INPUT_TOO_NEW";
        case ACTIVE_MASTERNODE_NOT_CAPABLE:     return "NOT_CAPABLE";
        case ACTIVE_MASTERNODE_STARTED:         return "STARTED";
        default:                                return "UNKNOWN";
    }
}

// -> state : string INITIAL, SYNC_IN_PROCESS, INPUT_TOO_NEW, NOT_CAPABLE, STARTED, UNKNOWN ]
// GetStateString ]
// GetStatus [
// -> state : string; INITIAL : "Node just started, not yet activated"... [

std::string CActiveMasternode::GetStatus() const
{
    switch (nState) {
        case ACTIVE_MASTERNODE_INITIAL:         return "Node just started, not yet activated";
        case ACTIVE_MASTERNODE_SYNC_IN_PROCESS: return "Sync in progress. Must wait until sync is complete to start Masternode";
        case ACTIVE_MASTERNODE_INPUT_TOO_NEW:   return strprintf("Masternode input must have at least %d confirmations", Params().GetConsensus().nMasternodeMinimumConfirmations);
        case ACTIVE_MASTERNODE_NOT_CAPABLE:     return "Not capable masternode: " + strNotCapableReason;
        case ACTIVE_MASTERNODE_STARTED:         return "Masternode successfully started";
        default:                                return "Unknown";
    }
}

// -> state : string; INITIAL : "Node just started, not yet activated"... ]
// GetStatus ]
// GetTypeString [
// -> type : string REMOTE, UNKNOWN [

std::string CActiveMasternode::GetTypeString() const
{
    std::string strType;
    switch(eType) {
    case MASTERNODE_REMOTE:
        strType = "REMOTE";
        break;
    default:
        strType = "UNKNOWN";
        break;
    }
    return strType;
}

// -> type : string REMOTE, UNKNOWN ]
// GetTypeString ]
// STR ]
// SendMasternodePing [

bool CActiveMasternode::SendMasternodePing(CConnman& connman)
{
    // !pingerEnabled -> exit [
    if(!fPingerEnabled) {
        LogPrint("masternode", "CActiveMasternode::SendMasternodePing -- %s: masternode ping service is disabled, skipping...\n", GetStateString());
        return false;
    }
    // !pingerEnabled -> exit ]

    // !mnodeman.Has(outpoint) ??? [
    if(!mnodeman.Has(outpoint)) {
        strNotCapableReason = "Masternode not in masternode list";
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        LogPrintf("CActiveMasternode::SendMasternodePing -- %s: %s\n", GetStateString(), strNotCapableReason);
        return false;
    }
    // !mnodeman.Has(outpoint) ??? ]
    // new CMasternodePing [

    CMasternodePing mnp(outpoint);
    mnp.nSentinelVersion = nSentinelVersion;
    mnp.fSentinelIsCurrent =
            (abs(GetAdjustedTime() - nSentinelPingTime) < MASTERNODE_SENTINEL_PING_MAX_SECONDS);
    if(!mnp.Sign(keyMasternode, pubKeyMasternode)) {
        LogPrintf("CActiveMasternode::SendMasternodePing -- ERROR: Couldn't sign Masternode Ping\n");
        return false;
    }
    // new CMasternodePing ]

    // check ping [
    // Update lastPing for our masternode in Masternode list
    if(mnodeman.IsMasternodePingedWithin(outpoint, MASTERNODE_MIN_MNP_SECONDS, mnp.sigTime)) {
        LogPrintf("CActiveMasternode::SendMasternodePing -- Too early to send Masternode Ping\n");
        return false;
    }
    // check ping ]
    // SetMasternodeLastPing -> Relay [

    mnodeman.SetMasternodeLastPing(outpoint, mnp);

    LogPrintf("CActiveMasternode::SendMasternodePing -- Relaying ping, collateral=%s\n", outpoint.ToStringShort());
    mnp.Relay(connman);

    // SetMasternodeLastPing -> Relay ]

    return true;
}

// SendMasternodePing ]
// UpdateSentinelPing [
// -> nSentinelVersion = version, nSentinelPingTime = date() [

bool CActiveMasternode::UpdateSentinelPing(int version)
{
    nSentinelVersion = version;
    nSentinelPingTime = GetAdjustedTime();

    return true;
}

// -> nSentinelVersion = version, nSentinelPingTime = date() ]
// UpdateSentinelPing ]
// ManageStateInitial [

void CActiveMasternode::ManageStateInitial(CConnman& connman)
{
    LogPrint("masternode", "CActiveMasternode::ManageStateInitial -- status = %s, type = %s, pinger enabled = %d\n", GetStatus(), GetTypeString(), fPingerEnabled);

    // !listen -> exit [
    // Check that our local network configuration is correct
    if (!fListen) {
        // listen option is probably overwritten by smth else, no good
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        strNotCapableReason = "Masternode must accept connections from outside. Make sure listen configuration option is not overwritten by some another parameter.";
        LogPrintf("CActiveMasternode::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
        return;
    }
    // !listen -> exit ]

    // find local peers () !!! [
    // First try to find whatever local address is specified by externalip option
    bool fFoundLocal = GetLocal(service) && CMasternode::IsValidNetAddr(service);
    if(!fFoundLocal) {
        bool empty = true;
        // If we have some peers, let's try to find our local address from one of them
        connman.ForEachNodeContinueIf(CConnman::AllNodes, [&fFoundLocal, &empty, this](CNode* pnode) {
            empty = false;
            if (pnode->addr.IsIPv4())
                fFoundLocal = GetLocal(service, &pnode->addr) && CMasternode::IsValidNetAddr(service);
            return !fFoundLocal;
        });
        // nothing and no live connections, can't do anything for now
        if (empty) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = "Can't detect valid external address. Will retry when there are some connections available.";
            LogPrintf("CActiveMasternode::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
            return;
        }
    }

    if(!fFoundLocal) {
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        strNotCapableReason = "Can't detect valid external address. Please consider using the externalip configuration option if problem persists. Make sure to use IPv4 address only.";
        LogPrintf("CActiveMasternode::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
        return;
    }
    // find local peers () !!! ]

    // check port (dup!) [
    int mainnetDefaultPort = Params(CBaseChainParams::MAIN).GetDefaultPort();
    if(Params().NetworkIDString() == CBaseChainParams::MAIN) {
        if(service.GetPort() != mainnetDefaultPort) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = strprintf("Invalid port: %u - only %d is supported on mainnet.", service.GetPort(), mainnetDefaultPort);
            LogPrintf("CActiveMasternode::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
            return;
        }
    } else if(service.GetPort() == mainnetDefaultPort) {
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        strNotCapableReason = strprintf("Invalid port: %u - %d is only supported on mainnet.", service.GetPort(), mainnetDefaultPort);
        LogPrintf("CActiveMasternode::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
        return;
    }
    // check port (dup!) ]

    // Check socket connectivity
    LogPrintf("CActiveMasternode::ManageStateInitial -- Checking inbound connection to '%s'\n", service.ToString());
    SOCKET hSocket;
    bool fConnected = ConnectSocket(service, hSocket, nConnectTimeout) && IsSelectableSocket(hSocket);
    CloseSocket(hSocket);

    if (!fConnected) {
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        strNotCapableReason = "Could not connect to " + service.ToString();
        LogPrintf("CActiveMasternode::ManageStateInitial -- %s: %s\n", GetStateString(), strNotCapableReason);
        return;
    }

    // Default to REMOTE
    eType = MASTERNODE_REMOTE;

    LogPrint("masternode", "CActiveMasternode::ManageStateInitial -- End status = %s, type = %s, pinger enabled = %d\n", GetStatus(), GetTypeString(), fPingerEnabled);
}

// ManageStateInitial ]
// ManageStateRemote [

void CActiveMasternode::ManageStateRemote()
{
    LogPrint("masternode", "CActiveMasternode::ManageStateRemote -- Start status = %s, type = %s, pinger enabled = %d, pubKeyMasternode.GetID() = %s\n", 
             GetStatus(), GetTypeString(), fPingerEnabled, pubKeyMasternode.GetID().ToString());

    // CheckMasternode (pubKeyMasternode)[
    mnodeman.CheckMasternode(pubKeyMasternode, true);
    // CheckMasternode (pubKeyMasternode)]

    // GetMasternodeInfo (pubKeyMasternode, infoMn)[
    masternode_info_t infoMn;
    if(mnodeman.GetMasternodeInfo(pubKeyMasternode, infoMn)) {
        // nProtocolVersion != PROTOCOL_VERSION -> bad [
        if(infoMn.nProtocolVersion != PROTOCOL_VERSION) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = "Invalid protocol version";
            LogPrintf("CActiveMasternode::ManageStateRemote -- %s: %s\n", GetStateString(), strNotCapableReason);
            return;
        }
        // nProtocolVersion != PROTOCOL_VERSION -> bad ]
        // service != infoMn.addr -> bad [
        if(service != infoMn.addr) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = "Broadcasted IP doesn't match our external address. Make sure you issued a new broadcast if IP of this masternode changed recently.";
            LogPrintf("CActiveMasternode::ManageStateRemote -- %s: %s\n", GetStateString(), strNotCapableReason);
            return;
        }
        // service != infoMn.addr -> bad ]
        // IsValidStateForAutoStart (nActiveState) ! -> bad [
        if(!CMasternode::IsValidStateForAutoStart(infoMn.nActiveState)) {
            nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
            strNotCapableReason = strprintf("Masternode in %s state", CMasternode::StateToString(infoMn.nActiveState));
            LogPrintf("CActiveMasternode::ManageStateRemote -- %s: %s\n", GetStateString(), strNotCapableReason);
            return;
        }
        // IsValidStateForAutoStart (nActiveState) ! -> bad ]
        // ok -> state = ACTIVE_MASTERNODE_STARTED [
        if(nState != ACTIVE_MASTERNODE_STARTED) {
            LogPrintf("CActiveMasternode::ManageStateRemote -- STARTED!\n");
            outpoint = infoMn.outpoint;
            service = infoMn.addr;
            fPingerEnabled = true;
            nState = ACTIVE_MASTERNODE_STARTED;
        }
        // ok -> state = ACTIVE_MASTERNODE_STARTED ]
    }
    else {
        // else -> bad [
        nState = ACTIVE_MASTERNODE_NOT_CAPABLE;
        strNotCapableReason = "Masternode not in masternode list";
        LogPrintf("CActiveMasternode::ManageStateRemote -- %s: %s\n", GetStateString(), strNotCapableReason);
        // else -> bad ]
    }
    // GetMasternodeInfo (pubKeyMasternode, infoMn)]
}

// ManageStateRemote ]
// class CActiveMasternode ]
