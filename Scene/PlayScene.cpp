#include <allegro5/allegro.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <functional>
#include <vector>
#include <queue>
#include <string>
#include <memory>

#include "Engine/AudioHelper.hpp"
#include "UI/Animation/DirtyEffect.hpp"
#include "Enemy/Enemy.hpp"
#include "Engine/GameEngine.hpp"
#include "Engine/Group.hpp"
#include "UI/Component/Label.hpp"
#include "Turret/LaserTurret.hpp"
#include "Turret/MachineGunTurret.hpp"
#include "Turret/MissileTurret.hpp"
#include "Turret/WizardTurret.hpp"
#include "Turret/GunTurret.hpp"
#include "Turret/Shovel.hpp"
#include "Turret/Bomb.hpp"
#include "UI/Animation/Plane.hpp"
#include "Enemy/PlaneEnemy.hpp"
#include "PlayScene.hpp"
#include "Engine/Resources.hpp"
#include "Enemy/SoldierEnemy.hpp"
#include "Enemy/TankEnemy.hpp"
#include "Turret/TurretButton.hpp"
#include "Scene/ScoreboardScene.hpp"
#include "Enemy/Tk1Enemy.hpp"
#include "Enemy/Tk2Enemy.hpp"

bool PlayScene::DebugMode = false;
const std::vector<Engine::Point> PlayScene::directions = { Engine::Point(-1, 0), Engine::Point(0, -1), Engine::Point(1, 0), Engine::Point(0, 1) };
const int PlayScene::MapWidth = 20, PlayScene::MapHeight = 13;
const int PlayScene::BlockSize = 64;
const float PlayScene::DangerTime = 7.61;
const Engine::Point PlayScene::SpawnGridPoint = Engine::Point(-1, 0);
const Engine::Point PlayScene::EndGridPoint = Engine::Point(MapWidth, MapHeight - 1);
const std::vector<int> PlayScene::code = { ALLEGRO_KEY_UP, ALLEGRO_KEY_DOWN, ALLEGRO_KEY_LEFT, ALLEGRO_KEY_RIGHT, ALLEGRO_KEY_ENTER };
// const std::vector<int> PlayScene::code = { ALLEGRO_KEY_UP, ALLEGRO_KEY_UP, ALLEGRO_KEY_DOWN, ALLEGRO_KEY_DOWN,
// 									ALLEGRO_KEY_LEFT, ALLEGRO_KEY_LEFT, ALLEGRO_KEY_RIGHT, ALLEGRO_KEY_RIGHT,
// 									ALLEGRO_KEY_B, ALLEGRO_KEY_A, ALLEGRO_KEYMOD_SHIFT, ALLEGRO_KEY_ENTER };
Engine::Point PlayScene::GetClientSize() { return Engine::Point(MapWidth * BlockSize, MapHeight * BlockSize); }
void PlayScene::Initialize() {
	mapState.clear();
	keyStrokes.clear();
	ticks = 0;
	deathCountDown = -1;
	lives = 10;
	money = 150;
	SpeedMult = 1;
	newRecord.reset();	// ?
	// Add groups from bottom to top.
	AddNewObject(TileMapGroup = new Group());
	AddNewObject(GroundEffectGroup = new Group());
	AddNewObject(DebugIndicatorGroup = new Group());
	AddNewObject(TowerGroup = new Group());
	AddNewObject(EnemyGroup = new Group());
	AddNewObject(BulletGroup = new Group());
	AddNewObject(EffectGroup = new Group());
	// Should support buttons.
	AddNewControlObject(UIGroup = new Group());
	ReadMap();
	ReadEnemyWave();
	mapDistance = CalculateBFSDistance();
	ConstructUI();
	imgTarget = new Engine::Image("play/target.png", 0, 0);
	imgTarget->Visible = false;
	preview = nullptr;
	UIGroup->AddNewObject(imgTarget);
	// Preload Lose Scene
	deathBGMInstance = Engine::Resources::GetInstance().GetSampleInstance("astronomia.ogg");
	Engine::Resources::GetInstance().GetBitmap("lose/benjamin-happy.png");
	// Start BGM.
	bgmId = AudioHelper::PlayBGM("play.ogg");
}
void PlayScene::Terminate() {
	AudioHelper::StopBGM(bgmId);
	AudioHelper::StopSample(deathBGMInstance);
	deathBGMInstance = std::shared_ptr<ALLEGRO_SAMPLE_INSTANCE>();
	IScene::Terminate();
}
void PlayScene::Update(float deltaTime) {
	// If we use deltaTime directly, then we might have Bullet-through-paper problem.
	// Reference: Bullet-Through-Paper
	if (SpeedMult == 0) deathCountDown = -1;
	else if (deathCountDown != -1) SpeedMult = 1;
	// Calculate danger zone.
	std::vector<float> reachEndTimes;
	for (auto& it : EnemyGroup->GetObjects()) {
		reachEndTimes.push_back(dynamic_cast<Enemy*>(it)->reachEndTime);
	}
	// Can use Heap / Priority-Queue instead. But since we won't have too many enemies, sorting is fast enough.
	std::sort(reachEndTimes.begin(), reachEndTimes.end());
	float newDeathCountDown = -1;
	int danger = lives;
	for (auto& it : reachEndTimes) {
		if (it <= DangerTime) {
			danger--;
			if (danger <= 0) { 					// Death Countdown
				float pos = DangerTime - it;
				if (it > deathCountDown) { 		// Restart Death Count Down BGM.
					AudioHelper::StopSample(deathBGMInstance);
					if (SpeedMult != 0) deathBGMInstance = AudioHelper::PlaySample("astronomia.ogg", false, AudioHelper::BGMVolume, pos);
				}
				float alpha = pos / DangerTime;
				alpha = std::max(0, std::min(255, static_cast<int>(alpha * alpha * 255)));
				dangerIndicator->Tint = al_map_rgba(255, 255, 255, alpha);
				newDeathCountDown = it;
				break;
			}
		}
	}
	deathCountDown = newDeathCountDown;
	if (SpeedMult == 0) AudioHelper::StopSample(deathBGMInstance);
	if (deathCountDown == -1 && lives > 0) {
		AudioHelper::StopSample(deathBGMInstance);
		dangerIndicator->Tint.a = 0;
	}
	if (SpeedMult == 0) deathCountDown = -1;
	for (int i = 0; i < SpeedMult; i++) {
		IScene::Update(deltaTime);
		// Check if we should create new enemy.
		ticks += deltaTime;
		if (enemyWaveData.empty()) {
			if (EnemyGroup->GetObjects().empty()) {
				// Free resources.
				// delete TileMapGroup;
				// delete GroundEffectGroup;
				// delete DebugIndicatorGroup;
				// delete TowerGroup;
				// delete EnemyGroup;
				// delete BulletGroup;
				// delete EffectGroup;
				// delete UIGroup;
				// delete imgTarget;
				Engine::GameEngine::GetInstance().ChangeScene("win");
			}
			continue;
		}
		auto current = enemyWaveData.front();
		if (ticks < current.second) continue;
		ticks -= current.second;
		enemyWaveData.pop_front();
		const Engine::Point SpawnCoordinate = Engine::Point(SpawnGridPoint.x * BlockSize + BlockSize / 2, SpawnGridPoint.y * BlockSize + BlockSize / 2);
		Enemy* enemy;
		switch (current.first) {
		case 1:
			EnemyGroup->AddNewObject(enemy = new SoldierEnemy(SpawnCoordinate.x, SpawnCoordinate.y));
			break;
		case 2:
			EnemyGroup->AddNewObject(enemy = new PlaneEnemy(SpawnCoordinate.x, SpawnCoordinate.y));
			break;
		case 3:
			EnemyGroup->AddNewObject(enemy = new TankEnemy(SpawnCoordinate.x, SpawnCoordinate.y));
			break;
        // TODO: [CUSTOM-ENEMY]: You need to modify 'Resource/enemy1.txt', or 'Resource/enemy2.txt' to spawn the 4th enemy.
        //         The format is "[EnemyId] [TimeDelay] [Repeat]".
        // TODO: [CUSTOM-ENEMY]: Enable the creation of the enemy.
		case 4:
			EnemyGroup->AddNewObject(enemy = new Tk1Enemy(SpawnCoordinate.x, SpawnCoordinate.y));
			break;
		case 5:
			EnemyGroup->AddNewObject(enemy = new Tk2Enemy(SpawnCoordinate.x, SpawnCoordinate.y));
			break;
		default:
			continue;
		}
		enemy->UpdatePath(mapDistance);
		// Compensate the time lost.
		enemy->Update(ticks);
	}
	if (preview) {
		preview->Position = Engine::GameEngine::GetInstance().GetMousePosition();
		// To keep responding when paused.
		preview->Update(deltaTime);
	}
}
void PlayScene::Draw() const {
	IScene::Draw();
	if (DebugMode) {
		// Draw reverse BFS distance on all reachable blocks.
		for (int i = 0; i < MapHeight; i++) {
			for (int j = 0; j < MapWidth; j++) {
				if (mapDistance[i][j] != -1) {
					// Not elegant nor efficient, but it's quite enough for debugging.
					Engine::Label label(std::to_string(mapDistance[i][j]), "pirulen.ttf", 32, (j + 0.5) * BlockSize, (i + 0.5) * BlockSize);
					label.Anchor = Engine::Point(0.5, 0.5);
					label.Draw();
				}
			}
		}
	}
}
void PlayScene::OnMouseDown(int button, int mx, int my) {
	if ((button & 1) && !imgTarget->Visible && preview) {
		// Cancel turret construct.
		UIGroup->RemoveObject(preview->GetObjectIterator());
		preview = nullptr;
	}
	IScene::OnMouseDown(button, mx, my);
}
void PlayScene::OnMouseMove(int mx, int my) {
	IScene::OnMouseMove(mx, my);
	const int x = mx / BlockSize;
	const int y = my / BlockSize;
	if (!preview || x < 0 || x >= MapWidth || y < 0 || y >= MapHeight) {
		imgTarget->Visible = false;
		return;
	}
	imgTarget->Visible = true;
	imgTarget->Position.x = x * BlockSize;
	imgTarget->Position.y = y * BlockSize;
}
void PlayScene::OnMouseUp(int button, int mx, int my) {
	IScene::OnMouseUp(button, mx, my);
	if (!imgTarget->Visible)
		return;
	const int x = mx / BlockSize;
	const int y = my / BlockSize;
	if (button & 1) {
		if (mapState[y][x] != TILE_OCCUPIED) {
			if (preview->GetPrice() == 0) return;
			if (!preview)
				return;
			// Check if valid.
			if (!CheckSpaceValid(x, y)) {
				Engine::Sprite* sprite;
				GroundEffectGroup->AddNewObject(sprite = new DirtyEffect("play/target-invalid.png", 1, x * BlockSize + BlockSize / 2, y * BlockSize + BlockSize / 2));
				sprite->Rotation = 0;
				return;
			}
			// Purchase.
			EarnMoney(-preview->GetPrice());
			// Remove Preview.
			preview->GetObjectIterator()->first = false;
			UIGroup->RemoveObject(preview->GetObjectIterator());
			// Construct real turret.
			preview->Position.x = x * BlockSize + BlockSize / 2;
			preview->Position.y = y * BlockSize + BlockSize / 2;
			preview->Enabled = true;
			preview->Preview = false;
			preview->Tint = al_map_rgba(255, 255, 255, 255);
			TowerGroup->AddNewObject(preview);
			// To keep responding when paused.
			preview->Update(0);
			// Remove Preview.
			preview = nullptr;

			mapState[y][x] = TILE_OCCUPIED;
			OnMouseMove(mx, my);
		} else if (preview && preview->GetPrice() == 0) {
			preview->Position.x = x * BlockSize + BlockSize / 2;
			preview->Position.y = y * BlockSize + BlockSize / 2;
			for (auto &it : TowerGroup->GetObjects()) {
				Turret* itt = dynamic_cast<Turret*>(it);
				if (itt->Position == preview->Position) {
					EarnMoney(static_cast<int>(0.75*itt->GetPrice()));
					TowerGroup->RemoveObject(itt->GetObjectIterator());
					break;
				}
			}
			UIGroup->RemoveObject(preview->GetObjectIterator());
			preview = nullptr;
		}
	}
}
void PlayScene::OnKeyDown(int keyCode) {
	IScene::OnKeyDown(keyCode);
	if (keyCode == ALLEGRO_KEY_TAB) {
		DebugMode = !DebugMode;
	}
	else {
		keyStrokes.push_back(keyCode);
		if (keyStrokes.size() > code.size())
			keyStrokes.pop_front();
		if (keyCode == ALLEGRO_KEY_ENTER && keyStrokes.size() == code.size()) {
			auto it = keyStrokes.begin();
			for (int c : code) {
				if (!(
					(*it == c) ||
					(c == ALLEGRO_KEYMOD_SHIFT && (*it == ALLEGRO_KEY_LSHIFT || *it == ALLEGRO_KEY_RSHIFT) )
				))
					return;
				++it;
			}
			EffectGroup->AddNewObject(new Plane());
			EarnMoney(10000);
		}
	}
	if (keyCode == ALLEGRO_KEY_Q) UIBtnClicked(0); 			// ? Hotkey for MachineGunTurret.
	else if (keyCode == ALLEGRO_KEY_W) UIBtnClicked(1);		// ? Hotkey for LaserTurret.
	else if (keyCode == ALLEGRO_KEY_E) UIBtnClicked(2);		// ? Hotkey for MissileTurret.
	else if (keyCode == ALLEGRO_KEY_R) UIBtnClicked(3);	// TODO: [CUSTOM-TURRET]: Make specific key to create the turret.
	else if (keyCode == ALLEGRO_KEY_A) UIBtnClicked(4);
	else if (keyCode == ALLEGRO_KEY_S) UIBtnClicked(5);
	else if (keyCode == ALLEGRO_KEY_D) BombDamage();
	else if (keyCode >= ALLEGRO_KEY_0 && keyCode <= ALLEGRO_KEY_9) {
		// Hotkey for Speed up.
		SpeedMult = keyCode - ALLEGRO_KEY_0;
	}
}
void PlayScene::Hit() {
	lives--;
	UILives->Text = std::string("Life ") + std::to_string(lives);
	if (lives <= 0) {
		Engine::GameEngine::GetInstance().ChangeScene("lose");
	}
}
int PlayScene::GetMoney() const {
	return money;
}
void PlayScene::EarnMoney(int money) {
	this->money += money;
	UIMoney->Text = std::string("$") + std::to_string(this->money);
	UIScore->Text = std::string("Score: ") + std::to_string(newRecord.score);
}
void PlayScene::ReadMap() {
	std::string filename = std::string("Resource/map") + std::to_string(MapId) + ".txt";
	char c;
	std::vector<bool> mapData;
	std::ifstream fin(filename);
	while (fin >> c) {
		switch (c) {
		case '0': mapData.push_back(false); break;
		case '1': mapData.push_back(true); break;
		case '\n':
		case '\r':
			if (static_cast<int>(mapData.size()) / MapWidth == 0) break;
		default: throw std::ios_base::failure("Map data is corrupted.");
		}
	}
	fin.close();
	// Validate map data.
	if (static_cast<int>(mapData.size()) != MapWidth * MapHeight)
		throw std::ios_base::failure("Map data is corrupted.");
	// Store map in 2d array.
	mapState = std::vector<std::vector<TileType>>(MapHeight, std::vector<TileType>(MapWidth));
	for (int i=0; i<MapHeight; i++) {
		for (int j=0; j<MapWidth; j++) {
			const int num = mapData[i * MapWidth + j];
			mapState[i][j] = num ? TILE_FLOOR : TILE_DIRT;
			if (num) TileMapGroup->AddNewObject(new Engine::Image("play/floor.png", j * BlockSize, i * BlockSize, BlockSize, BlockSize));
			else TileMapGroup->AddNewObject(new Engine::Image("play/dirt.png", j * BlockSize, i * BlockSize, BlockSize, BlockSize));
		}
	}
}
void PlayScene::ReadEnemyWave() {
    std::string filename = std::string("Resource/enemy") + std::to_string(MapId) + ".txt";
	float type, wait, repeat;
	enemyWaveData.clear();
	std::ifstream fin(filename);
	while (fin >> type && fin >> wait && fin >> repeat) {
		for (int i = 0; i < repeat; i++)
			enemyWaveData.emplace_back(type, wait);
	}
	fin.close();
}
void PlayScene::ConstructUI() {
	// Background
	UIGroup->AddNewObject(new Engine::Image("play/sand.png", 1280, 0, 320, 832));
	// Text
	UIGroup->AddNewObject(new Engine::Label(std::string("Stage ") + std::to_string(MapId), "pirulen.ttf", 40, 1294, 0));
	UIGroup->AddNewObject(UIScore = new Engine::Label(std::string("Score: ") + std::to_string(newRecord.score), "pirulen.ttf", 32, 1294, 48));
	UIGroup->AddNewObject(UILives = new Engine::Label(std::string("Life: ") + std::to_string(lives), "pirulen.ttf", 32, 1294, 88));
	UIGroup->AddNewObject(UIMoney = new Engine::Label(std::string("$") + std::to_string(money), "pirulen.ttf", 32, 1294, 128));
	TurretButton* btn;
	// Button 1
	btn = new TurretButton("play/floor.png", "play/dirt.png",
		Engine::Sprite("play/tower-base.png", 1294, 176, 0, 0, 0, 0),
		Engine::Sprite("play/turret-1.png", 1294, 176 - 8, 0, 0, 0, 0)
		, 1294, 176, MachineGunTurret::Price);
	// Reference: Class Member Function Pointer and std::bind.
	btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 0));
	UIGroup->AddNewControlObject(btn);
	UIGroup->AddNewObject(new Engine::Label("$" + std::to_string(MachineGunTurret::Price), "pirulen.ttf", 20, 1294 + 4, 176 + 62, 100, 100, 100));
	UIGroup->AddNewObject(new Engine::Label("Q", "pirulen.ttf", 24, 1294+2, 176+2, 255, 0, 0, 255, 0.5, 0.5));
	// Button 2
	btn = new TurretButton("play/floor.png", "play/dirt.png",
		Engine::Sprite("play/tower-base.png", 1370, 176, 0, 0, 0, 0),
		Engine::Sprite("play/turret-2.png", 1370, 176 - 8, 0, 0, 0, 0)
		, 1370, 176, LaserTurret::Price);
	btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 1));
	UIGroup->AddNewControlObject(btn);
	UIGroup->AddNewObject(new Engine::Label("$" + std::to_string(LaserTurret::Price), "pirulen.ttf", 20, 1370 - 4, 176 + 62, 100, 100, 100));
	UIGroup->AddNewObject(new Engine::Label("W", "pirulen.ttf", 24, 1370+2, 176+2, 255, 0, 0, 255, 0.5, 0.5));
	// Button 3
	btn = new TurretButton("play/floor.png", "play/dirt.png",
		Engine::Sprite("play/tower-base.png", 1446, 176, 0, 0, 0, 0),
		Engine::Sprite("play/turret-3.png", 1446, 176, 0, 0, 0, 0)
		, 1446, 176, MissileTurret::Price);
	btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 2));
	UIGroup->AddNewControlObject(btn);
	UIGroup->AddNewObject(new Engine::Label("$" + std::to_string(MissileTurret::Price), "pirulen.ttf", 20, 1446 - 4, 176 + 62, 100, 100, 100));
	UIGroup->AddNewObject(new Engine::Label("E", "pirulen.ttf", 24, 1446+2, 176+2, 255, 0, 0, 255, 0.5, 0.5));
	// TODO: [CUSTOM-TURRET]: Create a button to support constructing the turret.
	btn = new TurretButton("play/floor.png", "play/dirt.png",
		Engine::Sprite("play/tower-base.png", 1522, 176, 0, 0, 0, 0),
		Engine::Sprite("play/wizard.png", 1522, 176, 0, 0, 0, 0)
		, 1522, 176, WizardTurret::Price);
	btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 3));
	UIGroup->AddNewControlObject(btn);
	UIGroup->AddNewObject(new Engine::Label("$" + std::to_string(WizardTurret::Price), "pirulen.ttf", 20, 1522 - 4, 176 + 62, 100, 100, 100));
	UIGroup->AddNewObject(new Engine::Label("R", "pirulen.ttf", 24, 1522+2, 176+2, 255, 0, 0, 255, 0.5, 0.5));

	btn = new TurretButton("play/floor.png", "play/dirt.png",
		Engine::Sprite("play/tower-base.png", 1294, 270, 0, 0, 0, 0),
		Engine::Sprite("play/turret-6.png", 1294, 270, 0, 0, 0, 0)
		, 1294, 270, GunTurret::Price);
	btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 4));
	UIGroup->AddNewControlObject(btn);
	UIGroup->AddNewObject(new Engine::Label("$" + std::to_string(GunTurret::Price), "pirulen.ttf", 20, 1294 + 2, 270 + 62, 100, 100, 100));
	UIGroup->AddNewObject(new Engine::Label("A", "pirulen.ttf", 24, 1294+2, 270+2, 255, 0, 0, 255, 0.5, 0.5));

	btn = new TurretButton("play/floor.png", "play/dirt.png",
		Engine::Sprite("play/shovel.png", 1370, 270, 0, 0, 0, 0),
		Engine::Sprite("play/shovel.png", 1370, 270, 0, 0, 0, 0)
		, 1370, 270, Shovel::Price);
	btn->SetOnClickCallback(std::bind(&PlayScene::UIBtnClicked, this, 5));
	UIGroup->AddNewControlObject(btn);
	UIGroup->AddNewObject(new Engine::Label("$" + std::to_string(Shovel::Price), "pirulen.ttf", 20, 1370 + 15, 270 + 62, 100, 100, 100));
	UIGroup->AddNewObject(new Engine::Label("S", "pirulen.ttf", 24, 1370+2, 270+2, 255, 0, 0, 255, 0.5, 0.5));

	btn = new TurretButton("play/floor.png", "play/dirt.png",
		Engine::Sprite("play/Bomb.png", 1446, 270, 0, 0, 0, 0),
		Engine::Sprite("play/Bomb.png", 1446, 270, 0, 0, 0, 0)
		, 1446, 270, Bomb::Price);
	btn->SetOnClickCallback(std::bind(&PlayScene::BombDamage, this));
	UIGroup->AddNewControlObject(btn);
	UIGroup->AddNewObject(new Engine::Label("$" + std::to_string(Bomb::Price), "pirulen.ttf", 20, 1446 + 15, 270 + 62, 100, 100, 100));
	UIGroup->AddNewObject(new Engine::Label("D", "pirulen.ttf", 24, 1446+2, 270+2, 255, 0, 0, 255, 0.5, 0.5));

	UIGroup->AddNewObject(new Engine::Image("win/black.png", 1294-2.5, 620-2.5, 280+5, 60+5));
	Engine::ImageButton* btnn = new Engine::ImageButton("win/dirt.png", "win/floor.png", 1294, 620, 280, 60);
	btnn->SetOnClickCallback(std::bind(&PlayScene::Surrender, this));
	UIGroup->AddNewControlObject(btnn);
	UIGroup->AddNewObject(new Engine::Label("Surrender", "pirulen.ttf", 28, 1294 + 140, 620 + 30, 0, 0, 0, 255, 0.5, 0.5));

	int w = Engine::GameEngine::GetInstance().GetScreenSize().x;
	int h = Engine::GameEngine::GetInstance().GetScreenSize().y;
	int shift = 135 + 25;
	dangerIndicator = new Engine::Sprite("play/benjamin.png", w - shift, h - shift);
	dangerIndicator->Tint.a = 0;
	UIGroup->AddNewObject(dangerIndicator);

	UIGroup->AddNewObject(new Engine::Label("Tips:", "pirulen.ttf", 28, 1294, 700, 100, 100, 100));
	UIGroup->AddNewObject(new Engine::Label("[TAB]", "pirulen.ttf", 28, 1294, 730, 100, 100, 100));
	UIGroup->AddNewObject(new Engine::Label("Debug", "pirulen.ttf", 28, 1394, 730, 100, 100, 100));
	UIGroup->AddNewObject(new Engine::Label("[1-9]", "pirulen.ttf", 28, 1294, 760, 100, 100, 100));
	UIGroup->AddNewObject(new Engine::Label("Speed", "pirulen.ttf", 28, 1394, 760, 100, 100, 100));
	UIGroup->AddNewObject(new Engine::Label("[0]",   "pirulen.ttf", 28, 1294, 790, 100, 100, 100));
	UIGroup->AddNewObject(new Engine::Label("Stop",  "pirulen.ttf", 28, 1394, 790, 100, 100, 100));
}

void PlayScene::BombDamage() {
	if (money < Bomb::Price) return;
	money -= Bomb::Price;
	UIMoney->Text = std::string("$") + std::to_string(this->money);
	for (auto &it : EnemyGroup->GetObjects()) {
		Enemy* enemy = dynamic_cast<Enemy*>(it);
		if (!enemy->Visible) continue;
		enemy->Hit(Bomb::Damage);
		enemy->WizardOnExplode();
	}
}
void PlayScene::Surrender() {
	Engine::GameEngine::GetInstance().ChangeScene("lose");
}

void PlayScene::UIBtnClicked(int id) {
	if (preview)
		UIGroup->RemoveObject(preview->GetObjectIterator());
    // TODO: [CUSTOM-TURRET]: On callback, create the turret.
	if (id == 0 && money >= MachineGunTurret::Price)
		preview = new MachineGunTurret(0, 0);
	else if (id == 1 && money >= LaserTurret::Price)
		preview = new LaserTurret(0, 0);
	else if (id == 2 && money >= MissileTurret::Price)
		preview = new MissileTurret(0, 0);
	else if (id == 3 && money >= WizardTurret::Price)
		preview = new WizardTurret(0, 0);
	else if (id == 4 && money >= GunTurret::Price)
		preview = new GunTurret(0, 0);
	else if (id == 5 && money >= Shovel::Price)
		preview = new Shovel(0, 0);
	if (!preview)
		return;
	preview->Position = Engine::GameEngine::GetInstance().GetMousePosition();
	preview->Tint = al_map_rgba(255, 255, 255, 200);
	preview->Enabled = false;
	preview->Preview = true;
	UIGroup->AddNewObject(preview);
	OnMouseMove(Engine::GameEngine::GetInstance().GetMousePosition().x, Engine::GameEngine::GetInstance().GetMousePosition().y);
}

bool PlayScene::CheckSpaceValid(int x, int y) {
	if (x < 0 || x >= MapWidth || y < 0 || y >= MapHeight)
		return false;
	auto map00 = mapState[y][x];
	mapState[y][x] = TILE_OCCUPIED;
	std::vector<std::vector<int>> map = CalculateBFSDistance();
	mapState[y][x] = map00;
	if (map[0][0] == -1)
		return false;
	for (auto& it : EnemyGroup->GetObjects()) {
		Engine::Point pnt;
		pnt.x = floor(it->Position.x / BlockSize);
		pnt.y = floor(it->Position.y / BlockSize);
		if (pnt.x < 0) pnt.x = 0;
		if (pnt.x >= MapWidth) pnt.x = MapWidth - 1;
		if (pnt.y < 0) pnt.y = 0;
		if (pnt.y >= MapHeight) pnt.y = MapHeight - 1;
		if (map[pnt.y][pnt.x] == -1)
			return false;
	}
	// All enemy have path to exit.
	mapState[y][x] = TILE_OCCUPIED;
	mapDistance = map;
	for (auto& it : EnemyGroup->GetObjects())
		dynamic_cast<Enemy*>(it)->UpdatePath(mapDistance);
	return true;
}
std::vector<std::vector<int>> PlayScene::CalculateBFSDistance() {
	// Reverse BFS to find path.
	std::vector<std::vector<int>> map(MapHeight, std::vector<int>(std::vector<int>(MapWidth, -1)));
	std::queue<Engine::Point> que;
	// Push end point.
	// BFS from end point.
	if (mapState[MapHeight - 1][MapWidth - 1] != TILE_DIRT) return map;
	que.push(Engine::Point(MapWidth - 1, MapHeight - 1));
	map[MapHeight - 1][MapWidth - 1] = 0;
	while (!que.empty()) {
		Engine::Point p = que.front(); que.pop();
		
		// TODO?: [BFS PathFinding] : Implement a BFS starting from the most right-bottom block in the map.
		for (auto &i : directions) {
			int newX = p.x + i.x;
			int newY = p.y + i.y;
			if (newX>=0 && newX<MapWidth && newY>=0 && newY<MapHeight && mapState[newY][newX]==TILE_DIRT && map[newY][newX]==-1) {
				map[newY][newX] = map[p.y][p.x] + 1;
				que.push(Engine::Point(newX, newY));
			}
		}
	}
	return map;
}
