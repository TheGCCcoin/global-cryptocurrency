// @ [

using namespace std;

// @ ]
// m.3.1 wallet CreateCoinStake [

bool Stake(CStakeInput* stakeInput, unsigned int nBits, unsigned int nTimeBlockFrom, unsigned int& nTimeTx, uint256& hashProofOfStake);

bool CWallet::CreateCoinStake(const CKeyStore& keystore, unsigned int nBits, int64_t nSearchInterval, CMutableTransaction& txNew, unsigned int& nTxNewTime)
{
    // The following split & combine thresholds are important to security
    // Should not be adjusted if you don't understand the consequences
    //int64_t nCombineThreshold = 0;
    txNew.vin.clear();
    txNew.vout.clear();

    // Mark coin stake transaction
    CScript scriptEmpty;
    scriptEmpty.clear();
    txNew.vout.push_back(CTxOut(0, scriptEmpty));

    // Choose coins to use
    CAmount nBalance = GetBalance();

    if (ParseMoney(GetArg("-reservebalance", "0"), nReserveBalance))
        return error("CreateCoinStake : invalid reserve balance amount");

    if (nBalance > 0 && nBalance <= nReserveBalance)
        return false;

    // Get the list of stakable inputs
    std::list<std::unique_ptr<CStakeInput> > listInputs;
    if (!SelectStakeCoins(listInputs, nBalance - nReserveBalance))
        return false;

    if (listInputs.empty())
        return false;

    if (GetAdjustedTime() - chainActive.Tip()->GetBlockTime() < 60)
        MilliSleep(10000);

    CAmount nCredit = 0;
    CScript scriptPubKeyKernel;
    bool fKernelFound = false;
    for (std::unique_ptr<CStakeInput>& stakeInput : listInputs) {
        // Make sure the wallet is unlocked and shutdown hasn't been requested
        if (IsLocked() || ShutdownRequested())
            return false;

        //make sure that enough time has elapsed between
        CBlockIndex* pindex = stakeInput->GetIndexFrom();
        if (!pindex || pindex->nHeight < 1) {
            LogPrintf("*** no pindexfrom\n");
            continue;
        }

        // Read block header
        CBlockHeader block = pindex->GetBlockHeader();
        uint256 hashProofOfStake;
        nTxNewTime = GetAdjustedTime();

        //iterates each utxo inside of CheckStakeKernelHash()
        if (Stake(stakeInput.get(), nBits, block.GetBlockTime(), nTxNewTime, hashProofOfStake)) {
            LOCK(cs_main);
            //Double check that this will pass time requirements
            if (nTxNewTime <= chainActive.Tip()->GetMedianTimePast()) {
                LogPrintf("CreateCoinStake() : kernel found, but it is too far in the past \n");
                continue;
            }

            // Found a kernel
            LogPrintf("CreateCoinStake : kernel found\n");
            nCredit += stakeInput->GetValue();

            // Calculate reward
            CAmount nReward;
            nReward = GetBlockValue(chainActive.Height() + 1);
            nCredit += nReward;

            // Create the output transaction(s)
            vector<CTxOut> vout;
            if (!stakeInput->CreateTxOuts(this, vout, nCredit)) {
                LogPrintf("%s : failed to get scriptPubKey\n", __func__);
                continue;
            }
            txNew.vout.insert(txNew.vout.end(), vout.begin(), vout.end());

            CAmount nMinFee = 0;
            // Set output amount
            if (txNew.vout.size() == 3) {
                txNew.vout[1].nValue = ((nCredit - nMinFee) / 2 / CENT) * CENT;
                txNew.vout[2].nValue = nCredit - nMinFee - txNew.vout[1].nValue;
            } else
                txNew.vout[1].nValue = nCredit - nMinFee;

            // Limit size
            unsigned int nBytes = ::GetSerializeSize(txNew, SER_NETWORK, PROTOCOL_VERSION);
            if (nBytes >= DEFAULT_BLOCK_MAX_SIZE / 5)
                return error("CreateCoinStake : exceeded coinstake size limit");

            //Masternode payment
            FillBlockPayee(txNew, nMinFee, true);

            uint256 hashTxOut = txNew.GetHash();
            CTxIn in;
            if (!stakeInput->CreateTxIn(this, in, hashTxOut)) {
                LogPrintf("%s : failed to create TxIn\n", __func__);
                txNew.vin.clear();
                txNew.vout.clear();
                nCredit = 0;
                continue;
            }
            txNew.vin.emplace_back(in);

            fKernelFound = true;
            break;
        }
        if (fKernelFound)
            break; // if kernel is found stop searching
    }
    if (!fKernelFound)
        return false;

    int nIn = 0;
    for (CTxIn txIn : txNew.vin) {
        const CWalletTx *wtx = GetWalletTx(txIn.prevout.hash);
        if (!SignSignature(*this, *wtx, txNew, nIn++))
            return error("CreateCoinStake : failed to sign coinstake");
    }

    // Successfully generated coinstake
    return true;
}

// m.3.1 wallet CreateCoinStake ]
