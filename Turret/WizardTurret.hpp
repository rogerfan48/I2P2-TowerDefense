#ifndef WIZARDTURRET_HPP
#define WIZARDTURRET_HPP
#include "Turret.hpp"
#include "Engine/Point.hpp"

class WizardTurret: public Turret {
public:
	static const int Price;
	static const int Radius;
	static const float Damage;
    WizardTurret(float x, float y);
	void CreateBullet() override;
	bool inRadius(Engine::Point a);
};
#endif // WIZARDTURRET_HPP
