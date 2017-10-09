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

void frustum::set_from_matrix(const glm::mat4x4& proj)
{
    planes[0] = { { proj[0].w - proj[0].x, proj[1].w - proj[1].x, proj[2].w - proj[2].x }, proj[3].w - proj[3].x };
    planes[1] = { { proj[0].w + proj[0].x, proj[1].w + proj[1].x, proj[2].w + proj[2].x }, proj[3].w + proj[3].x };
    planes[2] = { { proj[0].w - proj[0].y, proj[1].w - proj[1].y, proj[2].w - proj[2].y }, proj[3].w - proj[3].y };
    planes[3] = { { proj[0].w + proj[0].y, proj[1].w + proj[1].y, proj[2].w + proj[2].y }, proj[3].w + proj[3].y };
    planes[4] = { { proj[0].w - proj[0].z, proj[1].w - proj[1].z, proj[2].w - proj[2].z }, proj[3].w - proj[3].z };
    planes[5] = { { proj[0].w + proj[0].z, proj[1].w + proj[1].z, proj[2].w + proj[2].z }, proj[3].w + proj[3].z };
}
} // namespace geometry

namespace culling
{
bool cull(const geometry::frustum& frustum, const geometry::aabb& aabb)
{
    glm::vec3 a = aabb.center - aabb.extents;
    glm::vec3 b = aabb.center + aabb.extents;

    for (const geometry::plane& plane : frustum.planes)
    {
        if ((glm::dot(plane.n, a) + plane.d) > 0 && (glm::dot(plane.n, b) + plane.d) > 0)
        {
            return true;
        }
    }

    return false;
}
}
