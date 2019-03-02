#pragma once

#include <vector>

struct Point2d
{
	float x;
	float y;

	Point2d& operator+=(const Point2d& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this;
	}
};

const float FPS = 60.f;

const float HORIZONTAL_VELOCITY = 0.6f / FPS;
const float VERTICAL_ACCELERATION = 9.8f / FPS;
const float JUMP_ACCELERATION = VERTICAL_ACCELERATION * 2.f;

struct LevelDescription
{
    /// Total width of the level
    float width;
    /// Total height of the level
    float height;

    struct Pylon
	{
        /// Center is between [0, 0] and [width, height]
        /// Refers to the point in the middle of the gap between top and bottom part of the pylon
        Point2d center;
        /// Width of the pylon
        float width;
        /// The hight of the gap between top and bottom part of the pylon
        float gapHeight;
    };

    /// Each pylon in the level
    std::vector<Pylon> pylons;
};

struct Game
{
	float FPS;
	float HorizontalVelocity;
	float VerticalAcceleration;
	float JumpAcceleartion;

	LevelDescription Level;

	Game(float fps, float horizontalVelocity, float verticalVelocity, float jumpAcceleration, const LevelDescription& level)
		: FPS(fps)
		, HorizontalVelocity(horizontalVelocity)
		, VerticalAcceleration(verticalVelocity)
		, JumpAcceleartion(jumpAcceleration)
		, Level(level)
	{
	}
};

/// Expected number of decisions is (level.width / (HORIZONTAL_VELOCITY * FPS))
std::vector<bool> getAgentDecisions(const LevelDescription& level);
