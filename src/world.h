#pragma once

#include <map>
#include <stdint.h>

struct Voxel
{
    enum class Type : uint16_t
    {
        Air,
        Bedrock,
        Brick,
        OreCoal,
        Cobble,
        MossyCobble,
        OreDiamond,
        Dirt,
        Grass,
        OreIron,
        OreGold,
        OreLapis,
        Leaves,
        Log,
        Planks,
        Stone
    };

    Type type;
};

struct Chunk
{
    static const uint32_t size = 32;
    static const uint32_t height = 256;
    Voxel voxels[size * size * height];

    Voxel& operator()(uint32_t x, uint32_t y, uint32_t z)
    {
        return voxels[y * size * size + z * size + x];
    }
};

struct ChunkMap
{
    typedef uint64_t Key;
    Key key(int32_t x, int32_t z) { return ((((uint64_t)x) << 32) & 0xffffffff00000000) | (((uint32_t)z) & 0xffffffff); }
    std::map<uint64_t, Chunk> chunks;
};