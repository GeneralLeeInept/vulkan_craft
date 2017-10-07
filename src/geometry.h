#pragma once

#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <noise.h>

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

    BlockType& block(int chunk_x, int chunk_y, int chunk_z)
    {
        return blocks[chunk_x + (chunk_z * chunk_size) + (chunk_y * max_height)];
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

    void generate_chunk(int chunk_x, int chunk_y, Chunk& chunk);

private:
    noise::module::Perlin _perlin;
};