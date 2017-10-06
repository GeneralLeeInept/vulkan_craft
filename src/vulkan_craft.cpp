#include <Windows.h>
#include <stdlib.h>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

#include "camera.h"
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

    _camera.position.z = 5.0f;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        float current_time = (float)glfwGetTime();
        float delta = current_time - prev_time;
        prev_time = current_time;

        update_input(window, delta);

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
        glm::mat4x4 proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 1.0f, 100.0f);
        proj[1] *= -1.0f;
        _renderer.set_proj_matrix(proj);
        poll_mouse(window, _mouse_x, _mouse_y);
    }

    if (!_renderer.set_window_size((uint32_t)width, (uint32_t)height))
    {
        glfwSetWindowShouldClose(window, true);
    }
}
