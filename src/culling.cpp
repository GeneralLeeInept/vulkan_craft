#include "culling.h"

#include <glm/common.hpp>
#include <glm/geometric.hpp>

namespace geometry
{
void aabb::set_from_corners(const glm::vec3& a, const glm::vec3& b)
{
    extents = glm::abs(a - b) * 0.5f;
    center = (a + b) * 0.5f;
}

void frustum::set_from_matrix(const glm::mat4x4& m)
{
    planes[0] = { { m[0][3] - m[0][0], m[1][3] - m[1][0], m[2][3] - m[2][0] }, -(m[3][3] - m[3][0]) };
    planes[1] = { { m[0][3] + m[0][0], m[1][3] + m[1][0], m[2][3] + m[2][0] }, -(m[3][3] + m[3][0]) };
    planes[2] = { { m[0][3] - m[0][1], m[1][3] - m[1][1], m[2][3] - m[2][1] }, -(m[3][3] - m[3][1]) };
    planes[3] = { { m[0][3] + m[0][1], m[1][3] + m[1][1], m[2][3] + m[2][1] }, -(m[3][3] + m[3][1]) };
    planes[4] = { { m[0][3] - m[0][2], m[1][3] - m[1][2], m[2][3] - m[2][2] }, -(m[3][3] - m[3][2]) };
    planes[5] = { { m[0][3] + m[0][2], m[1][3] + m[1][2], m[2][3] + m[2][2] }, -(m[3][3] + m[3][2]) };
}
} // namespace geometry

namespace culling
{
bool cull(const geometry::frustum& frustum, const geometry::aabb& b)
{
    for (const geometry::plane& p : frustum.planes)
    {
        if (geometry::testAabbPlane(b, p) < 0)
        {
            return true;
        }
    }

    return false;
}
}
