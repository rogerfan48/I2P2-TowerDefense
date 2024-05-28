#include <allegro5/base.h>
#include <cmath>
#include <string>

#include "Engine/AudioHelper.hpp"
#include "Bullet/FireBullet.hpp"
#include "Engine/Group.hpp"
#include "Bomb.hpp"
#include "Scene/PlayScene.hpp"
#include "Engine/Point.hpp"
#include "Enemy/Enemy.hpp"

const int Bomb::Price = 30;
const int Bomb::Damage = 2;
Bomb::Bomb(float x, float y) :
	// TODO: [CUSTOM-TOOL] You can imitate the 2 files: 'Bomb.hpp', 'Bomb.cpp' to create a new turret.
	Turret("play/bomb.png", "play/bomb.png", x, y, 0, Price, 100000) {}
void Bomb::CreateBullet() {
}