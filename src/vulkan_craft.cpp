#include <Windows.h>
#include <stdlib.h>

#include <GLFW/glfw3.h>

#include "renderer.h"

void run_game(GLFWwindow* window);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_VISIBLE, false);
    GLFWwindow* window = glfwCreateWindow(1024, 576, "VulkanCraft", nullptr, nullptr);

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

        if (!_renderer.draw_frame())
        {
            glfwSetWindowShouldClose(window, true);
        }
    }

    _renderer.shutdown();
}
