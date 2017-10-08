#include "geometry.h"

#include <noise.h>

#include "renderer.h"

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

// Unit cube vertices, counter-clockwise winding
static glm::vec3 unit_cube_face_verts[6][4] = {
    // Top face (Y = 1)
    { { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f, 0.0f } },

    // Bottom face (Y = 0)
    { { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f } },

    // North face (Z = 0)
    { { 1.0f, 1.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } },

    // South face (Z = 1)
    { { 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f } },

    // East face (X = 1)
    { { 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 0.0f } },

    // West face (X = 0)
    { { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f, 1.0f } }
};

// Unit cube face normals
static glm::vec3 unit_cube_face_normals[6] = {
    // Top face (Y = 1)
    { 0.0f, 1.0f, 0.0f },

    // Bottom face (Y = 0)
    { 0.0f, -1.0f, 0.0f },

    // North face (Z = 0)
    { 0.0f, 0.0f, -1.0f },

    // South face (Z = 1)
    { 0.0f, 0.0f, 1.0f },

    // East face (X = 1)
    { 1.0f, 0.0f, 0.0f },

    // West face (X = 0)
    { 0.0f, 1.0f, 0.0f },
};

static void add_face(int cx, int cz, int bx, int by, int bz, BlockType type, BlockFace face, Mesh& mesh)
{
    double dox, doz;
    chunk_to_world(cx, cz, bx, bz, dox, doz);
    glm::vec3 origin((float)dox, (float)by, (float)doz);
    float texture_layer = (float)block_texture_layers[(int)type][(int)face];

    add_polygon({ { origin + unit_cube_face_verts[(int)face][0], unit_cube_face_normals[(int)face], { 0.0f, 0.0f, texture_layer } },
                  { origin + unit_cube_face_verts[(int)face][1], unit_cube_face_normals[(int)face], { 0.0f, 1.0f, texture_layer } },
                  { origin + unit_cube_face_verts[(int)face][2], unit_cube_face_normals[(int)face], { 1.0f, 1.0f, texture_layer } },
                  { origin + unit_cube_face_verts[(int)face][3], unit_cube_face_normals[(int)face], { 1.0f, 0.0f, texture_layer } } },
                { 0, 1, 2, 0, 2, 3 }, mesh);
}

static inline bool is_transparent(BlockType block_type)
{
    return block_type == BlockType::Air;
}

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
                    if (is_transparent(block(bx, by + 1, bz)))
                    {
                        add_face(origin_x, origin_z, bx, by, bz, block_type, BlockFace::Top, mesh);
                    }
                    if (is_transparent(block(bx, by - 1, bz)))
                    {
                        add_face(origin_x, origin_z, bx, by, bz, block_type, BlockFace::Bottom, mesh);
                    }
                    if (is_transparent(block(bx, by, bz - 1)))
                    {
                        add_face(origin_x, origin_z, bx, by, bz, block_type, BlockFace::North, mesh);
                    }
                    if (is_transparent(block(bx, by, bz + 1)))
                    {
                        add_face(origin_x, origin_z, bx, by, bz, block_type, BlockFace::South, mesh);
                    }
                    if (is_transparent(block(bx + 1, by, bz)))
                    {
                        add_face(origin_x, origin_z, bx, by, bz, block_type, BlockFace::East, mesh);
                    }
                    if (is_transparent(block(bx - 1, by, bz)))
                    {
                        add_face(origin_x, origin_z, bx, by, bz, block_type, BlockFace::West, mesh);
                    }
                }
            }
        }
    }

    double dox, doz;
    chunk_to_world(origin_x, origin_z, 0, 0, dox, doz);
    glm::vec3 a = glm::vec3((float)dox, 0.0f, (float)doz);
    glm::vec3 b = a + glm::vec3((float)chunk_size, (float)max_height, (float)chunk_size);
    mesh.aabb.set_from_corners(a, b);
}

void Chunk::clear()
{
    memset(blocks, 0, sizeof(blocks));
}

float Chunk::get_height(int x, int z)
{
    if (in_bounds(x, 0, z))
    {
        for (int y = max_height; y > 0; --y)
        {
            if (block(x, y - 1, z) != BlockType::Air)
            {
                return (float)y;
            }
        }
    }

    return 0.0f;
}

WorldGen::WorldGen(Renderer& renderer)
    : _renderer(renderer)
{
    _perlin.SetFrequency(0.005);
    _perlin.SetOctaveCount(3);
}

float WorldGen::get_height(double x, double z)
{
    int cx, cz, bx, bz;
    world_to_chunk(x, z, cx, cz, bx, bz);
    return get_chunk(cx, cz).get_height(bx, bz);
}

Chunk& WorldGen::get_chunk(int chunk_x, int chunk_z)
{
    IntCoord pos = { chunk_x, chunk_z };
    ChunkMap::iterator it = _chunks.find(pos);

    if (it == _chunks.end())
    {
        generate_chunk(pos.x, pos.z);
    }

    return _chunks[pos];
}

void WorldGen::generate_chunk(int chunk_x, int chunk_z)
{
    IntCoord pos = { chunk_x, chunk_z };
    Chunk& chunk = _chunks[pos];
    chunk.clear();
    chunk.origin_x = chunk_x;
    chunk.origin_z = chunk_z;

    for (int bz = 0; bz < Chunk::chunk_size; bz++)
    {
        for (int bx = 0; bx < Chunk::chunk_size; bx++)
        {
            double x, z;
            chunk_to_world(chunk_x, chunk_z, bx, bz, x, z);
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

    chunk.create_mesh();
    _renderer.add_mesh(chunk.mesh);
}

void WorldGen::generate_around(double x, double z, int radius)
{
    int cx, cz;
    world_to_chunk(x, z, cx, cz);

    for (int z = -radius; z <= radius; ++z)
    {
        for (int x = -radius; x <= radius; ++x)
        {
            get_chunk(cx + x, cz + z);
        }
    }
}
