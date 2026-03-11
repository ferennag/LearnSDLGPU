# Resources

## Graphics

- GLTF Guide: https://www.khronos.org/files/gltf20-reference-guide.pdf
- Rendering engine architecture: https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Engine_Architecture/01_introduction.html

## Physics

- Programming vehicles in video games: https://wassimulator.com/blog/programming/programming_vehicles_in_games.html and https://www.youtube.com/watch?v=MrIAw980iYg
- Racing game physics: http://autoxer.skiblack.com/phys_racing/contents.htm
- Building a physics engine: https://www.youtube.com/watch?v=TtgS-b191V0

# TODO

- Fix the texture coordinate parsing in model.c. I think currently we only parse 1 texCoord, but there can be more than one per vertex?

# Current Goals

## Rendering engine

- Look into creating a more abstract rendering engine, currently I only have 1 pipeline, etc.
https://docs.vulkan.org/tutorial/latest/Building_a_Simple_Engine/Engine_Architecture/01_introduction.html

- Skybox

- Fix PBR issues: normal map is not used currently, need to be able to handle more materials, etc

- Lighting: currently we have 1 light baked in the fragment shader, need to make this a parameter to shaders

- Shadows: Need at least some shadows, e.g. for the car so it actually looks like it is on the ground

- Revisit the whole PBR rendering

## Make the car moving

We need a basic physics engine that can make the car moving, turning, accelerating, decelrating, stopping.

For this we need the following things to be implemented:

- Basic environment: We need this so we can see that the car is actually moving. Can be a basic plane with some texture as ground, and a bunch of
environment elements, which can be cubes randomly scattered around, so we can go around them, etc.

- Physics engine: Create a physics engine that runs separately from the rendering thread, with fixed ticks 

- Simulate basic car physics

- Imgui or similar for tweaking parameters on the fly

- Sound: Add engine sounds, tire sounds, etc
