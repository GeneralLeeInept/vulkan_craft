#pragma once

#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace geometry
{
    struct plane
    {
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
        void set_from_matrix(const glm::mat4x4& proj);

        plane planes[6];
    };
}

namespace culling
{
bool cull(const geometry::frustum& frustum, const geometry::aabb& aabb);  // return true if the aabb is culled
}
