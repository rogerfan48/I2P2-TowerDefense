#include <allegro5/base.h>
#include <cmath>
#include <string>

#include "Engine/AudioHelper.hpp"
#include "Bullet/FireBullet.hpp"
#include "Engine/Group.hpp"
#include "GunTurret.hpp"
#include "Scene/PlayScene.hpp"
#include "Engine/Point.hpp"

const int GunTurret::Price = 100;
GunTurret::GunTurret(float x, float y) :
	// TODO: [CUSTOM-TOOL] You can imitate the 2 files: 'GunTurret.hpp', 'GunTurret.cpp' to create a new turret.
	Turret("play/tower-base.png", "play/turret-6.png", x, y, 200, Price, 0.35) {
	// Move center downward, since we the turret head is slightly biased upward.
	Anchor.y += 8.0f / GetBitmapHeight();
}
void GunTurret::CreateBullet() {
	Engine::Point diff = Engine::Point(cos(Rotation - ALLEGRO_PI / 2), sin(Rotation - ALLEGRO_PI / 2));
	float rotation = atan2(diff.y, diff.x);
	Engine::Point normalized = diff.Normalize();
	// Change bullet position to the front of the gun barrel.
	getPlayScene()->BulletGroup->AddNewObject(new FireBullet(Position + normalized * 36, diff, rotation, this));
	AudioHelper::PlayAudio("gun.wav");
}
