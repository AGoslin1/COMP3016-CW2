#include <iostream>
#include <glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <shader_m.h>
#include <model.h>

void framebuffer_size_callback(GLFWwindow* win, int w, int h) {
    glViewport(0, 0, w, h);
}

int main() {

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Rock Model", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to init GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // ----------------------------------
    // LOAD SHADER
    // ----------------------------------
    Shader shader("model_loading.vert", "model_loading.frag");

    // ----------------------------------
    // LOAD ROCK MODEL (YOUR EXACT PATH)
    // ----------------------------------
    Model rock("media/rock/Rock07-Base.obj");

    // ----------------------------------
    // CAMERA MATRICES
    // ----------------------------------

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        800.0f / 600.0f,
        0.1f,
        100.0f
    );

    glm::mat4 view = glm::translate(glm::mat4(1.0f),
        glm::vec3(0.0f, -1.0f, -4.0f));

    glm::mat4 modelMat = glm::mat4(1.0f);

    // ----------------------------------
    // MAIN LOOP
    // ----------------------------------
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        shader.use();

        // --- STEP 2: LIGHT + CAMERA UNIFORMS ---
        shader.setVec3("lightPos", glm::vec3(2.0f, 2.0f, 2.0f));
        shader.setVec3("viewPos", glm::vec3(0.0f, 0.0f, 3.0f));
        // ---------------------------------------

            // --- STEP 3: ROTATE MODEL ---
        modelMat = glm::rotate(modelMat, glm::radians(0.2f), glm::vec3(0.0f, 1.0f, 0.0f));
        // -----------------------------

        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setMat4("model", modelMat);

        rock.Draw(shader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
