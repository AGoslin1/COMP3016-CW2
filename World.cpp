#define _USE_MATH_DEFINES
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include "model.h"
#include "World.h"
#include "stb_image.h"

//local helpers
static float frand(float a, float b)
{
    return a + (float(std::rand()) / RAND_MAX) * (b - a);
}

void GenerateSphereMesh(World& world, int lat, int lon)
{
    struct V { float x, y, z, nx, ny, nz; };
    std::vector<glm::vec3> tempVerts;
    std::vector<unsigned int> idx;
    std::vector<V> finalVerts;

    for (int y = 0; y <= lat; y++)
    {
        for (int x = 0; x <= lon; x++)
        {
            float xs = float(x) / lon;
            float ys = float(y) / lat;

            float px = std::cos(xs * 2 * M_PI) * std::sin(ys * M_PI);
            float py = std::cos(ys * M_PI);
            float pz = std::sin(xs * 2 * M_PI) * std::sin(ys * M_PI);

            tempVerts.push_back({ px,py,pz });
        }
    }

    for (int y = 0; y < lat; y++)
    {
        for (int x = 0; x < lon; x++)
        {
            int i0 = y * (lon + 1) + x;
            int i1 = i0 + lon + 1;

            idx.push_back(i0);
            idx.push_back(i1);
            idx.push_back(i0 + 1);

            idx.push_back(i0 + 1);
            idx.push_back(i1);
            idx.push_back(i1 + 1);
        }
    }

    for (unsigned int i : idx)
    {
        glm::vec3 p = tempVerts[i];
        glm::vec3 n = glm::normalize(p);
        finalVerts.push_back({ p.x,p.y,p.z, n.x,n.y,n.z });
    }

    world.sphereVertCount = (int)finalVerts.size();

    glGenVertexArrays(1, &world.sphereVAO);
    glGenBuffers(1, &world.sphereVBO);

    glBindVertexArray(world.sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, world.sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, finalVerts.size() * sizeof(V), finalVerts.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void GeneratePlane(World& world)
{
    float verts[] = {
        -1,0,-1,   0,1,0,    0,  0,
         1,0,-1,   0,1,0,    100,0,
         1,0, 1,   0,1,0,    100,100,

        -1,0,-1,   0,1,0,    0,  0,
         1,0, 1,   0,1,0,    100,100,
        -1,0, 1,   0,1,0,    0, 100
    };

    glGenVertexArrays(1, &world.planeVAO);
    glGenBuffers(1, &world.planeVBO);

    glBindVertexArray(world.planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, world.planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

static float DistanceXZ(const glm::vec3& a, const glm::vec3& b)
{
    glm::vec2 da(a.x - b.x, a.z - b.z);
    return glm::length(da);
}
static void ResetSkullMode(World& world);


void GeneratePedestalMesh(World& world)
{
 
    struct V { float x, y, z, nx, ny, nz; };
    std::vector<V> verts;
    verts.reserve(6);

    const float x0 = -1.0f;
    const float x1 = 1.0f;
    const float z0 = -1.0f;
    const float z1 = 1.0f;
    const float y = 0.0f;
    const float nx = 0.0f, ny = 1.0f, nz = 0.0f;

    //two triangles forming a square
    verts.push_back({ x0, y, z0, nx, ny, nz });
    verts.push_back({ x1, y, z0, nx, ny, nz });
    verts.push_back({ x1, y, z1, nx, ny, nz });

    verts.push_back({ x0, y, z0, nx, ny, nz });
    verts.push_back({ x1, y, z1, nx, ny, nz });
    verts.push_back({ x0, y, z1, nx, ny, nz });

    glGenVertexArrays(1, &world.pedestalVAO);
    glGenBuffers(1, &world.pedestalVBO);

    glBindVertexArray(world.pedestalVAO);
    glBindBuffer(GL_ARRAY_BUFFER, world.pedestalVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(V), verts.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

//QTE input handler
void HandleQTEInput(World& world)
{
    const float startRange = 2.0f; //how close player must be

    if (!world.qteActive)
    {
        float dist = DistanceXZ(world.player.pos, world.pedestalPos);
        if (dist <= startRange)
        {
            //start the minigame
            world.qteActive = true;
            world.qteVisible = true;
            world.qteCompleted = false;
            world.qteCurrentHits = 0;
            world.qteOuterShrinkTime = 1.5f; //reset to slow speed
            world.qteTimer = 0.0f;
            world.qteOuterRadius = world.qteOuterMaxRadius;
            world.qteThisRoundHit = false;

            std::cout << "QTE started\n";
        }
        return;
    }

    //if active E is an attempt
    if (!world.qteVisible)
        return;

    //success outer inside inner
    if (!world.qteThisRoundHit &&
        world.qteOuterRadius <= world.qteInnerRadius)
    {
        world.qteThisRoundHit = true;
        world.qteCurrentHits++;
        std::cout << "QTE hit " << world.qteCurrentHits << "/" << world.qteTargetHits << "\n";

        if (world.qteCurrentHits >= world.qteTargetHits)
        {
            world.qteCompleted = true;

            //award star once for this minigame
            if (!world.starQTEAwarded)
            {
                world.starQTEAwarded = true;
                world.starCount++;
                std::cout << "QTE COMPLETED (8 hits)! +1 STAR (total: " << world.starCount << ")\n";
            }
            else
            {
                std::cout << "QTE COMPLETED (8 hits, star already awarded)\n";
            }
        }

        //next round speed up
        world.qteOuterShrinkTime *= 0.85f;
        world.qteTimer = 0.0f;
        world.qteOuterRadius = world.qteOuterMaxRadius;
        world.qteThisRoundHit = false;
    }
    else
    {
        //reset whole minigame
        std::cout << "QTE FAILED, reset\n";
        world.qteActive = false;
        world.qteVisible = false;
        world.qteCurrentHits = 0;
        world.qteTimer = 0.0f;
        world.qteOuterRadius = world.qteOuterMaxRadius;
        world.qteThisRoundHit = false;
        world.qteCompleted = false;
    }
}

void HandleSkullModeInput(World& world)
{
    const float startRange = 2.0f;

    //ignore if already running
    if (world.skullModeActive)
        return;

    float dist = DistanceXZ(world.player.pos, world.skullSquarePos);
    if (dist <= startRange)
    {
        std::cout << "Skull mode START (E pressed)\n";

        //full reset
        ResetSkullMode(world);
        world.skullModeActive = true;
    }
}

void GenerateCylinderMesh(World& world, int)
{
    //square pit walls
    struct V { float x, y, z, nx, ny, nz; };
    std::vector<V> verts;
    verts.reserve(6 * 4);

    const float y0 = 0.0f;
    const float y1 = 1.0f;
    const float x0 = -1.0f;
    const float x1 = 1.0f;
    const float z0 = -1.0f;
    const float z1 = 1.0f;

    auto pushTri = [&](float xA, float yA, float zA,
        float xB, float yB, float zB,
        float xC, float yC, float zC,
        float nx, float ny, float nz)
        {
            verts.push_back({ xA, yA, zA, nx, ny, nz });
            verts.push_back({ xB, yB, zB, nx, ny, nz });
            verts.push_back({ xC, yC, zC, nx, ny, nz });
        };

    //+Z
    {
        float nx = 0.0f, ny = 0.0f, nz = 1.0f;
        pushTri(x0, y0, z1, x0, y1, z1, x1, y1, z1, nx, ny, nz);
        pushTri(x0, y0, z1, x1, y1, z1, x1, y0, z1, nx, ny, nz);
    }
    //-Z
    {
        float nx = 0.0f, ny = 0.0f, nz = -1.0f;
        pushTri(x0, y0, z0, x1, y1, z0, x0, y1, z0, nx, ny, nz);
        pushTri(x0, y0, z0, x1, y0, z0, x1, y1, z0, nx, ny, nz);
    }
    //+X
    {
        float nx = 1.0f, ny = 0.0f, nz = 0.0f;
        pushTri(x1, y0, z0, x1, y1, z0, x1, y1, z1, nx, ny, nz);
        pushTri(x1, y0, z0, x1, y1, z1, x1, y0, z1, nx, ny, nz);
    }
    //-X
    {
        float nx = -1.0f, ny = 0.0f, nz = 0.0f;
        pushTri(x0, y0, z0, x0, y1, z1, x0, y1, z0, nx, ny, nz);
        pushTri(x0, y0, z0, x0, y0, z1, x0, y1, z1, nx, ny, nz);
    }

    world.pitVertCount = (int)verts.size();

    glGenVertexArrays(1, &world.pitVAO);
    glGenBuffers(1, &world.pitVBO);

    glBindVertexArray(world.pitVAO);
    glBindBuffer(GL_ARRAY_BUFFER, world.pitVBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(V), verts.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(V), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(V), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

unsigned int LoadTexture(const char* path)
{
    unsigned int tex;
    glGenTextures(1, &tex);

    int w, h, ch;
    unsigned char* data = stbi_load(path, &w, &h, &ch, 0);

    if (!data)
    {
        std::cout << "FAILED TO LOAD TEXTURE: " << path << "\n";
        return 0;
    }

    GLenum format = (ch == 4 ? GL_RGBA : GL_RGB);

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    return tex;
}

static void SpawnSkullTowardsPlayer(World& world)
{
    if (!world.skullModeActive) return;

    SkullInstance inst;

    //spawn on a circle around the player
    float radius = 25.0f;
    float ang = frand(0.0f, 2.0f * (float)M_PI);
    glm::vec3 spawn(
        world.player.pos.x + std::cos(ang) * radius,
        world.player.pos.y + frand(0.5f, 3.0f), //random height
        world.player.pos.z + std::sin(ang) * radius
    );

    inst.pos = spawn;

    glm::vec3 toPlayer = world.player.pos - spawn;
    float len = glm::length(toPlayer);
    if (len < 0.0001f) len = 0.0001f;
    glm::vec3 dir = toPlayer / len;

    float speed = frand(6.0f, 10.0f); //fly speed
    inst.vel = dir * speed;
    inst.active = true;

    world.skulls.push_back(inst);
}

static void ResetSkullMode(World& world)
{
    world.skullModeActive = false;
    world.skullModeTime = 0.0f;
    world.skullSpawnTimer = 0.0f;
    world.skullSpawnInterval = 2.0f;
    world.skulls.clear();
}




void InitWorld(World& world)
{
    //construct models
    world.boulder = new Model("media/boulders/RockSpires_Obj/RockSpires_Obj/RockSpires_2.obj");
    world.grass1 = new Model("media/grass/Grass1.obj");
    world.grass2 = new Model("media/grass/Grass2.obj");
    world.grass3 = new Model("media/grass/Grass3.obj");
    world.cockroach = new Model("media/cockroach/cuban-cockroach/source/cuban_cockroach.obj");
    world.skull = new Model("media/skull/scull lp.obj");


    GenerateSphereMesh(world);
    GeneratePlane(world);
    GenerateCylinderMesh(world, 48);
    GeneratePedestalMesh(world);
    world.groundTex = LoadTexture("media/textures/ground.png");

    world.meTex = LoadTexture("media/me!/image.jpg");


    //UI for stars
    {
        // x, y, z,  u, v
        float uiVerts[] = {
            0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
            1.0f, 0.0f, 0.0f,  1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,  1.0f, 1.0f,

            0.0f, 0.0f, 0.0f,  0.0f, 0.0f,
            1.0f, 1.0f, 0.0f,  1.0f, 1.0f,
            0.0f, 1.0f, 0.0f,  0.0f, 1.0f
        };

        glGenVertexArrays(1, &world.uiQuadVAO);
        glGenBuffers(1, &world.uiQuadVBO);
        glBindVertexArray(world.uiQuadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, world.uiQuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(uiVerts), uiVerts, GL_STATIC_DRAW);

        //position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
    }

    world.player = { glm::vec3(0,2,0), glm::vec3(0), glm::vec3(0.5f,1.0f,0.5f) };
    world.lastPlayerPos = world.player.pos;

    auto addBoulder = [&](const glm::vec3& p)
        {
            world.boulderWall.push_back({ p, glm::vec3(0.0f), world.boulderHalf });
        };

    const int   boulderCount = 32;
    const float ringRadius = 30.0f;
    const float minYOffset = -world.boulderHalf.y * 0.5f;
    const float maxYOffset = world.boulderHalf.y * 0.2f;

    for (int i = 0; i < boulderCount; ++i)
    {
        float t = (float)i / (float)boulderCount;
        float ang = t * 2.0f * (float)M_PI;

        float x = world.player.pos.x + std::cos(ang) * ringRadius;
        float z = world.player.pos.z + std::sin(ang) * ringRadius;
        float y = world.boulderHalf.y + frand(minYOffset, maxYOffset);

        addBoulder(glm::vec3(x, y, z));
    }

    // BALL PIT
    world.balls.clear();
    world.balls.reserve(150);

    glm::vec3 pitCenter(0.0f, 0.5f, -10.0f);
    float     pitRadius = 4.0f;
    float     pitHeight = 1.4f;
    float     ballRadius = 0.3f;

    for (int i = 0; i < 150; ++i)
    {
        float ang = frand(0.0f, 2.0f * (float)M_PI);
        float r = std::sqrt(frand(0.0f, 1.0f)) * pitRadius;
        float x = pitCenter.x + std::cos(ang) * r;
        float z = pitCenter.z + std::sin(ang) * r;
        float y = pitCenter.y + frand(0.0f, pitHeight);

        Sphere s;
        s.pos = glm::vec3(x, y, z);
        s.vel = glm::vec3(0.0f);
        s.radius = ballRadius;
        s.mass = 1.0f;
        world.balls.push_back(s);
    }

    world.goldenBallIndex = (world.balls.empty() ? -1 : std::rand() % world.balls.size());

    // BALL PIT PHYSICS
    world.ballPitWalls.clear();
    world.ballPitWalls.reserve(4);

    auto addWall = [&](const glm::vec3& center, const glm::vec3& halfSize)
        {
            PhysicsBody wall;
            wall.pos = center;
            wall.vel = glm::vec3(0.0f);
            wall.size = halfSize;
            wall.grounded = false;
            world.ballPitWalls.push_back(wall);
        };

    const float radiusInset = 0.2f;
    const float wallRadius = pitRadius - radiusInset;
    const float wallThickness = 0.3f;
    const float wallHalfH = pitHeight * 0.5f;
    const float wallY = wallHalfH;

    //+Z wall
    addWall(glm::vec3(pitCenter.x, wallY, pitCenter.z + wallRadius),
        glm::vec3(wallRadius, wallHalfH, wallThickness));
    //-Z wall
    addWall(glm::vec3(pitCenter.x, wallY, pitCenter.z - wallRadius),
        glm::vec3(wallRadius, wallHalfH, wallThickness));
    //+X wall
    addWall(glm::vec3(pitCenter.x + wallRadius, wallY, pitCenter.z),
        glm::vec3(wallThickness, wallHalfH, wallRadius));
    //-X wall
    addWall(glm::vec3(pitCenter.x - wallRadius, wallY, pitCenter.z),
        glm::vec3(wallThickness, wallHalfH, wallRadius));

    //Grass
    world.grass.reserve(1500);
    for (int i = 0; i < 1500; ++i)
        world.grass.push_back({
            glm::vec3(frand(-25,25),0,frand(-25,25)),
            frand(0,360),
            frand(0.2f,0.4f),
            std::rand() % 3
            });

    //single cockroach values
    world.cockroachPos = glm::vec3(5.0f, 0.1f, 10.0f);
    world.cockroachDance = false;
    world.cockroachTime = 0.0f;

    //cockroach placements
    world.cockroaches.clear();

    {
        CockroachInstance inst;
        inst.pos = glm::vec3(10.0f, 0.1f, 10.0f);
        inst.dancing = false;
        inst.time = 0.0f;
        world.cockroaches.push_back(inst);
    }
    {
        CockroachInstance inst;
        inst.pos = glm::vec3(-6.0f, 0.1f, 24.0f);
        inst.dancing = false;
        inst.time = 0.0f;
        world.cockroaches.push_back(inst);
    }
    {
        CockroachInstance inst;
        inst.pos = glm::vec3(20.0f, 0.1f, -8.0f);
        inst.dancing = false;
        inst.time = 0.0f;
        world.cockroaches.push_back(inst);
    }
    {
        CockroachInstance inst;
        inst.pos = glm::vec3(-16.0f, 0.1f, -12.0f);
        inst.dancing = false;
        inst.time = 0.0f;
        world.cockroaches.push_back(inst);
    }
    {
        CockroachInstance inst;
        inst.pos = glm::vec3(0.0f, 0.1f, -24.0f);
        inst.dancing = false;
        inst.time = 0.0f;
        world.cockroaches.push_back(inst);
    }

    //first one
    if (!world.cockroaches.empty())
        world.cockroachPos = world.cockroaches[0].pos;

    //QTE pedestal setup
    world.pedestalPos = glm::vec3(10.0f, 0.0f, -10.0f);
    world.qteActive = false;
    world.qteVisible = false;
    world.qteCompleted = false;
    world.qteTargetHits = 8;
    world.qteCurrentHits = 0;
    world.qteInnerRadius = 0.25f;
    world.qteOuterMaxRadius = 0.7f;
    world.qteOuterRadius = 0.0f;
    world.qteOuterShrinkTime = 1.5f;
    world.qteTimer = 0.0f;
    world.qteThisRoundHit = false;


    //skull mode setup
    world.skullSquarePos = glm::vec3(-10.0f, 0.0f, -10.0f);
    world.skullModeActive = false;
    world.skullModeFailed = false;
    world.skullModeSurvived = false;
    ResetSkullMode(world);
}

void UpdateWorld(World& world, float dt)
{
    UpdatePhysics(world.player, dt);
    for (auto& b : world.balls) UpdateSphere(b, dt);

    //footstep SFX
    if (world.soundEngine)
    {
        glm::vec3 curPos = world.player.pos;

        //horizontal speed
        glm::vec2 deltaXZ(curPos.x - world.lastPlayerPos.x,
            curPos.z - world.lastPlayerPos.z);
        float dist = glm::length(deltaXZ);
        float instSpeed = (dt > 0.0f) ? (dist / dt) : 0.0f;

        //simple smoother so no jitter
        static float smoothedSpeed = 0.0f;
        const float smoothing = 0.2f;
        smoothedSpeed = smoothing * instSpeed + (1.0f - smoothing) * smoothedSpeed;

        //audio thresholds
        const float walkStartThreshold = 0.5f; //start walking above this
        const float walkStopThreshold = 0.3f; //stop walking below this

        static bool   isWalking = false;
        static irrklang::ISound* currentFootstep = nullptr;

        //cycling footstep sounds 1-4
        static int footstepIndex = 0;
        static const char* const footstepFiles[4] = {
            "media/music/dirt1.wav",
            "media/music/dirt2.wav",
            "media/music/dirt3.wav",
            "media/music/dirt4.wav"
        };

        if (world.player.grounded)
        {
            if (!isWalking && smoothedSpeed > walkStartThreshold)
                isWalking = true;
            else if (isWalking && smoothedSpeed < walkStopThreshold)
                isWalking = false;
        }
        else
        {
            //in air = no footsteps
            isWalking = false;
        }

        if (isWalking)
        {
            world.footstepTimer += dt;
            if (world.footstepTimer >= world.footstepInterval)
            {
                world.footstepTimer = 0.0f;

                //pick current footstep sound and advance to the next
                const char* footstepPath = footstepFiles[footstepIndex];
                footstepIndex = (footstepIndex + 1) % 4;

                std::cout << "FOOTSTEP (" << footstepPath << ")\n";

                //ensure no overlapping instances
                if (currentFootstep)
                {
                    currentFootstep->stop();
                    currentFootstep->drop();
                    currentFootstep = nullptr;
                }

                currentFootstep =
                    world.soundEngine->play2D(
                        footstepPath,
                        false,   //no loop
                        true,    //start paused so we can configure safely
                        true     //return ISound*
                    );

                if (currentFootstep)
                {
                    const float volume = 0.05f;
                    currentFootstep->setVolume(volume);
                    currentFootstep->setPan(0.0f);

                    //debug: log final volume and pan
                    std::cout << "Footstep vol=" << currentFootstep->getVolume()
                        << " pan=" << currentFootstep->getPan() << "\n";

                    currentFootstep->setIsPaused(false);
                }
            }
        }
        else
        {
            // Not walking so reset timer so next step has a full interval
            world.footstepTimer = 0.0f;
        }

        world.lastPlayerPos = curPos;
    }

    //song duration control
    if (world.cucarachaSound)
    {
        world.cucarachaTimer += dt;
        if (world.cucarachaTimer >= 30.0f)
        {
            world.cucarachaSound->stop();
            world.cucarachaSound->drop();
            world.cucarachaSound = nullptr;
            world.cucarachaTimer = 0.0f;
            std::cout << "La Cucaracha stopped after 30 seconds.\n";
        }
    }

    //QTE timer
    if (world.qteActive && world.qteVisible)
    {
        world.qteTimer += dt;
        float t = world.qteTimer / world.qteOuterShrinkTime;
        t = glm::clamp(t, 0.0f, 1.0f);

        world.qteOuterRadius = world.qteOuterMaxRadius * (1.0f - t);

        if (t >= 1.0f && !world.qteThisRoundHit)
        {
            //time ran out
            std::cout << "QTE timeout, reset\n";
            world.qteActive = false;
            world.qteVisible = false;
            world.qteCurrentHits = 0;
            world.qteTimer = 0.0f;
            world.qteOuterRadius = world.qteOuterMaxRadius;
            world.qteThisRoundHit = false;
            world.qteCompleted = false;
        }
    }

    const float skullSquareRange = 2.0f;


        //skull mode update
    if (world.skullModeActive)
    {
        world.skullModeTime += dt;
        world.skullSpawnTimer += dt;

        //survive 25 seconds award star
        if (world.skullModeTime >= 25.0f && !world.starSkullAwarded)
        {
            world.skullModeSurvived = true;
            world.skullModeFailed = false;

            world.starSkullAwarded = true;
            world.starCount++;
            std::cout << "Skull mode SURVIVED 25s! +1 STAR (total: " << world.starCount << ")\n";
        }

        // Decrease spawn interval over time
        float baseInterval = 2.0f;          //starting interval
        float minInterval = 0.05f;         // absolute minimum

        //exponential decay interval = baseInterval * exp(-k * time)
        float k = 0.05f;                    //controls how fast it ramps
        float interval = baseInterval * std::exp(-k * world.skullModeTime);
        world.skullSpawnInterval = std::max(minInterval, interval);

        //spawn when timer exceeds interval
        while (world.skullSpawnTimer >= world.skullSpawnInterval && world.skullModeActive)
        {
            world.skullSpawnTimer -= world.skullSpawnInterval;
            SpawnSkullTowardsPlayer(world);
        }

        //move skulls and check collisions
        const float playerHitRadius = 0.7f;
        const float skullKillRadius = 1.0f;
        for (auto& s : world.skulls)
        {
            if (!s.active) continue;
            s.pos += s.vel * dt;

            float d = glm::length(s.pos - world.player.pos);
            if (d <= (playerHitRadius + skullKillRadius))
            {
                //player got hit fail, clear skulls, allow retry
                std::cout << "Skull mode FAILED (hit by skull)\n";

                world.skullModeActive = false;
                world.skullModeFailed = true;
                world.skullModeSurvived = false;
                ResetSkullMode(world);  //clears skulls and timers
                break;
            }

            if (glm::length(s.pos - world.player.pos) > 60.0f)
                s.active = false;
        }
    }

    //victory cockroaches
    if (!world.starsCelebrationDone && world.starCount >= 4)
    {
        world.starsCelebrationDone = true;

        const int   extraCount = 8;
        const float radius = 3.0f;   //distance from player
        const float baseY = 0.1f;

        for (int i = 0; i < extraCount; ++i)
        {
            float t = static_cast<float>(i) / static_cast<float>(extraCount);
            float ang = t * 2.0f * static_cast<float>(M_PI);

            glm::vec3 pos(
                world.player.pos.x + std::cos(ang) * radius,
                baseY,
                world.player.pos.z + std::sin(ang) * radius
            );

            CockroachInstance inst;
            inst.pos = pos;
            inst.dancing = true;
            inst.time = 0.0f;
            world.cockroaches.push_back(inst);
        }

        std::cout << "4 STARS REACHED! Summoning 8 dancing cockroaches around you\n";

        //start song for up to 30 seconds
        if (world.soundEngine)
        {
            //stop previous instance if somehow still playing
            if (world.cucarachaSound)
            {
                world.cucarachaSound->stop();
                world.cucarachaSound->drop();
                world.cucarachaSound = nullptr;
            }

            world.cucarachaSound =
                world.soundEngine->play2D("media/music/La Cucaracha.mp3",
                    false,     
                    false,     
                    true);     

            if (world.cucarachaSound)
            {
                world.cucarachaTimer = 0.0f;
                std::cout << "Playing La Cucaracha!\n";
            }
            else
            {
                std::cout << "Failed to play La Cucaracha.mp3\n";
            }
        }
    }

    for (int it = 0; it < 8; ++it)
    {
        // ball–ball
        for (int i = 0; i < (int)world.balls.size(); ++i)
            for (int j = i + 1; j < (int)world.balls.size(); ++j)
                ResolveSphereSphere(world.balls[i], world.balls[j]);

        // ball–boulder
        for (auto& b : world.balls)
            for (auto& r : world.boulderWall)
                ResolveSphereAABB(b, r);

        // ball–ballpit
        for (auto& b : world.balls)
            for (auto& w : world.ballPitWalls)
                ResolveSphereAABB(b, w);

        // ball–player
        for (auto& b : world.balls)
            ResolveSphereAABB(b, world.player);

        // player–boulders
        for (auto& r : world.boulderWall)
            if (AABBCollide(world.player, r))
                ResolveAABB(world.player, r);
    }
}

void RenderWorld(World& world,
    Shader& shader,
    Shader& depthShader,
    const glm::mat4& lightSpace,
    const glm::mat4& view,
    const glm::mat4& proj,
    unsigned int depthTex,
    unsigned int depthFBO,
    unsigned int SHW,
    unsigned int SHH)
{
    //shadow pass
    glViewport(0, 0, SHW, SHH);
    glBindFramebuffer(GL_FRAMEBUFFER, depthFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    depthShader.use();
    depthShader.setMat4("lightSpaceMatrix", lightSpace);

    auto DrawDepth = [&](Shader& s)
        {
            glm::mat4 M(1);
            M = glm::scale(M, glm::vec3(100, 1, 100));
            s.setMat4("model", M);

            glBindVertexArray(world.planeVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);

            auto DrawModel = [&](Model& m, glm::vec3 pos, float scale)
                {
                    glm::mat4 mo(1);
                    mo = glm::translate(mo, pos);
                    mo = glm::scale(mo, glm::vec3(scale));
                    s.setMat4("model", mo);

                    if (!m.meshes.empty())
                        m.meshes[0].Draw(s);
                };

            for (auto& r : world.boulderWall)
                DrawModel(*world.boulder, r.pos, world.boulderScale);

            glBindVertexArray(world.sphereVAO);
            for (auto& b : world.balls)
            {
                glm::mat4 mo(1);
                mo = glm::translate(mo, b.pos);
                mo = glm::scale(mo, glm::vec3(b.radius));
                s.setMat4("model", mo);
                glDrawArrays(GL_TRIANGLES, 0, world.sphereVertCount);
            }

            for (auto& g : world.grass)
            {
                glm::mat4 mo(1);
                mo = glm::translate(mo, g.pos);
                mo = glm::rotate(mo, glm::radians(g.rot), glm::vec3(0, 1, 0));
                mo = glm::scale(mo, glm::vec3(g.scale));
                s.setMat4("model", mo);

                if (g.type == 0) world.grass1->Draw(s);
                else if (g.type == 1) world.grass2->Draw(s);
                else world.grass3->Draw(s);
            }

            if (world.cockroach && !world.cockroach->meshes.empty())
            {
                const float roachScale = 0.5f;
                for (const auto& rInst : world.cockroaches)
                {
                    glm::mat4 mo(1.0f);
                    glm::vec3 pos = rInst.pos;

                    mo = glm::translate(mo, pos);

                    if (rInst.dancing)
                    {
                        float t = rInst.time;

                        float hopBase = 1.6f;
                        float hopHeight = 0.4f * std::sin(t * 12.0f);
                        mo = glm::translate(mo, glm::vec3(0.0f, hopBase + hopHeight, 0.0f));

                        mo = glm::rotate(mo, glm::radians(-80.0f), glm::vec3(1, 0, 0));
                        mo = glm::rotate(mo, glm::radians(20.0f), glm::vec3(0, 0, 1));
                        mo = glm::rotate(mo, t * 8.0f, glm::vec3(0, 0, 1));
                    }

                    mo = glm::scale(mo, glm::vec3(roachScale));
                    s.setMat4("model", mo);
                    world.cockroach->Draw(s);
                }
            }
        };

    DrawDepth(depthShader);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //main pass

    glViewport(0, 0, world.screenWidth, world.screenHeight);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    shader.use();
    shader.setMat4("projection", proj);
    shader.setMat4("view", view);
    shader.setMat4("lightSpaceMatrix", lightSpace);

    //QTE circle
    shader.setInt("qteVisible", world.qteVisible ? 1 : 0);
    shader.setFloat("qteInnerRadius", world.qteInnerRadius);
    shader.setFloat("qteOuterRadius", world.qteOuterRadius);
    shader.setVec3("qteInnerColor", glm::vec3(1.0f, 1.0f, 1.0f));   //white inner
    shader.setVec3("qteOuterColor", glm::vec3(1.0f, 0.2f, 0.2f));   //red outer

    //aspect ratio for QTE circle
    float aspect = (world.screenHeight != 0)
        ? static_cast<float>(world.screenWidth) / static_cast<float>(world.screenHeight)
        : 800.0f / 600.0f;
    shader.setFloat("qteAspect", aspect);
    shader.setVec2("qteScreenSize", glm::vec2(world.screenWidth, world.screenHeight));

    shader.setInt("shadowMap", 1);
    shader.setInt("groundTex", 2);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTex);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, world.groundTex);

    //ground
    glm::mat4 GM(1);
    GM = glm::scale(GM, glm::vec3(100, 1, 100));
    shader.setMat4("model", GM);
    shader.setVec3("overrideColor", glm::vec3(-1));
    shader.setInt("useTexture", 1);
    glBindVertexArray(world.planeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    //ball pit box
    {
        glm::mat4 mo(1);
        glm::vec3 pitCenter(0.0f, 0.5f, -10.0f);
        float pitRadius = 4.0f;
        float pitHeight = 1.4f;

        mo = glm::translate(mo, glm::vec3(pitCenter.x, 0.0f, pitCenter.z));
        mo = glm::scale(mo, glm::vec3(pitRadius, pitHeight, pitRadius));

        shader.setMat4("model", mo);
        shader.setInt("useTexture", 0);
        shader.setVec3("overrideColor", glm::vec3(0.2f, 0.6f, 1.0f));

        glBindVertexArray(world.pitVAO);
        glDrawArrays(GL_TRIANGLES, 0, world.pitVertCount);

        shader.setVec3("overrideColor", glm::vec3(-1.0f));
    }

    //me
    if (world.meTex != 0)
    {
        //position
        glm::mat4 mo(1.0f);
        glm::vec3 pitCenter(-1.5f, 0.5f, -10.0f);

        //behind pit
        glm::vec3 imgPos(
            pitCenter.x,
            0.001f,          //offset above ground
            pitCenter.z - 6.0f
        );

        float imgWidth = 1.0f;  //size
        float imgHeight = 1.0f;  //aspect

        mo = glm::translate(mo, imgPos);

        mo = glm::rotate(mo, glm::radians(-90.0f), glm::vec3(1, 0, 0));

        mo = glm::scale(mo, glm::vec3(imgWidth, imgHeight, 1.0f));

        shader.setMat4("model", mo);

        shader.setVec3("overrideColor", glm::vec3(-1.0f));
        shader.setInt("useTexture", 1);

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, world.meTex);

        glBindVertexArray(world.uiQuadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        //ground texture
        glBindTexture(GL_TEXTURE_2D, world.groundTex);
        glBindVertexArray(0);
        shader.setInt("useTexture", 0);
    }

    //grass
    shader.setInt("useTexture", 0);
    for (auto& g : world.grass)
    {
        glm::mat4 mo(1);
        mo = glm::translate(mo, g.pos);
        mo = glm::rotate(mo, glm::radians(g.rot), glm::vec3(0, 1, 0));
        mo = glm::scale(mo, glm::vec3(g.scale));
        shader.setMat4("model", mo);
        shader.setVec3("overrideColor", glm::vec3(0.1f, 0.7f, 0.1f));

        if (g.type == 0) world.grass1->Draw(shader);
        else if (g.type == 1) world.grass2->Draw(shader);
        else world.grass3->Draw(shader);
    }
    shader.setVec3("overrideColor", glm::vec3(-1.0f));
    shader.setInt("useTexture", 0);

    //boulders
    for (auto& r : world.boulderWall)
    {
        glm::mat4 mo(1);
        mo = glm::translate(mo, r.pos);
        mo = glm::scale(mo, glm::vec3(world.boulderScale));
        shader.setMat4("model", mo);

        shader.setVec3("overrideColor", glm::vec3(0.5f));
        if (!world.boulder->meshes.empty())
            world.boulder->meshes[0].Draw(shader);
    }

    //balls
    glBindVertexArray(world.sphereVAO);
    for (int i = 0; i < (int)world.balls.size(); ++i)
    {
        const auto& b = world.balls[i];

        glm::mat4 mo(1);
        mo = glm::translate(mo, b.pos);
        mo = glm::scale(mo, glm::vec3(b.radius));
        shader.setMat4("model", mo);

        if (i == world.goldenBallIndex)
            shader.setVec3("overrideColor", glm::vec3(1.0f, 0.9f, 0.1f));
        else
            shader.setVec3("overrideColor", glm::vec3(1.0f, 0.95f, 0.6f));

        glDrawArrays(GL_TRIANGLES, 0, world.sphereVertCount);
    }
    shader.setVec3("overrideColor", glm::vec3(-1.0f));

    //roaches
    if (world.cockroach && !world.cockroach->meshes.empty())
    {
        const float roachScale = 0.5f;

        for (const auto& rInst : world.cockroaches)
        {
            glm::mat4 mo(1.0f);
            glm::vec3 pos = rInst.pos;

            mo = glm::translate(mo, pos);

            if (rInst.dancing)
            {
                float t = rInst.time;

                float hopBase = 1.6f;
                float hopHeight = 0.4f * std::sin(t * 12.0f);
                mo = glm::translate(mo, glm::vec3(0.0f, hopBase + hopHeight, 0.0f));

                mo = glm::rotate(mo, glm::radians(-80.0f), glm::vec3(1, 0, 0));
                mo = glm::rotate(mo, glm::radians(20.0f), glm::vec3(0, 0, 1));
                mo = glm::rotate(mo, t * 8.0f, glm::vec3(0, 0, 1));
            }

            mo = glm::scale(mo, glm::vec3(roachScale));
            shader.setMat4("model", mo);

            shader.setVec3("overrideColor", glm::vec3(-1.0f));
            shader.setInt("useTexture", 0);
            world.cockroach->Draw(shader);
        }
    }

    //QTE pillar
    {
        glm::mat4 mo(1.0f);
        glm::vec3 pos = world.pedestalPos;

        float pillarHalfSize = 0.4f; 
        float pillarHeight = 5.0f;  

        //pillar base at ground
        mo = glm::translate(mo, glm::vec3(pos.x, pos.y, pos.z));
        mo = glm::scale(mo, glm::vec3(pillarHalfSize, pillarHeight, pillarHalfSize));

        shader.setMat4("model", mo);
        shader.setVec3("overrideColor", glm::vec3(0.9f, 0.9f, 0.4f));

        glBindVertexArray(world.pitVAO);
        glDrawArrays(GL_TRIANGLES, 0, world.pitVertCount);

        //button
        {
            glm::mat4 btn(1.0f);

            float buttonSize = pillarHalfSize * 0.8f; 
            float buttonDepth = 0.1f;        

            //2/5ths up
            float buttonCenterY = pos.y + pillarHeight * 0.4f;

            glm::vec3 buttonPos(
                pos.x,
                buttonCenterY,
                pos.z + pillarHalfSize + buttonDepth * 0.8f
            );

            btn = glm::translate(btn, buttonPos);
            btn = glm::scale(btn, glm::vec3(buttonSize, buttonSize, buttonDepth));

            shader.setMat4("model", btn);
            shader.setVec3("overrideColor", glm::vec3(1.0f, 0.2f, 0.2f)); //red
            glDrawArrays(GL_TRIANGLES, 0, world.pitVertCount);
        }

        //red sphere on top of the pillar
        {
            if (world.sphereVAO && world.sphereVertCount > 0)
            {
                glm::mat4 sphereM(1.0f);

                //sphere at centre on pillar
                float sphereRadius = pillarHalfSize * 0.9f;
                glm::vec3 topPos(
                    pos.x,
                    pos.y + pillarHeight + sphereRadius,  //centre
                    pos.z
                );

                sphereM = glm::translate(sphereM, topPos);
                sphereM = glm::scale(sphereM, glm::vec3(sphereRadius));

                shader.setMat4("model", sphereM);
                shader.setVec3("overrideColor", glm::vec3(1.0f, 0.1f, 0.1f)); //red

                glBindVertexArray(world.sphereVAO);
                glDrawArrays(GL_TRIANGLES, 0, world.sphereVertCount);
            }
        }

        glBindVertexArray(0);
        shader.setVec3("overrideColor", glm::vec3(-1.0f));
    }


    //skulls
    if (world.skull && !world.skull->meshes.empty())
    {
        const float skullScale = 0.6f;
        for (const auto& sInst : world.skulls)
        {
            if (!sInst.active) continue;

            glm::mat4 mo(1.0f);
            mo = glm::translate(mo, sInst.pos);

			//face towards movement direction
            glm::vec3 dir = glm::normalize(sInst.vel);
            if (glm::length(dir) > 0.0001f)
            {
                float yaw = std::atan2(dir.x, dir.z);
                mo = glm::rotate(mo, yaw, glm::vec3(0, 1, 0));
            }

            mo = glm::scale(mo, glm::vec3(skullScale));
            shader.setMat4("model", mo);

            shader.setVec3("overrideColor", glm::vec3(0.7f, 0.2f, 0.9f));
            world.skull->Draw(shader);
        }

        shader.setVec3("overrideColor", glm::vec3(-1.0f));
    }

    //pillar where you stand to start skull mode
    {
        glm::mat4 mo(1.0f);
        glm::vec3 pos = world.skullSquarePos;

        float pillarHalfSize = 0.4f;
        float pillarHeight = 5.0f;

        mo = glm::translate(mo, glm::vec3(pos.x, pos.y, pos.z));
        mo = glm::scale(mo, glm::vec3(pillarHalfSize, pillarHeight, pillarHalfSize));
        shader.setMat4("model", mo);

        glm::vec3 col(0.7f, 0.2f, 0.9f);                  
        if (world.skullModeActive)        col = glm::vec3(1.0f, 0.1f, 0.1f);  //active red
        else if (world.skullModeSurvived) col = glm::vec3(0.1f, 1.0f, 0.1f);  //survived green
        else if (world.skullModeFailed)   col = glm::vec3(0.4f, 0.4f, 0.4f);  //failed gray

        shader.setVec3("overrideColor", col);

        glBindVertexArray(world.pitVAO);
        glDrawArrays(GL_TRIANGLES, 0, world.pitVertCount);

        //square same position as QTE
        {
            glm::mat4 btn(1.0f);

            float buttonSize = pillarHalfSize * 0.8f;
            float buttonDepth = 0.1f;

            float buttonCenterY = pos.y + pillarHeight * 0.4f;

            glm::vec3 buttonPos(
                pos.x,
                buttonCenterY,
                pos.z + pillarHalfSize + buttonDepth * 0.8f
            );

            btn = glm::translate(btn, buttonPos);
            btn = glm::scale(btn, glm::vec3(buttonSize, buttonSize, buttonDepth));

            shader.setMat4("model", btn);
            shader.setVec3("overrideColor", glm::vec3(1.0f, 0.2f, 0.2f)); // red
            glDrawArrays(GL_TRIANGLES, 0, world.pitVertCount);
        }

        //skull ontop of pillar
        if (world.skull && !world.skull->meshes.empty())
        {
            glm::mat4 skullM(1.0f);

            float skullOffsetUp = 0.8f;
            glm::vec3 topPos(
                pos.x,
                pos.y + pillarHeight + skullOffsetUp,
                pos.z
            );

            skullM = glm::translate(skullM, topPos);

            skullM = glm::rotate(skullM, glm::radians(0.0f), glm::vec3(0, 1, 0));

            float skullScaleTop = 0.8f;
            skullM = glm::scale(skullM, glm::vec3(skullScaleTop));

            shader.setMat4("model", skullM);
            shader.setVec3("overrideColor", glm::vec3(-1.0f));
            world.skull->Draw(shader);
        }

        glBindVertexArray(0);
        shader.setVec3("overrideColor", glm::vec3(-1.0f));
    }




    //star counter in top right
    {
        float w = static_cast<float>(world.screenWidth);
        float h = static_cast<float>(world.screenHeight);

        glm::mat4 uiProj = glm::ortho(0.0f, w, 0.0f, h);

        shader.use();
        shader.setInt("isUI", 1);
        shader.setMat4("uiProjection", uiProj);

        //star size and padding in pixels
        float starSize = 32.0f;
        float padding = 8.0f;

        int starsToDraw = std::min(world.starCount, 4);

        for (int i = 0; i < starsToDraw; ++i)
        {
            //right to left
            float x = w - padding - starSize * (i + 1);
            float y = h - padding - starSize;

            glm::mat4 m(1.0f);
            m = glm::translate(m, glm::vec3(x, y, 0.0f));
            m = glm::scale(m, glm::vec3(starSize, starSize, 1.0f));

            shader.setMat4("model", m);

            //golden color
            shader.setVec3("overrideColor", glm::vec3(1.0f, 0.9f, 0.3f));
            shader.setInt("useTexture", 0);

            glBindVertexArray(world.uiQuadVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glBindVertexArray(0);

        //reset to normal 3D rendering state
        shader.setInt("isUI", 0);
        shader.setVec3("overrideColor", glm::vec3(-1.0f));
    }

    shader.setVec3("overrideColor", glm::vec3(-1));
}