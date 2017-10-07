#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <noise.h>
#include <map>
#include <vector>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 tex_coord;
};

struct Mesh
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

enum class BlockFace : uint8_t
{
    Top,
    Bottom,
    North,
    South,
    East,
    West
};

enum class BlockType : uint8_t
{
    Air,
    Brick,
    OreCoal,
    Cobble,
    MossyCobble,
    OreDiamond,
    Dirt,
    Grass,
    OreIron,
    OreLapis,
    Leaves,
    Log,
    Planks
};

class Chunk
{
public:
    static const int chunk_size = 16;
    static const int max_height = 256;

    BlockType blocks[chunk_size * chunk_size * max_height];

    static inline bool in_bounds(int x, int y, int z)
    {
        return (x >= 0 && x < chunk_size && z >= 0 && z < chunk_size && y >= 0 && y < max_height);
    }

    static inline int block_index(int x, int y, int z)
    {
        return x + (z * chunk_size) + (y * chunk_size * chunk_size);
    }

    BlockType block(int x, int y, int z)
    {
        if (in_bounds(x, y, z))
        {
            return blocks[block_index(x, y, z)];
        }

        return BlockType::Air;
    }

    void set_block(int x, int y, int z, BlockType block_type)
    {
        if (in_bounds(x, y, z))
        {
            blocks[block_index(x, y, z)] = block_type;
        }
    }

    void clear();
    void create_mesh();

    Mesh mesh;
    int chunk_x = 0;
    int chunk_z = 0;
};

class WorldGen
{
public:
    WorldGen();

    Chunk& get_chunk(int x, int z);

    void generate_chunk(int chunk_x, int chunk_y, Chunk& chunk);

private:
    noise::module::Perlin _perlin;

    struct IntCoord
    {
        int x, z;
    };
    
    typedef std::map<IntCoord, Chunk> ChunkMap;
    ChunkMap chunks;
};