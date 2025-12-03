## Project 6: Final Project Gear Up

The project handout can be found [here](https://cs1230.graphics/projects/final/gear-up).

### Test Cases

## Depth buffers ⭐️
Depths buffers are implemented using a depth attachement to an fbo, and use this as a texture as an input to a shader `depth.frag` that displays depth as a colorful gradient.
### Parse Matrix
<img width="600" height="400" alt="parse_matrix_depth" src="https://github.com/user-attachments/assets/37621d39-060d-4c50-9e36-2560cc6901da" />

### Directional_lights_1
<img width="600" height="400" alt="directional_light_1_depth" src="https://github.com/user-attachments/assets/fcc6bc2b-74ec-4b55-9cc0-bd0cd762bc7e" />

### Recursive spheres
<img width="600" height="400" alt="recursive_sphere_4_depth" src="https://github.com/user-attachments/assets/abcad293-0c20-491f-aaea-c79a9e1328d3" />

## G-buffer ⭐️⭐️
In order to demonstrate the use of geometry buffer, I decided to save as texture velocities along the two axes of my objects, relative to the camera. The motivation behind this is that I'll use velocities in the next section for motion blur.

Velocity is computed using the difference in position at time `t` and time `t-1`. The difference is done in screen space using `ViewProjectionMatrix`and `previousViewProjectionMatrix`in `velocity.frag`.

On the following video, blue means a zero speed.
### Velocity in X
https://github.com/user-attachments/assets/c9392f50-57de-4ee1-8e38-8c3f4e0191e9

### Velocity in Y
https://github.com/user-attachments/assets/be66323b-29da-4471-9a30-3cb1614cf6da

## Screen-space motion-blur ⭐️⭐️
Screen space motion blur is achieved by mixing the current position's color and a few more along the previous positions. In order to compute a pixel color, I sample from my scene saved as a texture in `fbo_scene` rendered witht the phong `default.frag` shader. I sample at the current position but also at $uv + k* displacement$ and average to get the blur. This transformation is done in `motion_blur.frag`.

### Blur with 10 samples along the displacement vector
https://github.com/user-attachments/assets/f65ddf66-b48e-43ba-be77-01cb07f9a0ff

### Blur with 5 samples along the displacement vector
https://github.com/user-attachments/assets/5d287cd1-5f15-4330-96ca-b4ad0805b044

### Design Choices
The `realtime.h` class ended up having a lot of methods so I created `realtime_lights.cpp`, `realtime_geometry.cpp`, `realtime_fbo.cpp` and `realtime_paint.cpp` that focuses on what their name suggests, while `realtime.cpp` is the core of the logic (`initializeGL`, `paintGL`...).

I also have 5 different shaders each in their files in order to keep them as simple and independant as possible. `default.frag` (for phong illumination), `depth.frag` for rendering depth, `velocity.frag`and `motion_blur.frag`.

### References
- Learn OpenGL, [Deferred Shading] (https://learnopengl.com/Advanced-Lighting/Deferred-Shading)
- NVIDIA GPU GEMS, [Chapter 27. Motion Blur as a Post-Processing Effect] (https://developer.nvidia.com/gpugems/gpugems3/part-iv-image-effects/chapter-27-motion-blur-post-processing-effect)
### Known Bugs
N/A
