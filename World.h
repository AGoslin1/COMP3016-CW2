#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "shader_m.h"
#include "Physics.h"
#include <irrKlang.h>

class Model;

using namespace irrklang;

struct CockroachInstance
{
    glm::vec3 pos;
    bool      dancing = false;
    float     time = 0.0f;
};

struct GrassInstance
{
    glm::vec3 pos;
    float     rot;
    float     scale;
    int       type;
};

struct SkullInstance
{
    glm::vec3 pos;
    glm::vec3 vel;
    bool      active = false;
};

struct World {
    PhysicsBody                player;
    std::vector<PhysicsBody>   boulderWall;
    std::vector<Sphere>        balls;
    std::vector<GrassInstance> grass;

    int screenWidth = 800;
    int screenHeight = 600;

    unsigned int planeVAO = 0, planeVBO = 0;
    unsigned int sphereVAO = 0, sphereVBO = 0;
    int          sphereVertCount = 0;
    unsigned int groundTex = 0;

    //ball pit parameters
    unsigned int pitVAO = 0;
    unsigned int pitVBO = 0;
    int          pitVertCount = 0;
    std::vector<PhysicsBody>  ballPitWalls;

    glm::vec3 pedestalPos = glm::vec3(15.0f, 0.0f, -5.0f);

    bool  qteActive = false;  
    bool  qteVisible = false;  
    int   qteTargetHits = 10;     
    int   qteCurrentHits = 0;      
    float qteOuterRadius = 0.0f;   
    float qteInnerRadius = 0.25f;  
    float qteOuterMaxRadius = 0.7f;   
    float qteOuterShrinkTime = 1.5f;   
    float qteTimer = 0.0f;   
    bool  qteThisRoundHit = false;  
    bool  qteCompleted = false;  

    unsigned int pedestalVAO = 0;
    unsigned int pedestalVBO = 0;

    //star system
    int  starCount = 0;

    bool starRoachesAwarded = false;
    bool starQTEAwarded = false;
    bool starSkullAwarded = false;
    bool starGoldenBallAwarded = false;

    bool starsCelebrationDone = false;


    unsigned int meTex = 0;
    unsigned int meVAO = 0;
    unsigned int meVBO = 0;


    Model* boulder = nullptr;
    Model* grass1 = nullptr;
    Model* grass2 = nullptr;
    Model* grass3 = nullptr;
    Model* cockroach = nullptr;
    Model* skull = nullptr; 


    int goldenBallIndex = -1;  

    float     boulderScale = 0.7f;
    glm::vec3 boulderHalf = glm::vec3(5.2f);

    //dance state
    glm::vec3 cockroachPos = glm::vec3(5.0f, 0.1f, 10.0f);
    bool      cockroachDance = false;
    float     cockroachTime = 0.0f;

    //UI quad
    unsigned int uiQuadVAO = 0;
    unsigned int uiQuadVBO = 0;

    std::vector<CockroachInstance> cockroaches;

    //audio (irrKlang)
    ISoundEngine* soundEngine = nullptr;
    irrklang::ISound* cucarachaSound = nullptr;
    float cucarachaTimer = 0.0f;


    //footstep SFX
    float footstepTimer = 0.0f;
    float footstepInterval = 0.45f;
    glm::vec3 lastPlayerPos = glm::vec3(0.0f);
    int footstepIndex = 0;


    //skull attack mode
    glm::vec3 skullSquarePos = glm::vec3(-5.0f, 0.0f, -5.0f); 
    bool      skullModeActive = false;
    bool      skullModeFailed = false;
    bool      skullModeSurvived = false;   
    float     skullModeTime = 0.0f;    

    float     skullSpawnTimer = 0.0f;    
    float     skullSpawnInterval = 2.0f;  
    float     skullMinInterval = 0.3f;

    std::vector<SkullInstance> skulls;
};

void GenerateSphereMesh(World& world, int lat = 20, int lon = 20);
void GeneratePlane(World& world);
unsigned int LoadTexture(const char* path);

void InitWorld(World& world);
void UpdateWorld(World& world, float dt);
void RenderWorld(World& world,
    Shader& shader,
    Shader& depthShader,
    const glm::mat4& lightSpace,
    const glm::mat4& view,
    const glm::mat4& proj,
    unsigned int depthTex,
    unsigned int depthFBO,
    unsigned int SHW,
    unsigned int SHH);

//QTE input handler (used from main.cpp)
void HandleQTEInput(World& world);


//skull mode input handler (used from main.cpp)
void HandleSkullModeInput(World& world);