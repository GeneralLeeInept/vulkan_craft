#include "geometry.h"

void add_polygon(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, Mesh& mesh)
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
void add_face(int cx, int cz, int bx, int by, int bz, BlockFace face, Mesh& mesh)
{
    float ox = (float)(cx * Chunk::chunk_size + bx);
    float oy = (float)by;
    float oz = (float)(cz * Chunk::chunk_size - bz);

    switch (face)
    {
        case BlockFace::Top:
        {
            add_polygon(
                {
                    { { ox + 0.0f, oy + 1.0f, oz + 0.0f }, { 0.0f, 1.0f } },
                    { { ox + 1.0f, oy + 1.0f, oz + 0.0f }, { 1.0f, 1.0f } },
                    { { ox + 1.0f, oy + 1.0f, oz - 1.0f }, { 1.0f, 0.0f } },
                    { { ox + 0.0f, oy + 1.0f, oz - 1.0f }, { 0.0f, 0.0f } }
                },
                {
                    0, 1, 2, 0, 2, 3
                }, mesh
            );
            break;
        }
        case BlockFace::Bottom:
        {
            add_polygon(
            {
                { { ox + 0.0f, oy + 0.0f, oz + 0.0f }, { 0.0f, 0.0f } },
                { { ox + 0.0f, oy + 0.0f, oz - 1.0f }, { 0.0f, 1.0f } },
                { { ox + 1.0f, oy + 0.0f, oz - 1.0f }, { 1.0f, 1.0f } },
                { { ox + 1.0f, oy + 0.0f, oz + 0.0f }, { 1.0f, 0.0f } }
            },
            {
                0, 1, 2, 0, 2, 3
            }, mesh
            );
            break;
        }
        case BlockFace::North:
        {
            add_polygon(
            {
                { { ox + 0.0f, oy + 1.0f, oz - 1.0f }, { 1.0f, 0.0f } },
                { { ox + 1.0f, oy + 1.0f, oz - 1.0f }, { 0.0f, 0.0f } },
                { { ox + 1.0f, oy + 0.0f, oz - 1.0f }, { 0.0f, 1.0f } },
                { { ox + 0.0f, oy + 0.0f, oz - 1.0f }, { 1.0f, 1.0f } }
            },
            {
                0, 1, 2, 0, 2, 3
            }, mesh
            );
            break;
        }
        case BlockFace::East:
        {
            add_polygon(
            {
                { { ox + 1.0f, oy + 1.0f, oz + 0.0f }, { 0.0f, 0.0f } },
                { { ox + 1.0f, oy + 0.0f, oz + 0.0f }, { 0.0f, 1.0f } },
                { { ox + 1.0f, oy + 0.0f, oz - 1.0f }, { 1.0f, 1.0f } },
                { { ox + 1.0f, oy + 1.0f, oz - 1.0f }, { 1.0f, 0.0f } }
            },
            {
                0, 1, 2, 0, 2, 3
            }, mesh
            );
            break;
        }
        case BlockFace::South:
        {
            add_polygon(
            {
                { { ox + 0.0f, oy + 1.0f, oz + 0.0f }, { 0.0f, 0.0f } },
                { { ox + 0.0f, oy + 0.0f, oz + 0.0f }, { 0.0f, 1.0f } },
                { { ox + 1.0f, oy + 0.0f, oz + 0.0f }, { 1.0f, 1.0f } },
                { { ox + 1.0f, oy + 1.0f, oz + 0.0f }, { 1.0f, 0.0f } }
            },
            {
                0, 1, 2, 0, 2, 3
            }, mesh
            );
            break;
        }
        case BlockFace::West:
        {
            add_polygon(
            {
                { { ox + 0.0f, oy + 1.0f, oz + 0.0f }, { 1.0f, 0.0f } },
                { { ox + 0.0f, oy + 1.0f, oz - 1.0f }, { 0.0f, 0.0f } },
                { { ox + 0.0f, oy + 0.0f, oz - 1.0f }, { 0.0f, 1.0f } },
                { { ox + 0.0f, oy + 0.0f, oz + 0.0f }, { 1.0f, 1.0f } }
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

void Chunk::generate()
{
    for (int by = 0; by < max_height; by++)
    {
        for (int bz = 0; bz < chunk_size; bz++)
        {
            for (int bx = 0; bx < chunk_size; bx++)
            {
                block(bx, by, bz) = BlockType::Cobble;
            }
        }
    }
}
