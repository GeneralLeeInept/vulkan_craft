#include "geometry.h"

#include <noise.h>

static int block_texture_layers[][6] = {
    { 0, 0, 0, 0, 0, 0 },       // Air
    { 0, 0, 0, 0, 0, 0 },       // Bedrock
    { 1, 1, 1, 1, 1, 1 },       // Brick
    { 2, 2, 2, 2, 2, 2 },       // OreCoal
    { 3, 3, 3, 3, 3, 3 },       // Cobble
    { 4, 4, 4, 4, 4, 4 },       // MossyCobble
    { 15, 15, 15, 15, 15, 15 }, // OreDiamond
    { 16, 16, 16, 16, 16, 16 }, // Dirt
    { 21, 18, 18, 18, 18, 18 }, // Grass
    { 22, 22, 22, 22, 22, 22 }, // OreIron
    { 17, 17, 17, 17, 17, 17 }, // OreGold
    { 23, 23, 23, 23, 23, 23 }, // OreLapis
    { 24, 24, 24, 24, 24, 24 }, // Leaves
    { 26, 26, 25, 25, 25, 25 }, // Log
    { 28, 28, 28, 28, 28, 28 }, // Planks
    { 29, 29, 29, 29, 29, 29 }, // Stone
};

inline float chunk_to_world_x(int cx, int bx)
{
    return (float)(cx * Chunk::chunk_size + bx);
}

inline float chunk_to_world_z(int cz, int bz)
{
    return (float)(cz * -Chunk::chunk_size - bz);
}

static void add_polygon(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, Mesh& mesh)
{
    uint32_t base_index = (uint32_t)mesh.vertices.size();

    for (uint32_t i : indices)
    {
        mesh.indices.push_back(base_index + i);
    }

    for (const Vertex& v : vertices)
    {
        mesh.vertices.push_back(v);
    }
}

/* clang-format off */
static void add_face(int cx, int cz, int bx, int by, int bz, BlockType type, BlockFace face, Mesh& mesh)
{
    float ox = chunk_to_world_x(cx, bx);
    float oy = (float)by;
    float oz = chunk_to_world_z(cz, bz);
    float texture_layer = (float)block_texture_layers[(int)type][(int)face];

    switch (face)
    {
        case BlockFace::Top:
        {
            glm::vec3 normal(0.0f, 1.0f, 0.0f);
            add_polygon(
                {
                    { { ox + 0.0f, oy + 1.0f, oz + 0.0f }, normal, { 0.0f, 1.0f, texture_layer } },
                    { { ox + 1.0f, oy + 1.0f, oz + 0.0f }, normal, { 1.0f, 1.0f, texture_layer } },
                    { { ox + 1.0f, oy + 1.0f, oz - 1.0f }, normal, { 1.0f, 0.0f, texture_layer } },
                    { { ox + 0.0f, oy + 1.0f, oz - 1.0f }, normal, { 0.0f, 0.0f, texture_layer } }
                },
                {
                    0, 1, 2, 0, 2, 3
                }, mesh
            );
            break;
        }
        case BlockFace::Bottom:
        {
            glm::vec3 normal(0.0f, -1.0f, 0.0f);

            add_polygon(
            {
                { { ox + 0.0f, oy + 0.0f, oz + 0.0f }, normal, { 0.0f, 0.0f, texture_layer } },
                { { ox + 0.0f, oy + 0.0f, oz - 1.0f }, normal, { 0.0f, 1.0f, texture_layer } },
                { { ox + 1.0f, oy + 0.0f, oz - 1.0f }, normal, { 1.0f, 1.0f, texture_layer } },
                { { ox + 1.0f, oy + 0.0f, oz + 0.0f }, normal, { 1.0f, 0.0f, texture_layer } }
            },
            {
                0, 1, 2, 0, 2, 3
            }, mesh
            );
            break;
        }
        case BlockFace::North:
        {
            glm::vec3 normal(0.0f, 0.0f, -1.0f);

            add_polygon(
            {
                { { ox + 0.0f, oy + 1.0f, oz - 1.0f }, normal, { 1.0f, 0.0f, texture_layer } },
                { { ox + 1.0f, oy + 1.0f, oz - 1.0f }, normal, { 0.0f, 0.0f, texture_layer } },
                { { ox + 1.0f, oy + 0.0f, oz - 1.0f }, normal, { 0.0f, 1.0f, texture_layer } },
                { { ox + 0.0f, oy + 0.0f, oz - 1.0f }, normal, { 1.0f, 1.0f, texture_layer } }
            },
            {
                0, 1, 2, 0, 2, 3
            }, mesh
            );
            break;
        }
        case BlockFace::East:
        {
            glm::vec3 normal(1.0f, 0.0f, 0.0f);

            add_polygon(
            {
                { { ox + 1.0f, oy + 1.0f, oz + 0.0f }, normal, { 0.0f, 0.0f, texture_layer } },
                { { ox + 1.0f, oy + 0.0f, oz + 0.0f }, normal, { 0.0f, 1.0f, texture_layer } },
                { { ox + 1.0f, oy + 0.0f, oz - 1.0f }, normal, { 1.0f, 1.0f, texture_layer } },
                { { ox + 1.0f, oy + 1.0f, oz - 1.0f }, normal, { 1.0f, 0.0f, texture_layer } }
            },
            {
                0, 1, 2, 0, 2, 3
            }, mesh
            );
            break;
        }
        case BlockFace::South:
        {
            glm::vec3 normal(0.0f, 0.0f, 1.0f);

            add_polygon(
            {
                { { ox + 0.0f, oy + 1.0f, oz + 0.0f }, normal, { 0.0f, 0.0f, texture_layer } },
                { { ox + 0.0f, oy + 0.0f, oz + 0.0f }, normal, { 0.0f, 1.0f, texture_layer } },
                { { ox + 1.0f, oy + 0.0f, oz + 0.0f }, normal, { 1.0f, 1.0f, texture_layer } },
                { { ox + 1.0f, oy + 1.0f, oz + 0.0f }, normal, { 1.0f, 0.0f, texture_layer } }
            },
            {
                0, 1, 2, 0, 2, 3
            }, mesh
            );
            break;
        }
        case BlockFace::West:
        {
            glm::vec3 normal(-1.0f, 0.0f, 0.0f);

            add_polygon(
            {
                { { ox + 0.0f, oy + 1.0f, oz + 0.0f }, normal, { 1.0f, 0.0f, texture_layer } },
                { { ox + 0.0f, oy + 1.0f, oz - 1.0f }, normal, { 0.0f, 0.0f, texture_layer } },
                { { ox + 0.0f, oy + 0.0f, oz - 1.0f }, normal, { 0.0f, 1.0f, texture_layer } },
                { { ox + 0.0f, oy + 0.0f, oz + 0.0f }, normal, { 1.0f, 1.0f, texture_layer } }
            },
            {
                0, 1, 2, 0, 2, 3
            }, mesh
            );
            break;
        }
    }
}
/* clang-format on */

void Chunk::create_mesh()
{
    mesh.vertices.resize(0);
    mesh.indices.resize(0);

    for (int by = 0; by < max_height; by++)
    {
        for (int bz = 0; bz < chunk_size; bz++)
        {
            for (int bx = 0; bx < chunk_size; bx++)
            {
                BlockType block_type = block(bx, by, bz);
                if (block_type != BlockType::Air)
                {
                    if (block(bx, by + 1, bz) == BlockType::Air)
                    {
                        add_face(chunk_x, chunk_z, bx, by, bz, block_type, BlockFace::Top, mesh);
                    }
                    if (block(bx, by - 1, bz) == BlockType::Air)
                    {
                        add_face(chunk_x, chunk_z, bx, by, bz, block_type, BlockFace::Bottom, mesh);
                    }
                    if (block(bx, by, bz + 1) == BlockType::Air)
                    {
                        add_face(chunk_x, chunk_z, bx, by, bz, block_type, BlockFace::North, mesh);
                    }
                    if (block(bx, by, bz - 1) == BlockType::Air)
                    {
                        add_face(chunk_x, chunk_z, bx, by, bz, block_type, BlockFace::South, mesh);
                    }
                    if (block(bx + 1, by, bz) == BlockType::Air)
                    {
                        add_face(chunk_x, chunk_z, bx, by, bz, block_type, BlockFace::East, mesh);
                    }
                    if (block(bx - 1, by, bz) == BlockType::Air)
                    {
                        add_face(chunk_x, chunk_z, bx, by, bz, block_type, BlockFace::West, mesh);
                    }
                }
            }
        }
    }
}

void Chunk::clear()
{
    memset(blocks, 0, sizeof(blocks));
}

WorldGen::WorldGen()
{
    _perlin.SetFrequency(0.005);
    _perlin.SetOctaveCount(3);
}

void WorldGen::generate_chunk(int chunk_x, int chunk_z, Chunk& chunk)
{
    chunk.clear();
    chunk.chunk_x = chunk_x;
    chunk.chunk_z = chunk_z;

    for (int bz = 0; bz < Chunk::chunk_size; bz++)
    {
        for (int bx = 0; bx < Chunk::chunk_size; bx++)
        {
            double x = (double)chunk_to_world_x(chunk_x, bx);
            double z = (double)chunk_to_world_z(chunk_z, bz);
            float noise = (float)_perlin.GetValue(x, 1.0, z);
            int height = 64 + (int)(noise * 31.0f);

            for (int by = 0; by < height; ++by)
            {
                BlockType block_type;
                
                if (by == 0)
                {
                    block_type = BlockType::Bedrock;
                }
                else if (by == height - 1)
                {
                block_type = BlockType::Grass;
                }
                else if (by > height - 8)
                {
                    block_type = BlockType::Dirt;
                }
                else
                {
                    block_type = BlockType::Stone;
                }

                chunk.set_block(bx, by, bz, block_type);
            }
        }
    }
}
