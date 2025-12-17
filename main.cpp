#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <ctime>

#include <glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "shader_m.h"
#include "World.h"
#include "Physics.h"

#include <irrKlang.h>
using namespace irrklang;

//camera
glm::vec3 cameraPos(0, 2, 8);
glm::vec3 cameraFront(0, 0, -1);
glm::vec3 cameraUp(0, 1, 0);

float yaw = -90.f, pitch = 0.f;
float lastX = 400, lastY = 300;
bool firstMouse = true;

void mouse_callback(GLFWwindow*, double xpos, double ypos)
{
    if (firstMouse) { lastX = (float)xpos; lastY = (float)ypos; firstMouse = false; }

    float xoff = (float)xpos - lastX;
    float yoff = lastY - (float)ypos;
    lastX = (float)xpos; lastY = (float)ypos;

    float sens = 0.15f;
    xoff *= sens;
    yoff *= sens;

    yaw += xoff;
    pitch += yoff;

    if (pitch > 89) pitch = 89;
    if (pitch < -89) pitch = -89;

    glm::vec3 dir;
    dir.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    dir.y = sin(glm::radians(pitch));
    dir.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    cameraFront = glm::normalize(dir);
}

void framebuffer_size_callback(GLFWwindow* window, int w, int h)
{
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);

    World* world = static_cast<World*>(glfwGetWindowUserPointer(window));
    if (world)
    {
        world->screenWidth = w;
        world->screenHeight = h;
    }
}

int main()
{
    World world{};

    world.soundEngine = createIrrKlangDevice();
    if (!world.soundEngine)
    {
        std::cerr << "Failed to create irrKlang device\n";
        return -1;
    }

    std::srand((unsigned int)std::time(0));

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int initialWidth = 800;
    const int initialHeight = 600;

    GLFWwindow* window = glfwCreateWindow(initialWidth, initialHeight, "WORLD", nullptr, nullptr);
    glfwMakeContextCurrent(window);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // let callbacks access World for size updates
    glfwSetWindowUserPointer(window, &world);
    world.screenWidth = initialWidth;
    world.screenHeight = initialHeight;

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_DEPTH_TEST);

    Shader shader("model_loading.vert", "model_loading.frag");
    Shader depthShader("shadow_depth.vert", "shadow_depth.frag");

    InitWorld(world);

    //shadow init
    unsigned int SHW = 2048, SHH = 2048;
    unsigned int depthFBO;
    glGenFramebuffers(1, &depthFBO);

    unsigned int depthTex;
    glGenTextures(1, &depthTex);
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHW, SHH, 0,
        GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    float border[] = { 1,1,1,1 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_TEXTURE_2D, depthTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glm::vec3 lightDir = glm::normalize(glm::vec3(0, -1, 0));

    glm::mat4 lightProj =
        glm::ortho(-60.f, 60.f, -60.f, 60.f, 0.1f, 100.f);

    glm::mat4 lightView =
        glm::lookAt(glm::vec3(0, 40, 0),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 0, 1));

    glm::mat4 lightSpace = lightProj * lightView;

    float last = 0;
    bool  prevEPressed = false;

    while (!glfwWindowShouldClose(window))
    {
        float now = (float)glfwGetTime();
        float dt = now - last;
        last = now;

        //cockroach timers
        for (auto& r : world.cockroaches)
            r.time += dt;

        //INPUT
        float speed = 6.0f;

        glm::vec3 fwd = glm::normalize(glm::vec3(cameraFront.x, 0, cameraFront.z));
        glm::vec3 right = glm::normalize(glm::cross(fwd, cameraUp));

        if (glfwGetKey(window, 'W') == GLFW_PRESS) world.player.pos += fwd * speed * dt;
        if (glfwGetKey(window, 'S') == GLFW_PRESS) world.player.pos -= fwd * speed * dt;
        if (glfwGetKey(window, 'A') == GLFW_PRESS) world.player.pos -= right * speed * dt;
        if (glfwGetKey(window, 'D') == GLFW_PRESS) world.player.pos += right * speed * dt;

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && world.player.grounded)
        {
            world.player.vel.y = 6;
            world.player.grounded = false;
        }

        //E interaction
        int  eState = glfwGetKey(window, GLFW_KEY_E);
        bool ePressedNow = (eState == GLFW_PRESS);

        if (ePressedNow && !prevEPressed)
        {
            //cockroach interaction
            float bestDist2 = 3.0f * 3.0f;
            int   bestIdx = -1;

            for (int i = 0; i < (int)world.cockroaches.size(); ++i)
            {
                const auto& r = world.cockroaches[i];
                float d2 = glm::length(world.player.pos - r.pos);
                if (d2 < bestDist2)
                {
                    bestDist2 = d2;
                    bestIdx = i;
                }
            }

            if (bestIdx >= 0)
            {
                auto& r = world.cockroaches[bestIdx];
                r.dancing = !r.dancing;

                //check if all roaches are dancing
                bool allDancing = !world.cockroaches.empty();
                for (const auto& c : world.cockroaches)
                {
                    if (!c.dancing)
                    {
                        allDancing = false;
                        break;
                    }
                }

                if (allDancing)
                {
                    if (!world.starRoachesAwarded)
                    {
                        world.starRoachesAwarded = true;
                        world.starCount++;
                        std::cout << "All roaches dancing! +1 STAR (total: " << world.starCount << ")\n";
                    }
                    else
                    {
                        std::cout << "All roaches dancing (star already awarded)\n";
                    }
                }
            }

            //golden ball interaction
            if (world.goldenBallIndex >= 0 &&
                world.goldenBallIndex < (int)world.balls.size())
            {
                Sphere& golden = world.balls[world.goldenBallIndex];
                float dist = glm::length(world.player.pos - golden.pos);

                if (dist < 2.0f)
                {
                    golden.radius = 0.0f; //disappear
                    world.goldenBallIndex = -1;

                    //award star
                    if (!world.starGoldenBallAwarded)
                    {
                        world.starGoldenBallAwarded = true;
                        world.starCount++;
                        std::cout << "Found the golden ball! +1 STAR (total: " << world.starCount << ")\n";
                    }
                    else
                    {
                        std::cout << "Found the golden ball (star already awarded)\n";
                    }
                }
            }

            //QTE interaction
            HandleQTEInput(world);
            HandleSkullModeInput(world);
        }
        prevEPressed = ePressedNow;

        //update and camera
        UpdateWorld(world, dt);
        cameraPos = world.player.pos + glm::vec3(0, 1, 0);

        //use current window size for aspect
        float aspect = (world.screenHeight != 0)
            ? static_cast<float>(world.screenWidth) / static_cast<float>(world.screenHeight)
            : 800.0f / 600.0f;

        glm::mat4 proj =
            glm::perspective(glm::radians(70.f), aspect, 0.1f, 300.f);
        glm::mat4 view =
            glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        shader.use();
        shader.setVec3("lightDir", lightDir);

        RenderWorld(world, shader, depthShader,
            lightSpace, view, proj,
            depthTex, depthFBO, SHW, SHH);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    if (world.cucarachaSound)
        world.cucarachaSound->drop(); 
    if (world.soundEngine)
        world.soundEngine->drop();

    return 0;
}