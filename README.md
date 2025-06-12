# Interactive-3D-simulation-of-a-boat-with-openGL

Interactive 3D simulation of a boat featurs realistic water flow and dynamic day/night lighting modes. The boat floats naturally on water and can be controlled using keyboard input. User can freely adjust the camera to view the boat from any angle.

![final](https://github.com/user-attachments/assets/c2294493-0737-4be8-9bd5-4c1e530ab63d)

<img width="362" alt="Снимок экрана 2024-01-21 162423" src="https://github.com/user-attachments/assets/2cbf4fb4-4b65-4e58-b586-56d1edbd9f0e" />
<img width="362" alt="Снимок экрана 2024-01-21 162716" src="https://github.com/user-attachments/assets/8788908b-95d5-4adf-a5ff-80df9123fae7" />

## Controls & Interactive Usage

You can interact with the scene using both keyboard and mouse inputs:

### Camera Controls
- `0` – Reset camera to orbit mode, looking at the center
- `1` – Detach camera from boat (free view)
- `2` – Attach camera to follow the boat

### Lighting & Shading
- `C` – Toggle Blinn-Phong shading (on/off)
- `B` – Toggle binary search refinement for reflections (on/off)
- `N` – Switch to **night mode** lighting
- `M` – Switch to **day mode** lighting
- `L` – Toggle boat's spotlight (on/off)

### Boat Controls
- `W` – Increase throttle (move forward)
- `S` – Decrease throttle (move backward)
- `A` – Steer left
- `D` – Steer right

### Miscellaneous
- `P` – Take a screenshot and save as `screenshot.png`
- `ESC` – Exit the program

### Mouse Controls
- **Left Mouse Button** (hold + drag) – Orbit the camera around the scene
- **Mouse Scroll Wheel** – Zoom in/out (adjust camera distance)

## Technical Details

- **Language:** C++ with GLSL for shaders  
- **Graphics API:** OpenGL  
- **Shader Techniques:**  
  - Reflection vector computation  
  - View-space ray marching  
  - Depth buffer comparison  
- **Optimizations:**  
  - Binary search refinement for accurate intersection detection  
  - Linear interpolation for smooth texture sampling
 
 ## How to Run the Project

Follow these steps to build and run the application:

1. Open a terminal or command prompt.
2. Build the project using CMake:
   ```bash cmake --build build```
3. After the build completes, navigate to the output folder containing the executable. For example ```bash cd build/bin/Debug```
4. run executable file ```bash ./project.exe ```








