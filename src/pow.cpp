// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2019 The Kepler developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "pow.h"

#include "arith_uint256.h"
#include "chain.h"
#include "chainparams.h"
#include "primitives/block.h"
#include "uint256.h"
#include "util.h"

#include <math.h>

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, int algo, const Consensus::Params& params)
{
    const arith_uint256 nProofOfWorkLimit = UintToArith256(params.powLimit[algo]);

    // Genesis block
    if (pindexLast == NULL)
        return nProofOfWorkLimit.GetCompact();

    // find previous block with same algo
    const CBlockIndex* pindexPrev = GetLastBlockIndexForAlgo(pindexLast, algo);
    if (pindexPrev == NULL)
        return nProofOfWorkLimit.GetCompact();

    const CBlockIndex* pindexFirst = pindexPrev;

    // Go back by what we want to be nAveragingInterval blocks
    for (int i = 0; pindexFirst && i < params.nPoWAveragingInterval - 1; i++)
    {
        pindexFirst = pindexFirst->pprev;
        pindexFirst = GetLastBlockIndexForAlgo(pindexFirst, algo);
        if (pindexFirst == NULL){   
            return nProofOfWorkLimit.GetCompact();
            //LogPrintf("GetNextWorkRequired(%d):   pindexFirst null, returning powLimit  %d \n", algo, i);
        }   
    }

    //pindexFirst: 10 blocks before pindexPrev, pindexLast: latest block, pindexPrev: latest block for algo
    int64_t nActualTimespan = pindexPrev->GetMedianTimePast() - pindexFirst->GetMedianTimePast();

    //LogPrintf("GetNextWorkRequired(%d):   nActualTimespan = %d before bounds   %d   %d\n", algo, nActualTimespan, pindexPrev->GetMedianTimePast(), pindexFirst->GetMedianTimePast());
    
    // Initial mining phase, allow up to 20% difficulty change per block
    int64_t nMinActualTimespan;
    int64_t nMaxActualTimespan;

    if (pindexLast->nHeight < 2999){    
        nMinActualTimespan = params.nPoWAveragingTargetTimespan() * (100 - 20) / 100;
        nMaxActualTimespan = params.nPoWAveragingTargetTimespan() * (100 + 20) / 100;
    }else{    
        nMinActualTimespan = params.nPoWAveragingTargetTimespan() * (100 - params.nMaxAdjustUp) / 100;
        nMaxActualTimespan = params.nPoWAveragingTargetTimespan() * (100 + params.nMaxAdjustDown) / 100;
    }

    if (nActualTimespan < nMinActualTimespan)
        nActualTimespan = nMinActualTimespan;
    if (nActualTimespan > nMaxActualTimespan)
        nActualTimespan = nMaxActualTimespan;

    //LogPrintf("GetNextWorkRequired(%d):   nActualTimespan = %d after bounds   %d   %d\n", algo, nActualTimespan, nMinActualTimespan, nMaxActualTimespan);

    arith_uint256 bnNew;
    bnNew.SetCompact(pindexPrev->nBits);
    arith_uint256 bnOld;
    bnOld = bnNew;

    bnNew *= nActualTimespan;
    bnNew /= params.nPoWAveragingTargetTimespan();
    if(bnNew > nProofOfWorkLimit)
        bnNew = nProofOfWorkLimit;

    //LogPrintf("GetNextWorkRequired(Algo=%d) RETARGET\n", algo);
    //LogPrintf("nTargetTimespan = %d    nActualTimespan = %d\n", params.nPoWAveragingTargetTimespan(), nActualTimespan);
    //LogPrintf("Before: %08x  %s\n", pindexPrev->nBits, bnOld.ToString());
    //LogPrintf("After:  %08x  %s\n", bnNew.GetCompact(), bnNew.ToString());

    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, int algo, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit[algo]))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}

// BASED OF MYRIADCOIN
arith_uint256 GetPrevWorkForAlgoWithDecay(const CBlockIndex& block, int algo)
{
    int nDistance = 0;
    arith_uint256 nWork;
    const CBlockIndex* pindex = &block;
    while (pindex != NULL)
    {
        if (nDistance > 100)
        {
            return arith_uint256(0);
        }
        if (pindex->GetAlgo() == algo)
        {
            arith_uint256 nWork = GetBlockProofBase(*pindex);
            nWork *= (100 - nDistance);
            nWork /= 100;
            return nWork;
        }
        pindex = pindex->pprev;
        nDistance++;
    }
    return arith_uint256(0);
}


arith_uint256 uint256_nthRoot(const int root, const arith_uint256 bn)
{
    assert(root > 1);
    if (bn==0)
        return 0;
    assert(bn > 0);

    // starting approximation
    int nRootBits = (bn.bits() + root - 1) / root;
    int nStartingBits = std::min(8, nRootBits);
    arith_uint256 bnUpper = bn;
    bnUpper >>= (nRootBits - nStartingBits)*root;
    arith_uint256 bnCur = 0;
    for (int i = nStartingBits - 1; i >= 0; i--) {
        arith_uint256 bnNext = bnCur;
        bnNext += 1 << i;
        arith_uint256 bnPower = 1;
        for (int j = 0; j < root; j++)
            bnPower *= bnNext;
        if (bnPower <= bnUpper)
            bnCur = bnNext;
    }
    if (nRootBits == nStartingBits)
        return bnCur;
    bnCur <<= nRootBits - nStartingBits;

    // iterate: cur = cur + (bn / cur^^(root-1) - cur)/root
    arith_uint256 bnDelta;
    const arith_uint256 bnRoot = root;
    int nTerminate = 0;
    bool fNegativeDelta = false;
    // this should always converge in fewer steps, but limit just in case
    for (int it = 0; it < 20; it++)
    {
        arith_uint256 bnDenominator = 1;
        for (int i = 0; i < root - 1; i++)
            bnDenominator *= bnCur;
        if (bnCur > bn/bnDenominator)
            fNegativeDelta = true;
        if (bnCur == bn/bnDenominator)  // bnDelta=0
            return bnCur;
        if (fNegativeDelta) {
            bnDelta = bnCur - bn/bnDenominator;
            if (nTerminate == 1)
                return bnCur - 1;
            fNegativeDelta = false;
            if (bnDelta <= bnRoot) {
                bnCur -= 1;
                nTerminate = -1;
                continue;
            }
            fNegativeDelta = true;
        } else {
            bnDelta = bn/bnDenominator - bnCur;
            if (nTerminate == -1)
                return bnCur;
            if (bnDelta <= bnRoot) {
                bnCur += 1;
                nTerminate = 1;
                continue;
            }
        }
        if (fNegativeDelta) {
            bnCur -= bnDelta / bnRoot;
        } else {
            bnCur += bnDelta / bnRoot;
        }
        nTerminate = 0;
    }
    return bnCur;
}

arith_uint256 GetGeometricMeanPrevWork(const CBlockIndex& block)
{
    arith_uint256 bnRes;
    arith_uint256 nBlockWork = GetBlockProofBase(block);
    int nAlgo = block.GetAlgo();

    // Compute the geometric mean
    // We use the nthRoot product rule: nthroot(a*b*...) = nthroot(a)*nthroot(b)*...
    // TWe use the product rule to ensure we never overflow a uint256.
    nBlockWork = uint256_nthRoot(NUM_ALGOS, nBlockWork);

    for (int algo = 0; algo < NUM_ALGOS; algo++)
    {
        if (algo != nAlgo)
        {
            arith_uint256 nBlockWorkAlt = GetPrevWorkForAlgoWithDecay(block, algo);
            if (nBlockWorkAlt != 0)
                nBlockWork *= uint256_nthRoot(NUM_ALGOS,nBlockWorkAlt);
        }
    }
    // Compute the geometric mean
    bnRes = nBlockWork;

    // Scale to roughly match the old work calculation
    bnRes <<= 8;

    return bnRes;
}

arith_uint256 GetBlockProofBase(const CBlockIndex& block)
{
    arith_uint256 bnTarget;
    bool fNegative;
    bool fOverflow;
    bnTarget.SetCompact(block.nBits, &fNegative, &fOverflow);
    if (fNegative || fOverflow || bnTarget == 0)
        return 0;
    // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
    // as it's too large for a arith_uint256. However, as 2**256 is at least as large
    // as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) / (bnTarget+1)) + 1,
    // or ~bnTarget / (nTarget+1) + 1.
    return (~bnTarget / (bnTarget + 1)) + 1;
}

arith_uint256 GetBlockProof(const CBlockIndex& block)
{
    return GetGeometricMeanPrevWork(block);
}

int64_t GetBlockProofEquivalentTime(const CBlockIndex& to, const CBlockIndex& from, const CBlockIndex& tip, const Consensus::Params& params)
{
    arith_uint256 r;
    int sign = 1;
    if (to.nChainWork > from.nChainWork) {
        r = to.nChainWork - from.nChainWork;
    } else {
        r = from.nChainWork - to.nChainWork;
        sign = -1;
    }
    r = r * arith_uint256(params.nPowTargetSpacing) / GetBlockProof(tip);
    if (r.bits() > 63) {
        return sign * std::numeric_limits<int64_t>::max();
    }
    return sign * r.GetLow64();
}
