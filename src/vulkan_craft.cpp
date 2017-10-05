#include <Windows.h>
#include <stdlib.h>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>

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

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        _renderer.set_view_matrix(glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f - 3.0f * sinf(glm::radians(90.0f * (float)glfwGetTime()))), glm::vec3(),
                                              glm::vec3(0.0f, 1.0f, 0.0f)));
        _renderer.set_model_matrix(glm::rotate(glm::mat4x4(), glm::radians(90.0f * (float)glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f)));

        if (!_renderer.draw_frame())
        {
            glfwSetWindowShouldClose(window, true);
        }
    }

    _renderer.shutdown();
}

void set_window_size(GLFWwindow* window, int width, int height)
{
    glm::mat4x4 proj = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 10.0f);
    proj[1] *= -1.0f;
    _renderer.set_proj_matrix(proj);

    if (!_renderer.set_window_size((uint32_t)width, (uint32_t)height))
    {
        glfwSetWindowShouldClose(window, true);
    }
}
