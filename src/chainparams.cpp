// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin Core developers
// Copyright (c) 2014-2017 The kepler Core developers
// Copyright (c) 2019 The Kepler developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "chainparams.h"
#include "consensus/merkle.h"

#include "tinyformat.h"
#include "util.h"
#include "utilstrencodings.h"

#include "arith_uint256.h"

#include <assert.h>

#include <boost/assign/list_of.hpp>

#include "chainparamsseeds.h"

#include "primitives/block.h"

static CBlock CreateGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799/*dec a hex, parece ser los nbits de bitcoin*/ << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

static CBlock CreateDevNetGenesisBlock(const uint256 &prevBlockHash, const std::string& devNetName, uint32_t nTime, uint32_t nNonce, uint32_t nBits, const CAmount& genesisReward)
{
    assert(!devNetName.empty());

    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    // put height (BIP34) and devnet name into coinbase
    txNew.vin[0].scriptSig = CScript() << 1 << std::vector<unsigned char>(devNetName.begin(), devNetName.end());
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = CScript() << OP_RETURN;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = 4;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock = prevBlockHash;
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

/**
 * Build the genesis block. Note that the output of its generation
 * transaction cannot be spent since it did not originally exist in the
 * database.
 *
 * CBlock(hash=00000ffd590b14, ver=1, hashPrevBlock=00000000000000, hashMerkleRoot=e0028e, nTime=1390095618, nBits=1e0ffff0, nNonce=28917698, vtx=1)
 *   CTransaction(hash=e0028e, ver=1, vin.size=1, vout.size=1, nLockTime=0)
 *     CTxIn(COutPoint(000000, -1), coinbase 04ffff001d01044c5957697265642030392f4a616e2f3230313420546865204772616e64204578706572696d656e7420476f6573204c6976653a204f76657273746f636b2e636f6d204973204e6f7720416363657074696e6720426974636f696e73)
 *     CTxOut(nValue=50.00000000, scriptPubKey=0xA9037BAC7050C479B121CF)
 *   vMerkleTree: e0028e
 */
static CBlock CreateGenesisBlock(uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    const char* pszTimestamp = "XX/05/2019 NOTICIA";
    const CScript genesisOutputScript = CScript() << ParseHex("04b8bbf7e36419f96fc99b7d9d04a62e8d9a28f6c8dc548e7b9b84b44c380693b76e730f28d18894bc05a0a72d5bb8e35221dc0d375f8552c9485995f60a94d23a") << OP_CHECKSIG;
    return CreateGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nNonce, nBits, nVersion, genesisReward);
}

static CBlock FindDevNetGenesisBlock(const Consensus::Params& params, const CBlock &prevBlock, const CAmount& reward)
{
    std::string devNetName = GetDevNetName();
    assert(!devNetName.empty());

    CBlock block = CreateDevNetGenesisBlock(prevBlock.GetHash(), devNetName.c_str(), prevBlock.nTime + 1, 0, prevBlock.nBits, reward);

    arith_uint256 bnTarget;
    bnTarget.SetCompact(block.nBits);

    for (uint32_t nNonce = 0; nNonce < UINT32_MAX; nNonce++) {
        block.nNonce = nNonce;

        uint256 hash = block.GetHash();
        if (UintToArith256(hash) <= bnTarget)
            return block;
    }

    // This is very unlikely to happen as we start the devnet with a very low difficulty. In many cases even the first
    // iteration of the above loop will give a result already
    error("FindDevNetGenesisBlock: could not find devnet genesis block for %s", devNetName);
    assert(false);
}

/**
 * Main network
 */
/**
 * What makes a good checkpoint block?
 * + Is surrounded by blocks with reasonable timestamps
 *   (no blocks before with a timestamp after, none after with
 *    timestamp before)
 * + Contains no strange transactions
 */


class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";
        consensus.nSubsidyHalvingInterval = 1051200; // Note: Number of blocks per calendar year is 262800 :: 525600 mins por año
        consensus.nMasternodePaymentsStartBlock = 22000; // 1 month and 100 blocks after genesis
        consensus.nMNPaymentIncreaseBlocks = 5040; // steps in GetMasternodePayment, increase every week. 1 week = 10080 mins
        consensus.nMasternodeCost = 10000;

        consensus.nInstantSendConfirmationsRequired = 6;
        consensus.nInstantSendKeepLock = 24;

        consensus.nBudgetPaymentsStartBlock = 10000000; // actual historical value
        consensus.nBudgetPaymentsCycleBlocks = 1000; // ~(60*24*30)/2.6, actual number of blocks per month is 200700 / 12 = 16725
        consensus.nBudgetPaymentsWindowBlocks = 100;
        consensus.nSuperblockStartBlock = 10000001; // The block at which 12.1 goes live (end of final 12.0 budget cycle)
        consensus.nSuperblockStartHash = uint256S("00000ff0000fffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nSuperblockCycle = 10; // ~(60*24*30)/2.6, actual number of blocks per month is 200700 / 12 = 16725
        
        consensus.nGovernanceMinQuorum = 10;
        consensus.nGovernanceFilterElements = 20000;
        consensus.nMasternodeMinimumConfirmations = 15;
        consensus.BIP34Height = 1;consensus.BIP65Height = 1;consensus.BIP66Height = 1; 
        consensus.DIP0001Height = 1;

        consensus.powLimit[ALGO_SLOT1] = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // NeoScrypt
        consensus.powLimit[ALGO_SLOT2] = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // Argon2-4096
        consensus.powLimit[ALGO_SLOT3] = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // RainforestV2
        consensus.nPowTargetTimespan = 24 * 60 * 60; // Kepler: 1 day
        
        consensus.nPowTargetSpacing = 120; // 120 second block time, target is set to 120 * NUM_ALGOS, so every algorithm finds a block every 2 mins on avg
        consensus.nPoWAveragingInterval = 10; // 10 block averaging interval
        consensus.nMaxAdjustDown = 4; // 4% adjustment downwards
        consensus.nMaxAdjustUp = 4; // 4% adjustment upwards
        consensus.nBlockSequentialAlgoMaxCount = 10;

        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1916; // 95% of 2016
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing


        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 8;// was 28 but max is 8
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x"); // 1 getblockchaininfo: chainwork

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x"); // 1

        /**
         * The message start string is designed to be unlikely to occur in normal data.
         * The characters are rarely used upper ASCII, not valid as UTF-8, and produce
         * a large 32-bit integer with any alignment.
         */
        pchMessageStart[0] = 0xbf;
        pchMessageStart[1] = 0xc6;
        pchMessageStart[2] = 0xde;
        pchMessageStart[3] = 0xbe;
        vAlertPubKey = ParseHex("044282e4ccc62fa9a31ea871f9664e8927c84a82a71e0103f4dd7fe56579a160abbab00770595e137f150e325733094e183d66e049ecac6d12ac1f6b402a0a9b8b");
        nDefaultPort = 12102;
        nPruneAfterHeight = 100000;

        // timestamp, nNonce, nBits, nVersion, genesisReward 
        genesis = CreateGenesisBlock(1555986291, 36257733, 0x1e0ffff0, 1, 50 * COIN);
        if(false)
        {
            printf("Searching for mainnet genesis block...\n");
            bool fNegative;
            bool fOverflow;
            arith_uint256 bnTarget;
            bnTarget.SetCompact(genesis.nBits, &fNegative, &fOverflow);
            int algo = genesis.GetAlgo();

            uint256 thash = genesis.GetPoWHash(algo);

            while (UintToArith256(thash) > bnTarget)
            {
                thash = genesis.GetPoWHash(algo);
                if (UintToArith256(thash) <= bnTarget)
                    break;
                if ((genesis.nNonce & 0xFFFFF) == 0)
                {
                    printf("nonce %08X: PoWhash = %s (target = %s)\n", genesis.nNonce, thash.ToString().c_str(), bnTarget.ToString().c_str());
                }
                ++genesis.nNonce;
                if (genesis.nNonce == 0)
                {
                    printf("NONCE WRAPPED, incrementing time\n");
                    ++genesis.nTime;
                }
            }

            printf("genesis.nTime = %u \n", genesis.nTime);
            printf("genesis.nNonce = %u \n", genesis.nNonce);
            printf("genesis.GetHash = %s\n", genesis.GetHash().ToString().c_str());
            printf("genesis.GetPoWHash = %s\n", genesis.GetPoWHash(algo).ToString().c_str());
            printf("genesis.hashMerkleRoot = %s\n", BlockMerkleRoot(genesis).ToString().c_str());
        }


        consensus.hashGenesisBlock = genesis.GetHash();
        assert(consensus.hashGenesisBlock == uint256S("0x"));
        assert(genesis.hashMerkleRoot == uint256S("0x"));


        vSeeds.push_back(CDNSSeedData("kepler.cash", "seed1.kepler.cash"));
        vSeeds.push_back(CDNSSeedData("kepler.cash", "seed2.kepler.cash"));

        // Kepler addresses start with 'K'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,45);
        // Kepler script addresses start with '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,20);
        // Kepler private keys start with '8' // ??
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,49);
        // Kepler BIP32 pubkeys start with 'xpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x88)(0xB2)(0x1E).convert_to_container<std::vector<unsigned char> >();
        // Kepler BIP32 prvkeys start with 'xprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x88)(0xAD)(0xE4).convert_to_container<std::vector<unsigned char> >();

        // Kepler BIP44 coin type is '264', DASH, 5
        nExtCoinType = 264;

        vFixedSeeds = std::vector<SeedSpec6>(pnSeed6_main, pnSeed6_main + ARRAYLEN(pnSeed6_main));

        fMiningRequiresPeers = true; // set to false for debug
        fDefaultConsistencyChecks = false;
        fRequireStandard = true;
        fMineBlocksOnDemand = false;
        fAllowMultipleAddressesFromGroup = false;
        fAllowMultiplePorts = false;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 60*60; // fulfilled requests expire in 1 hour

        strSporkAddress = "KKuLrH5JHbt42YqXwUsnRwzX7mBwV7AXWV";

        checkpointData = (CCheckpointData) {
            //boost::assign::map_list_of
            //(  1, uint256S("0x"))
            //(  1000, uint256S("0x2fc4ae8e56ac19d9dc3f4aba22062d63627b8ea4d918a3d814c763923fa2b836"))
        };

        chainTxData = ChainTxData{
            0, // * UNIX timestamp of last known number of transactions
            0,    // * total number of transactions between genesis and that timestamp
                        //   (the tx=... number in the SetBestChain debug.log lines)
            0.1         // * estimated number of transactions per second after that timestamp
        };
    }
};
static CMainParams mainParams;

/**
 * Testnet (v3)
 */
class CTestNetParams : public CChainParams {
public:
    CTestNetParams() {
        strNetworkID = "test";
        consensus.nSubsidyHalvingInterval = 1051200;
        consensus.nMasternodePaymentsStartBlock = 130; // 2 months after genesis
        consensus.nMNPaymentIncreaseBlocks = 15; // steps in GetMasternodePayment, increase every week
        consensus.nMasternodeCost = 100;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nBudgetPaymentsStartBlock = 4100;
        consensus.nBudgetPaymentsCycleBlocks = 50;
        consensus.nBudgetPaymentsWindowBlocks = 10;
        consensus.nSuperblockStartBlock = 4200; // NOTE: Should satisfy nSuperblockStartBlock > nBudgetPeymentsStartBlock
        consensus.nSuperblockStartHash = uint256S("00000000cffabc0f646867fba0550afd6e30e0f4b0fc54e34d3e101a1552df5d");
        consensus.nSuperblockCycle = 24; // Superblocks can be issued hourly on testnet
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.BIP34Height = 5;
        consensus.BIP65Height = 10;
        consensus.BIP66Height = 15; 
        consensus.DIP0001Height = 20;
        consensus.powLimit[ALGO_SLOT1] = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.powLimit[ALGO_SLOT2] = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.powLimit[ALGO_SLOT3] = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 24 * 60 * 60; // Kepler: 1 day

        consensus.nPowTargetSpacing = 120; // 120 second block time
        consensus.nPoWAveragingInterval = 10; // 10 block averaging interval
        consensus.nMaxAdjustDown = 4; // 4% adjustment downwards
        consensus.nMaxAdjustUp = 4; // 4% adjustment upwards
        consensus.nBlockSequentialAlgoMaxCount = 20;

        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;
        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000003be69c34b1244f"); // 143200

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x0000000004a7878409189b7a8f75b3815d9b8c45ee8f79955a6c727d83bddb04"); // 143200

        pchMessageStart[0] = 0xcd;
        pchMessageStart[1] = 0x5a;
        pchMessageStart[2] = 0xd6;
        pchMessageStart[3] = 0xf8;
        vAlertPubKey = ParseHex("04b8bbf7e36419f96fc99b7d9d04a62e8d9a28f6c8dc548e7b9b84b44c380693b76e730f28d18894bc05a0a72d5bb8e35221dc0d375f8552c9485995f60a94d23a");
        nDefaultPort = 12202;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1390666206UL, 3861367235UL, 0x1e0ffff0, 1, 50 * COIN);

        if(false)
        {
            printf("Searching for testnet genesis block...\n");
            bool fNegative;
            bool fOverflow;
            arith_uint256 bnTarget;
            bnTarget.SetCompact(genesis.nBits, &fNegative, &fOverflow);
            int algo = genesis.GetAlgo();

            uint256 thash = genesis.GetPoWHash(algo);

            while (UintToArith256(thash) > bnTarget)
            {
                thash = genesis.GetPoWHash(algo);
                if (UintToArith256(thash) <= bnTarget)
                    break;
                if ((genesis.nNonce & 0xFFFFF) == 0)
                {
                    printf("nonce %08X: PoWhash = %s (target = %s)\n", genesis.nNonce, thash.ToString().c_str(), bnTarget.ToString().c_str());
                }
                ++genesis.nNonce;
                if (genesis.nNonce == 0)
                {
                    printf("NONCE WRAPPED, incrementing time\n");
                    ++genesis.nTime;
                }
            }

            printf("genesis.nTime = %u \n", genesis.nTime);
            printf("genesis.nNonce = %u \n", genesis.nNonce);
            printf("genesis.GetHash = %s\n", genesis.GetHash().ToString().c_str());
            printf("genesis.GetPoWHash = %s\n", genesis.GetPoWHash(algo).ToString().c_str());
            printf("genesis.hashMerkleRoot = %s\n", BlockMerkleRoot(genesis).ToString().c_str());
}
        consensus.hashGenesisBlock = genesis.GetHash();
        //assert(consensus.hashGenesisBlock == uint256S("0x00000bafbc94add76cb75e2ec92894837288a481e5c005f6563d91623bf8bc2c"));
        //assert(genesis.hashMerkleRoot == uint256S("0xe0028eb9648db56b1ac77cf090b99048a8007e2bb64b68f092c03c7f56a662c7"));

        vFixedSeeds.clear();
        vSeeds.clear();
        // nodes with support for servicebits filtering should be at the top
        /*vSeeds.push_back(CDNSSeedData("keplerdot.io",  "testnet-seed.keplerdot.io"));
        vSeeds.push_back(CDNSSeedData("masternode.io", "test.dnsseed.masternode.io"));*/
        vSeeds.push_back(CDNSSeedData("kepler.cash", "testseed.kepler.cash"));


        // Testnet Kepler addresses start with 'y'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
        // Testnet Kepler script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Testnet Kepler BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        // Testnet Kepler BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        // Testnet Kepler BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fAllowMultipleAddressesFromGroup = false;
        fAllowMultiplePorts = false;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        strSporkAddress = "yjPtiKh2uwk3bDutTEA2q9mCtXyiZRWn55";

        checkpointData = (CCheckpointData) {
            /*boost::assign::map_list_of
            (    261, uint256S("0x00000c26026d0815a7e2ce4fa270775f61403c040647ff2c3091f99e894a4618"))
            (   1999, uint256S("0x00000052e538d27fa53693efe6fb6892a0c1d26c0235f599171c48a3cce553b1"))
            (   2999, uint256S("0x0000024bc3f4f4cb30d29827c13d921ad77d2c6072e586c7f60d83c2722cdcc5"))
            ( 100000, uint256S("0x0000000003aa53e24b6e60ef97642e4193611f2bcb75ea1fa8105f0b5ffd5242"))
            ( 143200, uint256S("0x0000000004a7878409189b7a8f75b3815d9b8c45ee8f79955a6c727d83bddb04"))*/
        };

        chainTxData = ChainTxData{        
            1529294335, // * UNIX timestamp of last known number of transactions
            0,    // * total number of transactions between genesis and that timestamp
                        //   (the tx=... number in the SetBestChain debug.log lines)
            0.01        // * estimated number of transactions per second after that timestamp
        };

    }
};
static CTestNetParams testNetParams;

/**
 * Devnet
 */
class CDevNetParams : public CChainParams {
public:
    CDevNetParams() {
        strNetworkID = "dev";
        consensus.nSubsidyHalvingInterval = 210240;
        consensus.nMasternodePaymentsStartBlock = 40; // 2 months after genesis
        consensus.nMNPaymentIncreaseBlocks = 10; // steps in GetMasternodePayment, increase every week
        consensus.nMasternodeCost = 100;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nBudgetPaymentsStartBlock = 4100;
        consensus.nBudgetPaymentsCycleBlocks = 50;
        consensus.nBudgetPaymentsWindowBlocks = 10;
        consensus.nSuperblockStartBlock = 4200; // NOTE: Should satisfy nSuperblockStartBlock > nBudgetPeymentsStartBlock
        consensus.nSuperblockStartHash = uint256(); // do not check this on devnet
        consensus.nSuperblockCycle = 24; // Superblocks can be issued hourly on devnet
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 500;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.BIP34Height = 1; // BIP34 activated immediately on devnet
        consensus.BIP65Height = 1; // BIP65 activated immediately on devnet
        consensus.BIP66Height = 1; // BIP66 activated immediately on devnet
        consensus.DIP0001Height = 2; // DIP0001 activated immediately on devnet
        consensus.powLimit[ALGO_SLOT1] = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.powLimit[ALGO_SLOT2] = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.powLimit[ALGO_SLOT3] = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 24 * 60 * 60; // Kepler: 1 day

        consensus.nPowTargetSpacing = 120; // 120 second block time
        consensus.nPoWAveragingInterval = 10; // 10 block averaging interval
        consensus.nMaxAdjustDown = 4; // 4% adjustment downwards
        consensus.nMaxAdjustUp = 4; // 4% adjustment upwards
        consensus.nBlockSequentialAlgoMaxCount = 10;

        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = false;

        consensus.nRuleChangeActivationThreshold = 1512; // 75% for testchains
        consensus.nMinerConfirmationWindow = 2016; // nPowTargetTimespan / nPowTargetSpacing
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 1199145601; // January 1, 2008
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 1230767999; // December 31, 2008

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x000000000000000000000000000000000000000000000000000000000000000");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x000000000000000000000000000000000000000000000000000000000000000");

        pchMessageStart[0] = 0xe2;
        pchMessageStart[1] = 0xca;
        pchMessageStart[2] = 0xff;
        pchMessageStart[3] = 0xce;
        vAlertPubKey = ParseHex("04b8bbf7e36419f96fc99b7d9d04a62e8d9a28f6c8dc548e7b9b84b44c380693b76e730f28d18894bc05a0a72d5bb8e35221dc0d375f8552c9485995f60a94d23a");
        nDefaultPort = 12202;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1417713337, 1096447, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        //assert(consensus.hashGenesisBlock == uint256S("0x000008ca1832a4baf228eb1553c03d3a2c8e02399550dd6ea8d65cec3ef23d2e"));
        //assert(genesis.hashMerkleRoot == uint256S("0xe0028eb9648db56b1ac77cf090b99048a8007e2bb64b68f092c03c7f56a662c7"));

        devnetGenesis = FindDevNetGenesisBlock(consensus, genesis, 50 * COIN);
        consensus.hashDevnetGenesisBlock = devnetGenesis.GetHash();

        vFixedSeeds.clear();
        vSeeds.clear();
        //vSeeds.push_back(CDNSSeedData("keplerevo.org",  "devnet-seed.keplerevo.org"));

        // Testnet Kepler addresses start with 'y'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
        // Testnet Kepler script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Testnet private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Testnet Kepler BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        // Testnet Kepler BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        // Testnet Kepler BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;

        fMiningRequiresPeers = true;
        fDefaultConsistencyChecks = false;
        fRequireStandard = false;
        fMineBlocksOnDemand = false;
        fAllowMultipleAddressesFromGroup = true;
        fAllowMultiplePorts = true;

        nPoolMaxTransactions = 3;
        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        strSporkAddress = "yjPtiKh2uwk3bDutTEA2q9mCtXyiZRWn55";

        checkpointData = (CCheckpointData) {
            boost::assign::map_list_of
            (      0, uint256S("0x000008ca1832a4baf228eb1553c03d3a2c8e02399550dd6ea8d65cec3ef23d2e"))
            (      1, devnetGenesis.GetHash())
        };

        chainTxData = ChainTxData{
            devnetGenesis.GetBlockTime(), // * UNIX timestamp of devnet genesis block
            2,                            // * we only have 2 coinbase transactions when a devnet is started up
            0.01                          // * estimated number of transactions per second
        };
    }
};
static CDevNetParams *devNetParams;


/**
 * Regression test
 */
class CRegTestParams : public CChainParams {
public:
    CRegTestParams() {
        strNetworkID = "regtest";
        consensus.nSubsidyHalvingInterval = 150;
        consensus.nMasternodePaymentsStartBlock = 40; // 2 months after genesis
        consensus.nMNPaymentIncreaseBlocks = 10; // steps in GetMasternodePayment, increase every week
        consensus.nMasternodeCost = 100;
        consensus.nInstantSendConfirmationsRequired = 2;
        consensus.nInstantSendKeepLock = 6;
        consensus.nBudgetPaymentsStartBlock = 1000;
        consensus.nBudgetPaymentsCycleBlocks = 50;
        consensus.nBudgetPaymentsWindowBlocks = 10;
        consensus.nSuperblockStartBlock = 1500;
        consensus.nSuperblockStartHash = uint256(); // do not check this on regtest
        consensus.nSuperblockCycle = 10;
        consensus.nGovernanceMinQuorum = 1;
        consensus.nGovernanceFilterElements = 100;
        consensus.nMasternodeMinimumConfirmations = 1;
        consensus.BIP34Height = 100000000; // BIP34 has not activated on regtest (far in the future so block v1 are not rejected in tests)
        consensus.BIP65Height = 1351; // BIP65 activated on regtest (Used in rpc activation tests)
        consensus.BIP66Height = 1251; // BIP66 activated on regtest (Used in rpc activation tests)
        consensus.DIP0001Height = 2000;
        //consensus.powLimit = uint256S("7fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"); // ~uint256(0) >> 1
        consensus.powLimit[ALGO_SLOT1] = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.powLimit[ALGO_SLOT2] = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.powLimit[ALGO_SLOT3] = uint256S("0000ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 24 * 60 * 60; // Kepler: 1 day

        consensus.nPowTargetSpacing = 120; // 120 second block time
        consensus.nPoWAveragingInterval = 10; // 10 block averaging interval
        consensus.nMaxAdjustDown = 4; // 4% adjustment downwards
        consensus.nMaxAdjustUp = 4; // 4% adjustment upwards
        consensus.nBlockSequentialAlgoMaxCount = 10;
        
        consensus.fPowAllowMinDifficultyBlocks = true;
        consensus.fPowNoRetargeting = true;
        consensus.nRuleChangeActivationThreshold = 108; // 75% for testchains
        consensus.nMinerConfirmationWindow = 144; // Faster than normal for regtest (144 instead of 2016)
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].bit = 28;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nStartTime = 0;
        consensus.vDeployments[Consensus::DEPLOYMENT_TESTDUMMY].nTimeout = 999999999999ULL;

        // The best chain should have at least this much work.
        consensus.nMinimumChainWork = uint256S("0x00");

        // By default assume that the signatures in ancestors of this block are valid.
        consensus.defaultAssumeValid = uint256S("0x00");

        pchMessageStart[0] = 0xfc;
        pchMessageStart[1] = 0xc1;
        pchMessageStart[2] = 0xb7;
        pchMessageStart[3] = 0xdc;
        nDefaultPort = 19994;
        nPruneAfterHeight = 1000;

        genesis = CreateGenesisBlock(1417713337, 1096447, 0x207fffff, 1, 50 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();
        //assert(consensus.hashGenesisBlock == uint256S("0x000008ca1832a4baf228eb1553c03d3a2c8e02399550dd6ea8d65cec3ef23d2e"));
        //assert(genesis.hashMerkleRoot == uint256S("0xe0028eb9648db56b1ac77cf090b99048a8007e2bb64b68f092c03c7f56a662c7"));

        vFixedSeeds.clear(); //!< Regtest mode doesn't have any fixed seeds.
        vSeeds.clear();      //!< Regtest mode doesn't have any DNS seeds.

        fMiningRequiresPeers = false;
        fDefaultConsistencyChecks = true;
        fRequireStandard = false;
        fMineBlocksOnDemand = true;
        fAllowMultipleAddressesFromGroup = true;
        fAllowMultiplePorts = true;

        nFulfilledRequestExpireTime = 5*60; // fulfilled requests expire in 5 minutes

        // privKey: cP4EKFyJsHT39LDqgdcB43Y3YXjNyjb5Fuas1GQSeAtjnZWmZEQK
        strSporkAddress = "yj949n1UH6fDhw6HtVE5VMj2iSTaSWBMcW";

        checkpointData = (CCheckpointData){
            boost::assign::map_list_of
            ( 0, uint256S("0x000008ca1832a4baf228eb1553c03d3a2c8e02399550dd6ea8d65cec3ef23d2e"))
        };

        chainTxData = ChainTxData{
            0,
            0,
            0
        };

        // Regtest Kepler addresses start with 'y'
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1,140);
        // Regtest Kepler script addresses start with '8' or '9'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1,19);
        // Regtest private keys start with '9' or 'c' (Bitcoin defaults)
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1,239);
        // Regtest Kepler BIP32 pubkeys start with 'tpub' (Bitcoin defaults)
        base58Prefixes[EXT_PUBLIC_KEY] = boost::assign::list_of(0x04)(0x35)(0x87)(0xCF).convert_to_container<std::vector<unsigned char> >();
        // Regtest Kepler BIP32 prvkeys start with 'tprv' (Bitcoin defaults)
        base58Prefixes[EXT_SECRET_KEY] = boost::assign::list_of(0x04)(0x35)(0x83)(0x94).convert_to_container<std::vector<unsigned char> >();

        // Regtest Kepler BIP44 coin type is '1' (All coin's testnet default)
        nExtCoinType = 1;
   }

    void UpdateBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
    {
        consensus.vDeployments[d].nStartTime = nStartTime;
        consensus.vDeployments[d].nTimeout = nTimeout;
    }
};
static CRegTestParams regTestParams;

static CChainParams *pCurrentParams = 0;

const CChainParams &Params() {
    assert(pCurrentParams);
    return *pCurrentParams;
}

CChainParams& Params(const std::string& chain)
{
    if (chain == CBaseChainParams::MAIN)
            return mainParams;
    else if (chain == CBaseChainParams::TESTNET)
            return testNetParams;
    else if (chain == CBaseChainParams::DEVNET) {
            assert(devNetParams);
            return *devNetParams;
    } else if (chain == CBaseChainParams::REGTEST)
            return regTestParams;
    else
        throw std::runtime_error(strprintf("%s: Unknown chain %s.", __func__, chain));
}

void SelectParams(const std::string& network)
{
    if (network == CBaseChainParams::DEVNET) {
        devNetParams = new CDevNetParams();
    }

    SelectBaseParams(network);
    pCurrentParams = &Params(network);
}

void UpdateRegtestBIP9Parameters(Consensus::DeploymentPos d, int64_t nStartTime, int64_t nTimeout)
{
    regTestParams.UpdateBIP9Parameters(d, nStartTime, nTimeout);
}
