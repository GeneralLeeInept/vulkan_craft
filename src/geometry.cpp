#include "geometry.h"

#include <noise.h>

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
static void add_face(int cx, int cz, int bx, int by, int bz, BlockFace face, Mesh& mesh)
{
    float ox = (float)(cx * Chunk::chunk_size + bx);
    float oy = (float)by;
    float oz = (float)(cz * Chunk::chunk_size - bz);

    switch (face)
    {
        case BlockFace::Top:
        {
            glm::vec3 normal(0.0f, 1.0f, 0.0f);

            add_polygon(
                {
                    { { ox + 0.0f, oy + 1.0f, oz + 0.0f }, normal, { 0.0f, 1.0f } },
                    { { ox + 1.0f, oy + 1.0f, oz + 0.0f }, normal, { 1.0f, 1.0f } },
                    { { ox + 1.0f, oy + 1.0f, oz - 1.0f }, normal, { 1.0f, 0.0f } },
                    { { ox + 0.0f, oy + 1.0f, oz - 1.0f }, normal, { 0.0f, 0.0f } }
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
                { { ox + 0.0f, oy + 0.0f, oz + 0.0f }, normal, { 0.0f, 0.0f } },
                { { ox + 0.0f, oy + 0.0f, oz - 1.0f }, normal, { 0.0f, 1.0f } },
                { { ox + 1.0f, oy + 0.0f, oz - 1.0f }, normal, { 1.0f, 1.0f } },
                { { ox + 1.0f, oy + 0.0f, oz + 0.0f }, normal, { 1.0f, 0.0f } }
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
                { { ox + 0.0f, oy + 1.0f, oz - 1.0f }, normal, { 1.0f, 0.0f } },
                { { ox + 1.0f, oy + 1.0f, oz - 1.0f }, normal, { 0.0f, 0.0f } },
                { { ox + 1.0f, oy + 0.0f, oz - 1.0f }, normal, { 0.0f, 1.0f } },
                { { ox + 0.0f, oy + 0.0f, oz - 1.0f }, normal, { 1.0f, 1.0f } }
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
                { { ox + 1.0f, oy + 1.0f, oz + 0.0f }, normal, { 0.0f, 0.0f } },
                { { ox + 1.0f, oy + 0.0f, oz + 0.0f }, normal, { 0.0f, 1.0f } },
                { { ox + 1.0f, oy + 0.0f, oz - 1.0f }, normal, { 1.0f, 1.0f } },
                { { ox + 1.0f, oy + 1.0f, oz - 1.0f }, normal, { 1.0f, 0.0f } }
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
                { { ox + 0.0f, oy + 1.0f, oz + 0.0f }, normal, { 0.0f, 0.0f } },
                { { ox + 0.0f, oy + 0.0f, oz + 0.0f }, normal, { 0.0f, 1.0f } },
                { { ox + 1.0f, oy + 0.0f, oz + 0.0f }, normal, { 1.0f, 1.0f } },
                { { ox + 1.0f, oy + 1.0f, oz + 0.0f }, normal, { 1.0f, 0.0f } }
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
                { { ox + 0.0f, oy + 1.0f, oz + 0.0f }, normal, { 1.0f, 0.0f } },
                { { ox + 0.0f, oy + 1.0f, oz - 1.0f }, normal, { 0.0f, 0.0f } },
                { { ox + 0.0f, oy + 0.0f, oz - 1.0f }, normal, { 0.0f, 1.0f } },
                { { ox + 0.0f, oy + 0.0f, oz + 0.0f }, normal, { 1.0f, 1.0f } }
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
                if (block(bx, by, bz) != BlockType::Air)
                {
                    add_face(chunk_x, chunk_z, bx, by, bz, BlockFace::Top, mesh);
                    add_face(chunk_x, chunk_z, bx, by, bz, BlockFace::Bottom, mesh);
                    add_face(chunk_x, chunk_z, bx, by, bz, BlockFace::North, mesh);
                    add_face(chunk_x, chunk_z, bx, by, bz, BlockFace::South, mesh);
                    add_face(chunk_x, chunk_z, bx, by, bz, BlockFace::East, mesh);
                    add_face(chunk_x, chunk_z, bx, by, bz, BlockFace::West, mesh);
                }
            }
        }
    }
}

void Chunk::clear()
{
    memset(blocks, 0, sizeof(blocks));
}

void WorldGen::generate_chunk(int chunk_x, int chunk_z, Chunk& chunk)
{
    chunk.clear();
    chunk.chunk_x = chunk_x;
    chunk.chunk_z = chunk_z;

    _perlin.SetFrequency(0.01);

    for (int bz = 0; bz < Chunk::chunk_size; bz++)
    {
        for (int bx = 0; bx < Chunk::chunk_size; bx++)
        {
            float noise = (float)_perlin.GetValue((double)(bx + chunk_x * Chunk::chunk_size), 1.0, (double)(chunk_z * Chunk::chunk_size - bz));
            int height = 64 + (int)(noise * 63.0f);

            for (int by = 0; by < height; ++by)
            {
                chunk.block(bx, by, bz) = BlockType::Dirt;
            }
        }
    }
}
