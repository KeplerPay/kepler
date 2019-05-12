// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2019 The Kepler developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "primitives/block.h"

#include "hash.h"
#include "tinyformat.h"
#include "utilstrencodings.h"
#include "crypto/common.h"

#include "crypto/neoscrypt/neoscrypt.h"
#include "crypto/hashargon2d.h"


#define BEGIN(a)            ((char*)&(a))
#define END(a)              ((char*)&((&(a))[1]))

uint256 CBlockHeader::GetHash() const
{
    return SerializeHash(*this); // SHA256
    //return HashX11(BEGIN(nVersion), END(nNonce));// idk why I did this in monea     return Hash(BEGIN(nVersion), END(nNonce));
}

uint256 CBlockHeader::GetPoWHash(int algo) const
{
    strprintf("DEBUG: GetPoWHash %d \n",algo);
    switch (algo)
    {
        case ALGO_SLOT1:
        {
            uint256 thash;
            unsigned int profile = 0x0;
            neoscrypt((unsigned char *) &nVersion, (unsigned char *) &thash, profile);
            return thash;
        }
        case ALGO_SLOT2:
            return HashArgon2d(BEGIN(nVersion), END(nNonce));
        case ALGO_SLOT3:
        {    
            return RainforestV2(BEGIN(nVersion), END(nNonce));
        }   
    }
    // catch-all if above doesn't match anything to algo
    uint256 thash;
    unsigned int profile = 0x0;
    neoscrypt((unsigned char *) &nVersion, (unsigned char *) &thash, profile);
    return thash;
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, pow_algo=%d, pow_hash=%s, hashPrevBlock=%s, hashMerkleRoot=%s, nTime=%u, nBits=%08x, nNonce=%u, vtx=%u)\n",
        GetHash().ToString(),
        nVersion,
        GetAlgo(),
        GetPoWHash(GetAlgo()).ToString(),
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        nTime, nBits, nNonce,
        vtx.size());
    for (unsigned int i = 0; i < vtx.size(); i++)
    {
        s << "  " << vtx[i]->ToString() << "\n";
    }
    return s.str();
}

int GetAlgo(int nVersion)
{
    switch (nVersion & BLOCK_VERSION_ALGO)
    {
        case 0:
            return ALGO_SLOT1;
        case BLOCK_VERSION_SLOT2:
            return ALGO_SLOT2;
        case BLOCK_VERSION_SLOT3:
            return ALGO_SLOT3;           
    }
    return ALGO_SLOT1;
}
