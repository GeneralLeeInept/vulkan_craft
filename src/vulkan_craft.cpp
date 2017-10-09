#include <Windows.h>
#include <stdlib.h>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include "camera.h"
#include "geometry.h"
#include "renderer.h"

void run_game(GLFWwindow* window);
void set_window_size(GLFWwindow* window, int width, int height);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    wchar_t cwd[MAX_PATH];
    GetCurrentDirectory(MAX_PATH, cwd);

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, false);
    GLFWwindow* window = glfwCreateWindow(1024, 576, "VulkanCraft", nullptr, nullptr);
    glfwSetWindowSizeCallback(window, set_window_size);

    run_game(window);

    glfwDestroyWindow(window);
    glfwTerminate();

    return EXIT_SUCCESS;
}

Renderer _renderer;
Camera _camera;
float _mouse_x;
float _mouse_y;
WorldGen _world_gen(_renderer);

void poll_mouse(GLFWwindow* window, float& x, float& y)
{
    double dx, dy;
    glfwGetCursorPos(window, &dx, &dy);
    x = (float)dx;
    y = (float)dy;
}

void update_input(GLFWwindow* window, float delta)
{
    float new_mouse_x;
    float new_mouse_y;
    poll_mouse(window, new_mouse_x, new_mouse_y);
    _camera.turn((new_mouse_x - _mouse_x) * 2.0f, (new_mouse_y - _mouse_y) * 2.0f, delta);
    _mouse_x = new_mouse_x;
    _mouse_y = new_mouse_y;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        _camera.move_forward(5.0f, delta);
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        _camera.move_forward(-5.0f, delta);
    }

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        _camera.strafe(5.0f, delta);
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        _camera.strafe(-5.0f, delta);
    }

    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
    {
        _camera.move_up(5.0f, delta);
    }

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
    {
        _camera.move_up(-5.0f, delta);
    }
}

extern bool UpdateClipFrustum;

void run_game(GLFWwindow* window)
{
    if (!window)
    {
        return;
    }

    if (!_renderer.initialise(window))
    {
        return;
    }

    glfwShowWindow(window);
    
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    float prev_time = (float)glfwGetTime();

    int gen_radius = (200 + Chunk::chunk_size) / Chunk::chunk_size;
    _world_gen.generate_around(0.0, 0.0, gen_radius);
    poll_mouse(window, _mouse_x, _mouse_y);

    int p_state = glfwGetKey(window, GLFW_KEY_P);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        float current_time = (float)glfwGetTime();
        float delta = current_time - prev_time;
        prev_time = current_time;

        update_input(window, delta);

        if (glfwGetKey(window, GLFW_KEY_P) != p_state)
        {
            p_state = glfwGetKey(window, GLFW_KEY_P);
            if (p_state == GLFW_PRESS)
            {
                UpdateClipFrustum = !UpdateClipFrustum;
            }
        }

        _world_gen.generate_around(_camera.position.x, _camera.position.z, gen_radius);
        float height = _world_gen.get_height(_camera.position.x, _camera.position.z) + 1.8f;

        if (_camera.position.y < height || glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        {
            _camera.position.y = height;
        }

        _renderer.set_view_matrix(_camera.get_view_matrix());

        glm::mat4x4 model;
        _renderer.set_model_matrix(model);

        if (!_renderer.draw_frame())
        {
            glfwSetWindowShouldClose(window, true);
        }
    }

    _renderer.shutdown();
}

void set_window_size(GLFWwindow* window, int width, int height)
{
    if (width && height)
    {
        glm::mat4x4 proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.25f, 200.0f);
        proj[1] *= -1.0f;
        _renderer.set_proj_matrix(proj);
        poll_mouse(window, _mouse_x, _mouse_y);
    }

    if (!_renderer.set_window_size((uint32_t)width, (uint32_t)height))
    {
        glfwSetWindowShouldClose(window, true);
    }
}
