#ifndef GUNTURRET_HPP
#define GUNTURRET_HPP
#include "Turret.hpp"

class GunTurret: public Turret {
public:
	static const int Price;
    GunTurret(float x, float y);
	void CreateBullet() override;
};
#endif // GUNTURRET_HPP
