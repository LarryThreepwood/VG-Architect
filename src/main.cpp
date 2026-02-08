#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <vector>

namespace
{
constexpr float kPi = 3.14159265358979323846f;

double g_scrollOffset = 0.0;

void OnScroll(GLFWwindow*, double, double yoffset)
{
    g_scrollOffset += yoffset;
}

void OnFramebufferSize(GLFWwindow*, int width, int height)
{
    glViewport(0, 0, width, height);
}

GLuint CompileShader(GLenum type, const char* source)
{
    const GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    return shader;
}

GLuint CreateProgram(const char* vertexSource, const char* fragmentSource)
{
    const GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
    const GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

float ClampFloat(float value, float minValue, float maxValue)
{
    return value < minValue ? minValue : (value > maxValue ? maxValue : value);
}
}

int main()
{
    if (glfwInit() != GLFW_TRUE)
    {
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "VG Architect", nullptr, nullptr);
    if (window == nullptr)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glfwSetScrollCallback(window, OnScroll);
    glfwSetFramebufferSizeCallback(window, OnFramebufferSize);

    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0)
    {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    int framebufferWidth = 0;
    int framebufferHeight = 0;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    std::vector<float> lineVertices;
    lineVertices.reserve(1024);

    constexpr int gridExtent = 20;
    constexpr float gridColor = 0.35f;

    for (int i = -gridExtent; i <= gridExtent; ++i)
    {
        const float value = static_cast<float>(i);

        lineVertices.insert(lineVertices.end(), {value, 0.0f, static_cast<float>(-gridExtent), gridColor, gridColor, gridColor});
        lineVertices.insert(lineVertices.end(), {value, 0.0f, static_cast<float>(gridExtent), gridColor, gridColor, gridColor});

        lineVertices.insert(lineVertices.end(), {static_cast<float>(-gridExtent), 0.0f, value, gridColor, gridColor, gridColor});
        lineVertices.insert(lineVertices.end(), {static_cast<float>(gridExtent), 0.0f, value, gridColor, gridColor, gridColor});
    }

    lineVertices.insert(lineVertices.end(), {0.0f, 0.0f, 0.0f, 1.0f, 0.2f, 0.2f, 5.0f, 0.0f, 0.0f, 1.0f, 0.2f, 0.2f});
    lineVertices.insert(lineVertices.end(), {0.0f, 0.0f, 0.0f, 0.2f, 1.0f, 0.2f, 0.0f, 5.0f, 0.0f, 0.2f, 1.0f, 0.2f});
    lineVertices.insert(lineVertices.end(), {0.0f, 0.0f, 0.0f, 0.2f, 0.4f, 1.0f, 0.0f, 0.0f, 5.0f, 0.2f, 0.4f, 1.0f});

    GLuint vao = 0;
    GLuint vbo = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(lineVertices.size() * sizeof(float)), lineVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), reinterpret_cast<const void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    const char* vertexShaderSource =
        "#version 330 core\n"
        "layout (location = 0) in vec3 aPosition;\n"
        "layout (location = 1) in vec3 aColor;\n"
        "uniform mat4 uViewProjection;\n"
        "out vec3 vColor;\n"
        "void main()\n"
        "{\n"
        "    gl_Position = uViewProjection * vec4(aPosition, 1.0);\n"
        "    vColor = aColor;\n"
        "}\n";

    const char* fragmentShaderSource =
        "#version 330 core\n"
        "in vec3 vColor;\n"
        "out vec4 FragColor;\n"
        "void main()\n"
        "{\n"
        "    FragColor = vec4(vColor, 1.0);\n"
        "}\n";

    const GLuint shaderProgram = CreateProgram(vertexShaderSource, fragmentShaderSource);
    const GLint viewProjectionLocation = glGetUniformLocation(shaderProgram, "uViewProjection");

    glEnable(GL_DEPTH_TEST);

    float currentYaw = -kPi * 0.25f;
    float desiredYaw = currentYaw;

    float currentPitch = -0.6f;
    float desiredPitch = currentPitch;

    float currentDistance = 15.0f;
    float desiredDistance = currentDistance;

    glm::vec3 currentTarget(0.0f, 0.0f, 0.0f);
    glm::vec3 desiredTarget = currentTarget;

    double previousMouseX = 0.0;
    double previousMouseY = 0.0;
    glfwGetCursorPos(window, &previousMouseX, &previousMouseY);

    double previousTime = glfwGetTime();

    while (!glfwWindowShouldClose(window))
    {
        const double currentTime = glfwGetTime();
        const float deltaTime = static_cast<float>(currentTime - previousTime);
        previousTime = currentTime;

        double mouseX = 0.0;
        double mouseY = 0.0;
        glfwGetCursorPos(window, &mouseX, &mouseY);

        const double deltaX = mouseX - previousMouseX;
        const double deltaY = mouseY - previousMouseY;
        previousMouseX = mouseX;
        previousMouseY = mouseY;

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
        {
            desiredYaw -= static_cast<float>(deltaX) * 0.005f;
            desiredPitch -= static_cast<float>(deltaY) * 0.005f;
            desiredPitch = ClampFloat(desiredPitch, -1.5f, 1.5f);
        }

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS)
        {
            const glm::vec3 orbitDirection(
                std::cos(currentPitch) * std::cos(currentYaw),
                std::sin(currentPitch),
                std::cos(currentPitch) * std::sin(currentYaw));
            const glm::vec3 forward = glm::normalize(orbitDirection);
            const glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
            const glm::vec3 up = glm::normalize(glm::cross(right, forward));
            const float panScale = currentDistance * 0.0015f;

            desiredTarget -= right * static_cast<float>(deltaX) * panScale;
            desiredTarget += up * static_cast<float>(deltaY) * panScale;
        }

        if (g_scrollOffset != 0.0)
        {
            desiredDistance *= std::pow(0.9f, static_cast<float>(g_scrollOffset));
            desiredDistance = ClampFloat(desiredDistance, 2.0f, 100.0f);
            g_scrollOffset = 0.0;
        }

        const float blend = 1.0f - std::exp(-12.0f * deltaTime);
        currentYaw += (desiredYaw - currentYaw) * blend;
        currentPitch += (desiredPitch - currentPitch) * blend;
        currentDistance += (desiredDistance - currentDistance) * blend;
        currentTarget += (desiredTarget - currentTarget) * blend;

        const glm::vec3 orbitDirection(
            std::cos(currentPitch) * std::cos(currentYaw),
            std::sin(currentPitch),
            std::cos(currentPitch) * std::sin(currentYaw));
        const glm::vec3 cameraPosition = currentTarget - orbitDirection * currentDistance;

        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        const float aspectRatio = framebufferHeight > 0 ? static_cast<float>(framebufferWidth) / static_cast<float>(framebufferHeight) : 1.0f;

        const glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 500.0f);
        const glm::mat4 view = glm::lookAt(cameraPosition, currentTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 viewProjection = projection * view;

        glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(viewProjectionLocation, 1, GL_FALSE, &viewProjection[0][0]);

        glBindVertexArray(vao);
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(lineVertices.size() / 6));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
