#include <allegro5/base.h>
#include <cmath>
#include <string>

#include "Engine/AudioHelper.hpp"
#include "Bullet/FireBullet.hpp"
#include "Engine/Group.hpp"
#include "Shovel.hpp"
#include "Scene/PlayScene.hpp"
#include "Engine/Point.hpp"

const int Shovel::Price = 0;
Shovel::Shovel(float x, float y) :
	// TODO: [CUSTOM-TOOL] You can imitate the 2 files: 'Shovel.hpp', 'Shovel.cpp' to create a new turret.
	Turret("play/shovel.png", "play/shovel.png", x, y, 0, Price, 100000) {}
void Shovel::CreateBullet() {
}
