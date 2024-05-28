#include <allegro5/base.h>
#include <cmath>
#include <string>
#include <iostream>

#include "Engine/AudioHelper.hpp"
#include "Bullet/FireBullet.hpp"
#include "Engine/Group.hpp"
#include "WizardTurret.hpp"
#include "Scene/PlayScene.hpp"
#include "Engine/Point.hpp"
#include "Enemy/Enemy.hpp"

const int WizardTurret::Price = 300;
const int WizardTurret::Radius = 150;
const float WizardTurret::Damage = 0.5;
WizardTurret::WizardTurret(float x, float y) :
	Turret("play/tower-base.png", "play/wizard.png", x, y, Radius, Price, 0.5) {
	Anchor.x = 0.5;
	Anchor.y = 0.5;
}
void WizardTurret::CreateBullet() {
	for (auto &it : getPlayScene()->EnemyGroup->GetObjects()) {
		Enemy* enemy = dynamic_cast<Enemy*>(it);
		if (!enemy->Visible) continue;
		if (inRadius(enemy->Position)) {
			enemy->Hit(Damage);
			enemy->WizardOnExplode();
		}
	}
}
bool WizardTurret::inRadius(Engine::Point a) {
	return ((a-Position).Magnitude() < Radius);
}
