#include "Utilities.h"
#include "../CCL.h"
namespace UTIL
{
	GW::MATH::GVECTORF GetRandomVelocityVector()
	{
		GW::MATH::GVECTORF vel = {float((rand() % 20) - 10), 0.0f, float((rand() % 20) - 10)};
		if (vel.x <= 0.0f && vel.x > -1.0f)
			vel.x = -1.0f;
		else if (vel.x >= 0.0f && vel.x < 1.0f)
			vel.x = 1.0f;

		if (vel.z <= 0.0f && vel.z > -1.0f)
			vel.z = -1.0f;
		else if (vel.z >= 0.0f && vel.z < 1.0f)
			vel.z = 1.0f;

		GW::MATH::GVector::NormalizeF(vel, vel);

		return vel;
	}

    GW::MATH::GVECTORF GetRandomDiagonalDirection() {
        GW::MATH::GVECTORF direction;

        // Generate random x and z components that are not too small
        direction.x = (rand() % 2 == 0) ? -1.0f : 1.0f;
        direction.z = (rand() % 2 == 0) ? -1.0f : 1.0f;

        // Add some randomness to the direction
        direction.x += (static_cast<float>(rand()) / RAND_MAX * 0.5f - 0.25f);
        direction.z += (static_cast<float>(rand()) / RAND_MAX * 0.5f - 0.25f);

        // Normalize the direction
        float length = std::sqrt(direction.x * direction.x + direction.z * direction.z);
        direction.x /= length;
        direction.y = 0.0f; // Keep movement on the XZ plane
        direction.z /= length;

        return direction;
    }
} // namespace UTIL