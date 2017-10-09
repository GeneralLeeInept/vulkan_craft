#pragma once

#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace geometry
{
struct plane
{
    // dot(n, p) - d = 0 for all p on plane
    glm::vec3 n;
    float d;
};

struct aabb
{
    void set_from_corners(const glm::vec3& a, const glm::vec3& b);

    glm::vec3 center;
    glm::vec3 extents;
};

struct frustum
{
    void set_from_matrix(const glm::mat4x4& m);

    plane planes[6];
};

// -1 b is in -ve half-space of p, +1 b is in +ve half-space, 0 b intersects p
inline int testAabbPlane(const aabb& b, const plane& p)
{
//    float r = glm::dot(b.extents, glm::abs(p.n));
    float r = b.extents[0] * fabs(p.n[0]) + b.extents[1] * fabs(p.n[1]) + b.extents[2] * fabs(p.n[2]);
    float s = glm::dot(p.n, b.center) - p.d;
    return s < -r ? -1 : s > r ? 1 : 0;
}
} // namespace geometry

namespace culling
{
bool cull(const geometry::frustum& frustum, const geometry::aabb& b); // return true if the aabb is culled
}
