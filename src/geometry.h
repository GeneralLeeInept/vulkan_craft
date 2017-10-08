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

class Chunk
{
public:
    static const int chunk_size = 64;
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

    float get_height(int x, int z);

    Mesh mesh;
    int origin_x = 0;
    int origin_z = 0;
};

class Renderer;

class WorldGen
{
public:
    WorldGen(Renderer& renderer);

    float get_height(double x, double z);

    Chunk& get_chunk(int chunk_x, int chunk_z);
    void generate_around(double x, double z, int radius);

private:
    void generate_chunk(int chunk_x, int chunk_z);

    noise::module::Perlin _perlin;
    Renderer& _renderer;

    struct IntCoord
    {
        int x, z;
    };
    
    struct IntCoordCompare
    {
        bool operator()(const IntCoord& a, const IntCoord& b) const
        {
            return (a.x != b.x) ? (a.x < b.x) : (a.z < b.z);
        }
    };

    typedef std::map<IntCoord, Chunk, IntCoordCompare> ChunkMap;
    ChunkMap _chunks;
};

inline void world_to_chunk(double world_x, double world_z, int& chunk_x, int& chunk_z)
{
    chunk_x = (int)floor(world_x / (double)Chunk::chunk_size);
    chunk_z = (int)floor(world_z / (double)Chunk::chunk_size);
}

inline void world_to_chunk(double world_x, double world_z, int& chunk_x, int& chunk_z, int& block_x, int& block_z)
{
    world_to_chunk(world_x, world_z, chunk_x, chunk_z);
    block_x = (int)floor(world_x - chunk_x * (double)Chunk::chunk_size);
    block_z = (int)floor(world_z - chunk_z * (double)Chunk::chunk_size);
}

inline void chunk_to_world(int chunk_x, int chunk_z, int block_x, int block_z, double& world_x, double& world_z)
{
    world_x = chunk_x * Chunk::chunk_size + block_x;
    world_z = chunk_z * Chunk::chunk_size + block_z;
}
