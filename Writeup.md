# COMP3016 Coursework 2 – OpenGL Prototype

## 1. Project Overview

This repository contains the source code and assets for my COMP3016 Coursework 2 OpenGL prototype, implemented in C++14 using modern OpenGL.  

The prototype is a small first‑person playground built around a ball pit, environmental props, and two interactive minigames:

- A Quick Time Event (QTE) pedestal that requires precise timing.
- A Skull survival mode that gradually ramps difficulty.

The project demonstrates:

- modern OpenGL with custom vertex/fragment shaders.
- model loading using ASSIMP.
- texturing and basic real time lighting with a shadow map pass.
- procedural content generation (PCG) for environment elements and enemies.
- Bbsic physics/interaction for a ball pit and player movement.
- audio via irrKlang (footsteps and event music).
- a simple HUD (star counter) using a UI projection pass.

---

## 2. Video Link

   TODO: `<https://youtu.be/your_unlisted_video_id>`  

## 3. Dependencies

all libraries are permitted according to the module spec.

### Runtime / Build Dependencies

- **C++14**
- **OpenGL 4.x core profile**
- **GLFW** – window creation, input handling and context:
  - Keyboard/mouse input (WASD + mouse look, E for interactions).
- **GLAD** – OpenGL function loader.
- **GLM** – math library for vectors/matrices.
- **ASSIMP** – loading 3D models.
- **stb_image** – image loading for textures.
- **irrKlang** – audio playback.

### Project Structure (High Level)

- `main.cpp` – initialization, window + GL context, main loop
- `World.h / World.cpp` – main game state, update and render functions.
- `Physics.h / Physics.cpp` – simple physics and collision helpers.
- `model_loading.vert / model_loading.frag` – main vertex & fragment shaders.
- `stb_image.cpp` – stb_image implementation unit.
- `media/` – models, textures, music, SFX.

---

## 4. Gameplay Description

### 4.1 Core Loop

The player starts in a **3D outdoor environment** containing:

- A large ball pit filled with many physics‑simulated balls.
- A surrounding ring of boulders enclosing the space.
- Procedural grass patches scattered across the terrain.
- A QTE pillar and a Skull mode pillar.
- Several cockroaches that can dance under certain conditions.

The typical loop:

1. Explore and move around the environment using first person controls.
2. Interact with special locations:
   - QTE pillar: stand near and press **E** to start the timing mini‑game.
   - Skull pillar: stand near and press **E** to start skull survival.
3. Earn stars by:
   - Completing the QTE mini‑game (once per game).
   - Surviving long enough in skull mode.
   - Finding/using the golden ball.
4. Upon reaching 4 stars, the game celebrates with dancing cockroaches and La Cucaracha music.

### 4.2 Key Mechanics

- **Movement**
  - Standard WASD + mouse look with gravity and collision against boulders and the ball pit walls.
  - Footstep SFX when the player is grounded and moving.

- **Ball Pit**
  - 150 spheres spawned procedurally inside a square pit.
  - Simple sphere–sphere and sphere–AABB collision.
  - One ball may be designated as a golden ball.

- **QTE pillar**
  - Standing within 2 units of the pillar and pressing **E** starts a QTE.
  - On screen, two circles appear: inner (target) and a shrinking outer circle.
  - When the outer circle reaches the inner circle, pressing the QTE key counts as a **hit**.
  - 8 hits are required for completion; each successful hit speeds up the next round.
  - Failing or missing a timing window resets the QTE.
  - First full completion grants 1 star.

- **Skull Survival Mode**
  - Standing near the skull pedestal and pressing **E** starts skull mode.
  - Skulls spawn at the edge of a circle around the player and fly toward them.
  - Spawn rate accelerates over time using an exponential decay function for the interval.
  - Surviving 25 seconds without being hit awards 1 star.
  - Getting hit resets the mode and allows you to start again.
  - Mode can continue even after the star is earned, difficulty keeps ramping.

- **Star System & Celebration**
  - Stars are used as progress, max of 4 visible on the UI.
  - Each earned star adds to the HUD.
  - Upon reaching 4 stars:
    - 8 dancing cockroaches spawn in a circle around the player.
    - La Cucaracha plays via irrKlang.
    - Cockroaches hop and spin in a stylized dance animation.

---

## 5. Use of AI

I used AI tools (GitHub copilot) in these ways:

  - Generating initial openGL setup code patterns.
  - Suggesting example structures for physics helpers and game loop code.

  - Asking for explanations of GL errors.
  - Getting suggestions for organizing my code and creating function names.

I did not copy complete gameplay systems from AI output. 
I reviewed the code given and, understood it and wrote new code using the knowledge the AI had given. 
All AI assisted content has been reviewed, understood, and made by me, and I am the author of all the final implementations.

---

## 6. Game Programming Patterns Used

### 6.1 “World” as a game state container

- `struct World` in `World.h` holds all global game states:
  - Player, physics bodies, collections of balls, boulders, and roaches.
  - QTE state (`qteActive`, `qteTimer`, radii, hit counters).
  - Skull mode state (`skullModeActive`, timers, skull list).
  - Audio handles (footsteps, La Cucaracha).
  - Rendering handles (VAOs, VBOs, textures).

This resembles a Scene pattern, `UpdateWorld` and `RenderWorld` operate on a single `World` instance passed around as a context.

### 6.2 Update and render separation

- `UpdateWorld(world, dt)`:
  - Handles physics updates, timers, QTE/skull state changes, star logic, and audio.
- `RenderWorld(world, ...)`:
  - Performs a shadow map pass and a main pass.
  - Draws ground, pit, boulders, balls, props, characters, and UI overlay.

This is a game loop pattern with an update step and a render step.

### 6.3 Structures for simple entities

- Entities like `Sphere`, `PhysicsBody`, `SkullInstance`, and `CockroachInstance` have minimal data fields:
  - `pos`, `vel`, `radius`, `active`, etc.
- Behavior is implemented as functions:
  - `UpdateSphere`, `ResolveSphereSphere`, `ResolveSphereAABB`, etc.

### 6.4 Procedural generation functions

- Helper functions like `GenerateSphereMesh`, `GeneratePlane`, `GenerateCylinderMesh`, `GeneratePedestalMesh` have geometric generation.

---

## 7. Game Mechanics and How They Are Coded

This section focuses on the two main mini‑games and key systems.

### 7.1 QTE mini game

 logic – `HandleQTEInput(World& world)`

- Checks distance between `world.player.pos` and `world.pedestalPos` using a helper `DistanceXZ`.
- If within `startRange` and not already active:
  - Sets `qteActive = true`, `qteVisible = true`, resets counters and timers.
- On each attempt:
  - If outer radius =< inner radius and not already counted:
    - Counts as a hit, increments `qteCurrentHits`.
    - If hits >= `qteTargetHits`, marks `qteCompleted`, awards star if not already awarded.
    - Every successful hit shrinks the time for the next round (`qteOuterShrinkTime *= 0.85f`).
  - Otherwise (pressed too early/late), resets the QTE state.

Timer and circle update – in `UpdateWorld`

- While active and visible:
  - Increments `qteTimer` and recomputes:
    ```cpp
    float t = world.qteTimer / world.qteOuterShrinkTime;
    t = glm::clamp(t, 0.0f, 1.0f);
    world.qteOuterRadius = world.qteOuterMaxRadius * (1.0f - t);
    ```
  - If `t >= 1.0f` and no hit, treat as timeout → reset QTE.

Rendering

- QTE circles are implemented as uniforms to the fragment shader (`qteVisible`, radii, colors, `qteAspect`, `qteScreenSize`).  

### 7.2 Skull Survival Mode

Input – `HandleSkullModeInput(World& world)`

- If not already active and player is within `startRange` of `skullSquarePos`:
  - Calls `ResetSkullMode(world)` and sets `skullModeActive = true`.

Spawning and difficulty ramp – in `UpdateWorld`

- While `skullModeActive`:
  - `skullModeTime` and `skullSpawnTimer` accumulate `dt`.
  - Compute spawn interval as an exponential decay:
    ```cpp
    float baseInterval = 2.0f;
    float minInterval = 0.05f;
    float k = 0.05f;
    float interval = baseInterval * std::exp(-k * world.skullModeTime);
    world.skullSpawnInterval = std::max(minInterval, interval);
    ```
  - When `skullSpawnTimer >= skullSpawnInterval`, repeatedly call `SpawnSkullTowardsPlayer(world)`.

Skull movement & collision

- Each active skull:
  - Moves using `s.pos += s.vel * dt;` where `vel` is directed at the player.
  - If distance to player ≤ `playerHitRadius + skullKillRadius`:
    - Failure, set `skullModeFailed = true`, call `ResetSkullMode` to clear skulls and timers.
  - If too far from player set `active = false` to recycle.

Success condition

- If `skullModeTime >= 25.0f` and star not yet awarded:
  - `skullModeSurvived = true;`
  - `starSkullAwarded = true;`
  - Increment `starCount`.

  ## 8. Sample screens

The repo has screenshots under `Screenshots/`:


## 9. exception handling and test cases

### 9.1 exception / error handling

The code uses checks and logging:

- Texture loading (`LoadTexture`):
  - If `stbi_load` fails, prints an error and returns texture ID `0`.
- Audio:
  - Always checks that `soundEngine` and returned `ISound*` are not `nullptr`.
  - On failure to play audio, logs to stdout.
- Model/VAO assumptions:
  - Many draw calls check `if (!model->meshes.empty())` before drawing.
  - World init sets defaults and reserves to avoid reallocations.

### 9.2 Informal Test Cases

The following tests were performed manually:

1. Startup test
   - Run the program with all required assets present.
   - Expected: window opens, ground + ball pit visible, no crashes.

2. Movement and collision test
   - Move towards boulders and the pit walls.
   - Expected: player cannot pass through geometry, slides along surfaces.

3. QTE mode
   - Start QTE by standing near the pillar, pressing E.
   - Hit timing correctly 8 times.
     - Expected: star count increases by 1.
   - Press at obviously wrong times.
     - Expected: QTE resets, variables zeroed.

4. Skull mode
   - Start skull mode and stand still.
     - Expected: skulls spawn increasingly fast, hitting player ends mode and sets `skullModeFailed`.
   - Start mode and actively dodge for 25 seconds.
     - Expected: star count increases by 1, mode can continue with more skulls.

5. Star celebration
   - Use debug or repeated plays to reach 4 stars.
   - Expected:
     - Additional cockroaches spawn in a circle.
     - La Cucaracha plays, then stops after a maximum of 30 seconds.
     - Further star gains do not retrigger the event.

6. Audio Robustness
   - Player movement does not cause overlapping footsteps.

---

## 10. Further details

1. Shadow map pass
   - Renders ground, boulders, balls, grass and cockroaches into a depth texture (`depthTex`).

2. Main pass
   - `model_loading.vert` and `model_loading.frag` used.
   - Binds shadow map (`shadowMap`) and ground texture.
   - Renders:
     - Ground plane (textured).
     - Ball pit walls (coloured).
     - “me” image quad on the ground.
     - Grass, boulders, balls , cockroaches, skulls.
     - QTE and Skull pedestals with colored buttons and top decorations.


---

## 11. Evaluation and Reflection

### 11.1 What I achieved

I believe this prototype successfully demonstrates a complete 3D scene build completely using C++ and OpenGL, also uses non trivial procedural generation with two fully working mini games that have a star progression system. It also includes a audio system with ambient movement and celebratory music.



### 11.2 What I would do differently

Given more time and knowing what I know now, I would:

1. Richer Visuals
   - Implement full Blinn–Phong shading with specular maps and normal maps for at least the skull and boulders.
   - Add skybox/environment lighting to improve overall ambience.

2. Better UI
   - Create a small UI framework for buttons, on‑screen prompts (like “Press E to start QTE”).

3. Polish gameplay
   - Tweak movement feel, jumping sound effects and make more minigames and have existing mini games feel more immersive and have deeper gameplay.

However, I am still happy with the final result I have made, I believe it is a solid foundation and it clearly demonstrates my understanding of game development and of C++ programming and OpenGL for this module.
---