#include <GpO.h>
#include "Modelo3D.h"

#include <iostream>

int ANCHO = 800, ALTO = 600;
GLFWwindow* window;

float angulo_h = 0.0f;
float angulo_v = 0.3f;
float radio    = 1.0f;

bool rightMouseDown = false;
double lastX, lastY;

Modelo3D* modelo = nullptr;

// ===================== RATÓN =====================

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS) {
            rightMouseDown = true;
            glfwGetCursorPos(window, &lastX, &lastY);
        }
        else if (action == GLFW_RELEASE) {
            rightMouseDown = false;
        }
    }
}

void mouse_orbital(GLFWwindow* window, double xpos, double ypos)
{
    if (!rightMouseDown) return;

    float dx = xpos - lastX;
    float dy = ypos - lastY;

    lastX = xpos;
    lastY = ypos;

    float sens = 0.005f;

    angulo_h -= dx * sens;
    angulo_v += dy * sens;

    if (angulo_v < -1.2f) angulo_v = -1.2f;
    if (angulo_v >  1.2f) angulo_v =  1.2f;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    radio -= yoffset * 0.06f;

    if (radio > 2.0f) radio = 2.0f;
    if (radio < 0.005f) radio = 0.005f;
}

// ===================== INIT =====================

void init_scene()
{
    glEnable(GL_DEPTH_TEST);

    glfwSetCursorPosCallback(window, mouse_orbital);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetScrollCallback(window, scroll_callback);
}

// ===================== RENDER =====================

void render_scene()
{
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mat4 P = perspective(
        glm::radians(45.0f),
        float(ANCHO) / float(ALTO),
        0.01f,
        100.0f
    );

    vec3 cam(
        radio * cos(angulo_h) * cos(angulo_v),
        radio * sin(angulo_h) * cos(angulo_v),
        radio * sin(angulo_v)
    );

    mat4 V = lookAt(
        cam,
        vec3(0.0f, 0.0f, 0.0f),
        vec3(0.0f, 0.0f, 1.0f)
    );

    mat4 M = mat4(1.0f);
    M = scale(M, vec3(0.01f));
    M = rotate(M, glm::radians(-90.0f), vec3(1.0f, 0.0f, 0.0f));
    modelo->draw(P, V, M);
}

// ===================== CALLBACKS WINDOW =====================

void ResizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    ANCHO = width;
    ALTO = height;
}

void KeyCallback(GLFWwindow* window, int key, int code, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, true);
}

void asigna_funciones_callback(GLFWwindow* window)
{
    glfwSetWindowSizeCallback(window, ResizeCallback);
    glfwSetKeyCallback(window, KeyCallback);
}

// ===================== MAIN =====================

int main()
{
    init_GLFW();

    window = Init_Window("Star Wars Model Viewer");

    load_Opengl();
    modelo = new Modelo3D("../data/Mask.obj");

    asigna_funciones_callback(window);

    init_scene();

    while (!glfwWindowShouldClose(window))
    {
        render_scene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}