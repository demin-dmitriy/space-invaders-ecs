#include "ecs.h"

#include <curses.h>

#include <string>
#include <thread>

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

struct Drawable final : ComponentI
{
	std::string m_sprite;
};

struct Weapon final : ComponentI
{
	int m_cooldown = 0;
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

struct Bullet final : ComponentI
{
	Vector m_velocity;
	int m_damage = 0;
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

		if (entities.size() == 0)
		{
			return;
		}

		Entity* const player_ship = entities.front();
		assert(player_ship->has_component<Action>());

		Action& action = player_ship->get_component<Action>();

		for (int ch; ERR != (ch = getch());)
		{
			if (ch == ' ')
			{
				action.m_fire = true;
			}
			else if (ch == KEY_LEFT)
			{
				action.m_direction = Direction::LEFT;
			}
			else if (ch == KEY_RIGHT)
			{
				action.m_direction = Direction::RIGHT;
			}
			else if (ch == KEY_UP)
			{
				action.m_direction = Direction::UP;
			}
			else if (ch == KEY_DOWN)
			{
				action.m_direction = Direction::DOWN;
			}
			else if (ch == 'q')
			{
				manager.create_entity()->add_component<GameOver>();
			}
		} 
	}

	void assign_enemy_action(EntityManager& manager)
	{

	}

	void execute_actions(EntityManager& manager)
	{

	}

	void fly_bullets(EntityManager& manager)
	{

	}

	void detect_collisions(EntityManager& manager)
	{

	}

	void deal_damage(EntityManager& manager)
	{

	}

	void recharge_weapons(EntityManager& manager)
	{

	}

	void cleanup_dead(EntityManager& manager)
	{

	}

	void draw(EntityManager& manager)
	{
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
		app::system::cleanup_dead(m_manager);
		app::system::draw(m_manager);
	}

	bool game_over()
	{
		return m_manager.filter<PlayerShip>().size() == 0
			or m_manager.filter<GameOver>().size() == 1;
	}

private:
	void setup_initial_position()
	{
		Entity* player_ship = m_manager.create_entity();

		player_ship->add_component<Position>();
		player_ship->add_component<Action>();
		player_ship->add_component<Ship>();
		player_ship->add_component<Weapon>();
		player_ship->add_component<AABB>();
		player_ship->add_component<Drawable>();
		player_ship->add_component<PlayerShip>();
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
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		printw("%d", i++);
	}
}
