#ifndef GAME_COMPONENTS_H_
#define GAME_COMPONENTS_H_

namespace GAME
{
	///*** Tags ***///
	struct DoNotRender {}; // Tag to exclude entities from rendering
	struct Player {};      // Tag to identify player entity
	struct Enemy {};       // Tag to identify enemy entity
	struct Bullet {};      // Tag to identify bullet entity
	///*** Components ***///
	struct Transform {
		GW::MATH::GMATRIXF matrix; // Matrix representing the transform of the entity 
	};

	struct MeshCollection {
		std::string name;
		std::vector<entt::entity> meshEntities; // Collection of mesh entities 
	};
}// namespace GAME
#endif // !GAME_COMPONENTS_H_