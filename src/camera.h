#pragma once

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Camera
{
public:
    void move_forward(float speed, float dt)
    {
        glm::vec3 move = { sinf(glm::radians(yaw)), 0.0f, cosf(glm::radians(yaw)) };
        position += move * speed * dt;
    }

    void move_up(float speed, float dt)
    {
        position.y += speed * dt;
    }

    void strafe(float speed, float dt)
    {
        glm::vec3 move = { cosf(glm::radians(yaw)), 0.0f, -sinf(glm::radians(yaw)) };
        position += move * speed * dt;
    }

    void turn(float yaw_speed, float pitch_speed, float dt)
    {
        yaw -= yaw_speed * dt;

        while (yaw > 180.0f)
        {
            yaw -= 360.0f;
        }

        while (yaw < -180.0f)
        {
            yaw += 360.0f;
        }

        pitch -= pitch_speed * dt;

        if (pitch > 90.0f)
        {
            pitch = 90.0f;
        }

        if (pitch < -90.0f)
        {
            pitch = -90.0f;
        }
    }

    glm::mat4x4 get_view_matrix()
    {
        glm::mat4x4 camera_world;
        camera_world = glm::rotate(camera_world, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
        camera_world = glm::rotate(camera_world, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));
        camera_world[3] = glm::vec4(position, 1.0f);
        return glm::inverse(camera_world);
    }

    float pitch = 0.0f;
    float yaw = 0.0f;
    glm::vec3 position = glm::vec3();
};
