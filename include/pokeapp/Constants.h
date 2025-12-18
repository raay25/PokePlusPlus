#pragma once

/*
	Contains various constant definitions for the application, including
	window settings, camera parameters, physics constants, and visual effect settings.
*/

namespace pokepp {
    namespace constants {
        // Window settings
        constexpr int DEFAULT_WINDOW_WIDTH = 800;
        constexpr int DEFAULT_WINDOW_HEIGHT = 600;
        constexpr float DEFAULT_FOV = 60.0f;
        constexpr float NEAR_PLANE = 0.1f;
        constexpr float FAR_PLANE = 100.0f;

        // Camera settings
        constexpr float DEFAULT_MOVE_SPEED = 2.5f;
        constexpr float DEFAULT_MOUSE_SENSITIVITY = 0.1f;
        constexpr float DEFAULT_CAMERA_HEIGHT = 1.7f;
        constexpr float MAX_PITCH = 89.0f;
        constexpr float CAMERA_RESET_HEIGHT = 2.0f;
        constexpr float CAMERA_RESET_DISTANCE = 6.0f;
        
        // Movement
        constexpr float SPRINT_MULTIPLIER = 2.0f;
        constexpr float MAX_SLOPE_COSINE = 35.0f; // Degrees
		constexpr float JUMP_VELOCITY = 5.0f;

        // Physics
        constexpr float GRAVITY = 9.8f;
        constexpr float BOUNCE_RESTITUTION = 0.6f;
        constexpr float BOUNCE_FRICTION = 0.95f;
        constexpr float GROUND_Y = -0.5f;
        constexpr float PHYSICS_TIMESTEP = 1.0f / 120.0f;

        // Grid
        constexpr int GRID_SIZE = 20;
        constexpr float GRID_SPACING = 1.0f;

        // Projectiles
        constexpr float MIN_THROW_SPEED = 6.0f;
        constexpr float MAX_THROW_SPEED = 22.0f;
        constexpr float MAX_CHARGE_TIME = 1.0f;
        constexpr float PROJECTILE_RADIUS = 0.2f;
        constexpr float PROJECTILE_LIFETIME = 15.0f;
        constexpr float PROJECTILE_SPAWN_DISTANCE = 1.0f;
        constexpr float PROJECTILE_UPWARD_VELOCITY = 2.0f;

        // Lighting
        constexpr float DEFAULT_SHININESS = 32.0f;
        constexpr float POINT_LIGHT_INTENSITY = 2.0f;
        constexpr float ATTENUATION_CONSTANT = 1.0f;
        constexpr float ATTENUATION_LINEAR = 0.22f;
        constexpr float ATTENUATION_QUADRATIC = 0.20f;
        constexpr float FLASHLIGHT_INTENSITY = 1.0f;

        // Visual effects
        constexpr float TINT_AMPLITUDE = 0.3f;
        constexpr float TINT_FREQUENCY = 2.0f;
        constexpr float TRAJECTORY_POINT_SIZE = 6.0f;
        constexpr float TRAJECTORY_PREVIEW_POINT_SIZE = 7.0f;
        constexpr int TRAJECTORY_SIMULATION_FPS = 60;
        constexpr float WORLD_HALF_X = 12.0f;
        constexpr float WORLD_HALF_Z = 12.0f;

        // Pokeball animations
        constexpr float SHAKE_DURATION = 0.6f;
        constexpr int MAX_SHAKES = 3;
    }
}