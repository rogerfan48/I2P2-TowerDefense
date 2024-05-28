#ifndef BOMB_HPP
#define BOMB_HPP
#include "Turret.hpp"

class Bomb: public Turret {
public:
	static const int Price;
	static const int Damage;
    Bomb(float x, float y);
	void CreateBullet() override;
};
#endif // BOMB_HPP
