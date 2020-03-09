#include "ecs.h"

#include <curses.h>

#include <string>
#include <thread>

constexpr int WINDOW_X_SIZE = 120;
constexpr int WINDOW_Y_SIZE = 40;

struct Vector final
{
	int m_x = 0;
	int m_y = 0;
};

Vector operator+(Vector a, Vector b)
{
	return Vector
	{
		.m_x = a.m_x + b.m_x,
		.m_y = a.m_y + b.m_y
	};
}

Vector operator*(int c, Vector a)
{
	return Vector
	{
		.m_x = a.m_x * c,
		.m_y = a.m_y * c
	};
}

struct Position final : ComponentI
{
	Vector m_position;
};

struct AABB final : ComponentI
{
	Vector m_bottom_left;
	Vector m_top_right;
};

const AABB WINDOW_AABB = []
	{
		AABB aabb;

		aabb.m_bottom_left = { .m_x = 0, .m_y = 0 };
		aabb.m_top_right = { .m_x = WINDOW_X_SIZE, .m_y = WINDOW_Y_SIZE };

		return aabb;
	}();

bool do_intersect(const Vector& a_position, const AABB& a_aabb, const Vector& b_position, const AABB& b_aabb)
{
	const Vector a_bottom_left = a_aabb.m_bottom_left + a_position;
	const Vector a_top_right = a_aabb.m_top_right + a_position;
	const Vector b_bottom_left = b_aabb.m_bottom_left + b_position;
	const Vector b_top_right = b_aabb.m_top_right + b_position;

	return b_top_right.m_x >= a_bottom_left.m_x
		and a_top_right.m_x >= b_bottom_left.m_x
		and b_top_right.m_y >= a_bottom_left.m_y
		and a_top_right.m_y >= b_bottom_left.m_y;
}

bool contains(const AABB& aabb, const Vector& point)
{
	return do_intersect(Vector{}, aabb, point, AABB{});
}

struct Drawable final : ComponentI
{
	char m_sprite;
};

struct Bullet final : ComponentI
{
	Vector m_velocity;
	int m_damage = 0;
};

struct Weapon final : ComponentI
{
	int m_cooldown = 0;
	int m_time_to_ready = 0;
	Vector m_relative_position;
	Bullet m_bullet_template;
};

enum class Direction
{
	NONE, LEFT, RIGHT, UP, DOWN
};

Vector direction_to_vector(Direction direction)
{
	switch (direction)
	{
		case Direction::NONE:  return Vector { .m_x =  0, .m_y =  0 };
		case Direction::LEFT:  return Vector { .m_x = -1, .m_y =  0 };
		case Direction::RIGHT: return Vector { .m_x = +1, .m_y =  0 };
		case Direction::UP:    return Vector { .m_x =  0, .m_y = +1 };
		case Direction::DOWN:  return Vector { .m_x =  0, .m_y = -1 };
		default: assert(false);
	}
}

struct Action final : ComponentI
{
	Direction m_direction = Direction::NONE;
	bool m_fire = false;
};

struct Ship final : ComponentI
{
	int m_health = 0;
	int m_speed = 0;
};

struct EnemyShip final : ComponentI
{
};

struct PlayerShip final : ComponentI
{
};

struct Collision final : ComponentI
{
	Entity* m_a = nullptr;
	Entity* m_b = nullptr;
};

struct Dead final : ComponentI
{
};

struct GameOver final : ComponentI
{
};

namespace app::system
{
	void process_input(EntityManager& manager)
	{
		const auto entities = manager.filter<PlayerShip>();

		Action* action = nullptr;

		if (entities.size() > 0)
		{
			Entity* const player_ship = entities.front();
			assert(player_ship->has_component<Action>());

			action = &player_ship->get_component<Action>();
		}

		for (int ch; ERR != (ch = getch());)
		{
			if (ch == 'q')
			{
				manager.create_entity()->add_component<GameOver>();
			}
			else if (action)
			{
				if (ch == ' ')
				{
					action->m_fire = true;
				}
				else if (ch == KEY_LEFT)
				{
					action->m_direction = Direction::LEFT;
				}
				else if (ch == KEY_RIGHT)
				{
					action->m_direction = Direction::RIGHT;
				}
				else if (ch == KEY_UP)
				{
					action->m_direction = Direction::UP;
				}
				else if (ch == KEY_DOWN)
				{
					action->m_direction = Direction::DOWN;
				}
			}
		} 
	}

	void assign_enemy_action(EntityManager& manager)
	{

	}

	void execute_actions(EntityManager& manager)
	{
		for (Entity* entity : manager.filter<Action>())
		{
			Action& action = entity->get_component<Action>();

			Ship* ship = entity->try_get_component<Ship>();
			Position* position = entity->try_get_component<Position>();

			if (ship and position)
			{
				position->m_position = position->m_position + ship->m_speed * direction_to_vector(action.m_direction);
			}

			if (action.m_fire and position)
			{
				if (Weapon* weapon = entity->try_get_component<Weapon>())
				{
					if (weapon->m_time_to_ready == 0)
					{
						Entity* bullet_entity = manager.create_entity();
						bullet_entity->add_component<Position>().m_position = position->m_position + weapon->m_relative_position;
						bullet_entity->add_component<AABB>();
						bullet_entity->add_component<Drawable>().m_sprite = '|';
						bullet_entity->add_component<Bullet>(weapon->m_bullet_template);
					}
				}
			}

			action.m_direction = Direction::NONE;
			action.m_fire = false;
		}
	}

	void fly_bullets(EntityManager& manager)
	{
		for (Entity* entity : manager.filter<Bullet>())
		{
			Bullet& bullet = entity->get_component<Bullet>();
			assert(entity->has_component<Position>());
			Position& position = entity->get_component<Position>();

			position.m_position = position.m_position + bullet.m_velocity;
		}
	}

	void detect_collisions(EntityManager& manager)
	{
		const auto entities = manager.filter<AABB>();

		for (size_t i = 0; i < entities.size(); ++i)
		{
			for (size_t j = i + 1; j < entities.size(); ++j)
			{
				const AABB& a_aabb = entities[i]->get_component<AABB>();
				const Position& a_position = entities[i]->get_component<Position>();
				const AABB& b_aabb = entities[j]->get_component<AABB>();
				const Position& b_position = entities[j]->get_component<Position>();

				if (do_intersect(a_position.m_position, a_aabb, b_position.m_position, b_aabb))
				{
					Entity* collision_entity = manager.create_entity();
					Collision& collision = collision_entity->add_component<Collision>();
					collision.m_a = entities[i];
					collision.m_b = entities[j];
				}
			}
		}
	}

	void deal_damage(EntityManager& manager)
	{

	}

	void recharge_weapons(EntityManager& manager)
	{
		for (Entity* entity : manager.filter<Weapon>())
		{
			Weapon& weapon = entity->get_component<Weapon>();

			if (weapon.m_time_to_ready > 0)
			{
				weapon.m_time_to_ready -= 1;
			}
		}
	}

	void kill_oob_entities(EntityManager& manager)
	{
		for (Entity* entity : manager.filter<Position>())
		{
			if (not contains(WINDOW_AABB, entity->get_component<Position>().m_position))
			{
				manager.destroy_entity(entity);
			}
		}
	}

	void cleanup_dead(EntityManager& manager)
	{
		for (Entity* entity : manager.filter<Dead>())
		{
			manager.destroy_entity(entity);
		}
	}

	void cleanup_collisions(EntityManager& manager)
	{
		for (Entity* entity : manager.filter<Collision>())
		{
			manager.destroy_entity(entity);
		}
	}

	void draw(EntityManager& manager)
	{
		erase();

		for (Entity* entity : manager.filter<Drawable>())
		{
			Drawable& drawable = entity->get_component<Drawable>();
			Position* position = entity->try_get_component<Position>();

			if (position and contains(WINDOW_AABB, position->m_position))
			{
				mvaddch(WINDOW_Y_SIZE - position->m_position.m_y, position->m_position.m_x, drawable.m_sprite);
			}
		}

		refresh();
	}
};

struct App final
{
	App()
	{
		initscr();
		raw();
		keypad(stdscr, true);
		noecho();
		nodelay(stdscr, true);
		curs_set(0);

		setup_initial_position();
	}

	~App()
	{
		endwin();
	}

	void run_step()
	{
		app::system::process_input(m_manager);
		app::system::assign_enemy_action(m_manager);
		app::system::execute_actions(m_manager);
		app::system::fly_bullets(m_manager);
		app::system::detect_collisions(m_manager);
		app::system::deal_damage(m_manager);
		app::system::recharge_weapons(m_manager);
		app::system::kill_oob_entities(m_manager);
		app::system::cleanup_dead(m_manager);
		app::system::cleanup_collisions(m_manager);
		app::system::draw(m_manager);
	}

	bool game_over()
	{
		return m_manager.filter<PlayerShip>().size() == 0
			or m_manager.filter<GameOver>().size() == 1;
	}

private:
	static constexpr Vector PLAYER_START_POSITION { .m_x = 60, .m_y = 2 };
	inline static const std::vector<Vector> ENEMY_START_POSITIONS
	{
		{ .m_x = 20, .m_y = 38 },
		{ .m_x = 30, .m_y = 38 },
		{ .m_x = 40, .m_y = 38 },
		{ .m_x = 50, .m_y = 38 },
		{ .m_x = 60, .m_y = 38 },
		{ .m_x = 70, .m_y = 38 },
		{ .m_x = 80, .m_y = 38 },
		{ .m_x = 90, .m_y = 38 },
		{ .m_x = 100, .m_y = 38 },
	};

	void setup_player_ship()
	{
		Entity* const player_ship = m_manager.create_entity();

		Position& position = player_ship->add_component<Position>();
		position.m_position = PLAYER_START_POSITION;

		Action& action = player_ship->add_component<Action>();
		Ship& ship = player_ship->add_component<Ship>();
		ship.m_health = 100;
		ship.m_speed = 1;

		Weapon& weapon = player_ship->add_component<Weapon>();
		weapon.m_cooldown = 5;
		weapon.m_relative_position = Vector { .m_x = 0, .m_y = 1 };
		weapon.m_bullet_template.m_velocity = Vector { .m_x = 0, .m_y = 1 };
		weapon.m_bullet_template.m_damage = 10;

		AABB& aabb = player_ship->add_component<AABB>();
		Drawable& drawable = player_ship->add_component<Drawable>();
		drawable.m_sprite = 'X';

		player_ship->add_component<PlayerShip>();
	}

	void setup_enemy_ships()
	{
		for (const Vector& start_position : ENEMY_START_POSITIONS)
		{
			Entity* const enemy_ship = m_manager.create_entity();

			Position& position = enemy_ship->add_component<Position>();
			position.m_position = start_position;

			Action& action = enemy_ship->add_component<Action>();
			Ship& ship = enemy_ship->add_component<Ship>();
			ship.m_health = 10;
			ship.m_speed = 1;

			Weapon& weapon = enemy_ship->add_component<Weapon>();
			weapon.m_cooldown = 5;
			weapon.m_relative_position = Vector { .m_x = 0, .m_y = -1 };
			weapon.m_bullet_template.m_velocity = Vector { .m_x = 0, .m_y = -1 };
			weapon.m_bullet_template.m_damage = 10;

			AABB& aabb = enemy_ship->add_component<AABB>();
			Drawable& drawable = enemy_ship->add_component<Drawable>();
			drawable.m_sprite = 'o';

			enemy_ship->add_component<EnemyShip>();
		}
	}

	void setup_initial_position()
	{
		setup_player_ship();
		setup_enemy_ships();
	}

	EntityManager m_manager;
};

int main()
{
	App app;

	int i = 0;

	while (not app.game_over())
	{
		app.run_step();
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		// TODO: calculate fps
	}
}
