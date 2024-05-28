#include <functional>
#include <string>

#include "Engine/AudioHelper.hpp"
#include "Engine/GameEngine.hpp"
#include "UI/Component/Image.hpp"
#include "UI/Component/ImageButton.hpp"
#include "UI/Component/Label.hpp"
#include "PlayScene.hpp"
#include "Engine/Point.hpp"
#include "WinScene.hpp"
#include "Scene/ScoreboardScene.hpp"

void WinScene::Initialize() {
	ticks = 0;
	int w = Engine::GameEngine::GetInstance().GetScreenSize().x;
	int h = Engine::GameEngine::GetInstance().GetScreenSize().y;
	int halfW = w / 2;
	int halfH = h / 2;
	AddNewObject(new Engine::Image("win/benjamin-sad.png", halfW, halfH, 0, 0, 0.5, 0.5));
	AddNewObject(new Engine::Label("You Win!", "pirulen.ttf", 48, halfW, halfH / 4 -10, 255, 255, 255, 255, 0.5, 0.5));
	Engine::ImageButton* btn;
	btn = new Engine::ImageButton("win/dirt.png", "win/floor.png", halfW - 200, halfH * 7 / 4 - 50, 400, 100);
	btn->SetOnClickCallback(std::bind(&WinScene::BackOnClick, this, 2));
	AddNewControlObject(btn);
	AddNewObject(new Engine::Label("Next", "pirulen.ttf", 48, halfW, halfH * 7 / 4, 0, 0, 0, 255, 0.5, 0.5));
	bgmId = AudioHelper::PlayAudio("win.wav");
	AddNewObject(new Engine::Image("win/white.png", halfW, halfH, 720+5, 100+5, 0.5, 0.5)); 	// ? ERROR ? BUT WHY
	AddNewObject(new Engine::Image("win/dirt.png", halfW, halfH, 720, 100, 0.5, 0.5));
	AddNewObject(NameGroup = new Group());
	NameGroup->AddNewObject(new Engine::Label(newRecord.name, "pirulen.ttf", 72, halfW, halfH, 0, 0, 0, 255, 0.5, 0.5));
}
void WinScene::Terminate() {
	IScene::Terminate();
	AudioHelper::StopBGM(bgmId);
}
void WinScene::Update(float deltaTime) {
	ticks += deltaTime;
	if (ticks > 4 && ticks < 100 &&
		dynamic_cast<PlayScene*>(Engine::GameEngine::GetInstance().GetScene("play"))->MapId == 2) {
		ticks = 100;
		bgmId = AudioHelper::PlayBGM("happy.ogg");
	}
	int w = Engine::GameEngine::GetInstance().GetScreenSize().x;
	int h = Engine::GameEngine::GetInstance().GetScreenSize().y;
	int halfW = w / 2;
	int halfH = h / 2;
	NameGroup->RemoveObject((NameGroup->GetObjects()).front()->GetObjectIterator());
	NameGroup->AddNewObject(new Engine::Label(newRecord.name, "pirulen.ttf", 72, halfW, halfH, 0, 0, 0, 255, 0.5, 0.5));
}
void WinScene::BackOnClick(int stage) {
	Engine::GameEngine::GetInstance().ChangeScene("scoreboard");
}
void WinScene::OnKeyDown(int keyCode) {
	if (((keyCode >= ALLEGRO_KEY_A && keyCode <= ALLEGRO_KEY_9)) && newRecord.name.size()<12) {
		newRecord.name += (al_keycode_to_name(keyCode));
	} else if (keyCode == ALLEGRO_KEY_BACKSPACE) {
		if (!newRecord.name.empty())newRecord.name.pop_back();
	} else if (keyCode == ALLEGRO_KEY_ENTER) {
		if (newRecord.name == "") newRecord.name = "Undefined";
		Engine::GameEngine::GetInstance().ChangeScene("scoreboard");
	}
}