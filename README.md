# Subject: CSCI420 - Computer Graphics 
## Assignment 2: Simulating a Roller Coaster
### Author: Mahesh Joseph Sadashiv
#### USC ID: 4911440672

**Description:** In this assignment, we use Catmull-Rom splines along with OpenGL core profile shader-based texture mapping and Phong shading to create a roller coaster simulation.

**Core Credit Features:**
1. Uses OpenGL core profile, version 3.2 or higher - 
2. Completed all Levels:
    - Level 1 : - done
    - Level 2 : - done
    - Level 3 : - done
    - Level 4 : - done
    - Level 5 : - done
3. Rendered the camera at a reasonable speed in a continuous path/orientation - yes
4. Run at interactive frame rate (>15fps at 1280 x 720) - yes
5. Understandably written, well-commented code - yes
6. Attached an Animation folder containing not more than 1000 screenshots - yes
7. Attached this ReadMe File - yes

**Extra Credit Features:**
1. Render a T-shaped rail cross-section - yes
2. Render a Double Rail - yes
3. Made the track circular and closed it with C1 continuity - No
4. Any Additional Scene Elements? (list them here) - no
5. Render a sky-box - yes
6. Create tracks that mimic the real-world roller coaster - no
7. Generate track from several sequences of splines - yes
8. Draw splines using recursive subdivision - no
9. Render environment in a better manner - no
10. Improved coaster normals - yes
11. Modify velocity with which the camera moves - yes
12. Derive the steps that lead to the physically realistic equation of updating u - yes

**Open-Ended Problems:** (Please document approaches to any open-ended problems that you have tackled)
1. Dynamic crosstie creation - created cross ties to align perpendicular with the track using the normals and binormals

**Keyboard/Mouse controls:** (Please document Keyboard/Mouse controls if any)
1. Space bar to pause the animation

**Names of the .cpp files you made changes to:**
1. hw1.cpp
2. pipelineProgram.cpp create a function to set Vec3 uniform variables
   ```java
   void PipelineProgram::SetUniformVariableVec3(const char *name, float value[]) {
        glUniform3f(GetUniformVariableHandle(name), value[0], value[1], value[2]);
   }
    ```

## Installation and Execution
1. Compile the program using a C++ compiler. Make sure to link OpenGL, GLUT, and GLEW libraries properly.
2. Run the executable with a heightmap image file as an argument.
   ```
   make
   ./hw1 /splines.txt
    ```
3. Make sure that mention the .sp file names in splines.txt 