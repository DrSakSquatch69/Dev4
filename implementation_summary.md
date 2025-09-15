# Assignment 3 Implementation Summary

## Overview
We have successfully implemented all the requirements for Assignment 3, which focused on adding physics and enemy logic to the game. The implementation includes velocity-based movement, collision detection, and enemy shattering behavior.

## Implemented Features

### 1. Velocity Component and Movement System
- Created a `Velocity` component in GameComponents.h with direction and speed properties
- Implemented a velocity system in GameManager.cpp that updates entity transforms based on velocity and delta time
- Applied the velocity system to both projectiles and enemies

### 2. Direction Setting
- Set projectile velocity based on arrow key input direction in player.cpp
- Implemented random diagonal direction generation for enemies in CreateEnemy function
- Ensured diagonal movement by validating that both x and z components are significant

### 3. Collision System
- Added `Collidable` tag to entities that should participate in collision detection
- Implemented collision detection in GameManager.cpp's CheckCollisions function
- Created a `ToDestroy` tag for entities that should be removed from the game
- Implemented bounce physics for enemies when they collide with obstacles
- Added logic to destroy projectiles when they hit walls or enemies

### 4. Enemy Shattering
- Created a `Shatters` component with properties for shatter count, amount, and scale
- Implemented enemy shattering logic when hit by projectiles
- Created smaller versions of enemies with reduced shatter count
- Applied scaling and positioning to shattered enemy pieces
- Assigned new random velocities to shattered pieces

## Potential Improvements

1. **Collision Detection**: The current collision detection is simplified and could be improved with proper OBB-OBB collision detection.

2. **Physics Accuracy**: The bounce physics could be more realistic by calculating reflection vectors based on collision normals.

3. **Performance Optimization**: For a large number of entities, spatial partitioning could be implemented to reduce collision checks.

4. **Visual Feedback**: Adding visual effects for collisions and shattering would enhance the game experience.

5. **Sound Effects**: Adding sound effects for firing, collisions, and shattering would make the game more immersive.

## Conclusion
The implementation successfully meets all the requirements specified in Assignment 3. The game now has enemies that move around with velocity, bounce off walls, and shatter into smaller versions when hit by projectiles. The player can fire projectiles in different directions, and collisions are properly detected and handled.