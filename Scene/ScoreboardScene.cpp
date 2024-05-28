#include <functional>
#include <string>
#include <fstream>
#include <ctime>
#include <algorithm>

#include "Engine/AudioHelper.hpp"
#include "Engine/GameEngine.hpp"
#include "UI/Component/Image.hpp"
#include "UI/Component/ImageButton.hpp"
#include "UI/Component/Label.hpp"
#include "PlayScene.hpp"
#include "Engine/Point.hpp"
#include "Engine/LOG.hpp"
#include "ScoreboardScene.hpp"

void ScoreboardScene::Initialize() {
	ticks = 0;
	int w = Engine::GameEngine::GetInstance().GetScreenSize().x;
	int h = Engine::GameEngine::GetInstance().GetScreenSize().y;
	halfW = w / 2;
	halfH = h / 2;
	AddNewObject(new Engine::Label("Scoreboard", "pirulen.ttf", 64, halfW, halfH / 4 -10, 255, 255, 255, 255, 0.5, 0.5));
	Engine::ImageButton* btn;
	btn = new Engine::ImageButton("win/dirt.png", "win/floor.png", halfW - 200, halfH * 7 / 4 - 50, 400, 100);
	btn->SetOnClickCallback(std::bind(&ScoreboardScene::BackOnClick, this, 2));
	AddNewControlObject(btn);
	AddNewObject(new Engine::Label("Back", "pirulen.ttf", 48, halfW, halfH * 7 / 4, 0, 0, 0, 255, 0.5, 0.5));
	bgmInstance = AudioHelper::PlaySample("select.ogg", true, AudioHelper::BGMVolume);
	AddNewControlObject(ScoreboardButtonGroup = new Group());
	AddNewObject(ScoreboardDataGroup = new Group());
	ScoreboardPagePrevState = 0;
	ScoreboardPageState = 1;
	btn = new Engine::ImageButton("win/dirt.png", "win/floor.png", halfW - 200 + 450, halfH * 7 / 4 - 50, 400, 100);
	btn->SetOnClickCallback(std::bind(&ScoreboardScene::NextOnClick, this));
	ScoreboardButtonGroup->AddNewControlObject(btn);
	ScoreboardButtonGroup->AddNewObject(new Engine::Label("Next", "pirulen.ttf", 48, halfW + 450, halfH * 7 / 4, 0, 0, 0, 255, 0.5, 0.5));
    
	std::ifstream inFile("Resource/scores.txt");
	if (!inFile) Engine::LOG(Engine::ERROR) << "Unable to open scoreboard file\n";
	int recordNum, iScore;
	std::string iDate, iTime, iName;
	inFile >> recordNum;
	records.clear();
	for (int i=0; i<recordNum; i++) {
		inFile >> iDate >> iTime >> iName >> iScore;
		records.emplace_back(iDate, iTime, iName, iScore);
	}
	inFile.close();
	if (newRecord.name != "") {
		auto nowTime = std::time(nullptr);
		auto t = std::localtime(&nowTime);
		records.emplace_back(std::to_string(t->tm_year+1900) + "/" + (t->tm_mon<9 ? "0" : "") + std::to_string(t->tm_mon+1) + "/" + (t->tm_mday<10 ? "0" : "")+ std::to_string(t->tm_mday),
							(t->tm_hour<10 ? "0" : "") + std::to_string(t->tm_hour) + ":" + (t->tm_min<10 ? "0" : "") + std::to_string(t->tm_min),
							newRecord.name, newRecord.score);
		// std::cout << "###" << records.size() << std::endl;
		std::sort(records.begin(), records.end());
		newRecord.reset();
		std::ofstream outFile("Resource/scores.txt");
		if (!outFile) Engine::LOG(Engine::ERROR) << "Unable to write in scoreboard file\n";
		if (records.size()==21) records.pop_back();
		outFile << records.size() << std::endl;
		for (auto i : records) {
			outFile << i.date << " " << i.time << " " << i.name << " " << i.score << std::endl;
		}
		outFile.close();
	}
}
void ScoreboardScene::Terminate() {
	AudioHelper::StopSample(bgmInstance);
	bgmInstance = std::shared_ptr<ALLEGRO_SAMPLE_INSTANCE>();
	IScene::Terminate();
}
void ScoreboardScene::Update(float deltaTime) {
    if (ScoreboardPageState==1 && ScoreboardPagePrevState!=1) {
		ScoreboardButtonGroup->Clear();
		Engine::ImageButton* btn = new Engine::ImageButton("win/dirt.png", "win/floor.png", halfW - 200 + 450, halfH * 7 / 4 - 50, 400, 100);
		btn->SetOnClickCallback(std::bind(&ScoreboardScene::NextOnClick, this));
		ScoreboardButtonGroup->AddNewControlObject(btn);
		ScoreboardButtonGroup->AddNewObject(new Engine::Label("Next", "pirulen.ttf", 48, halfW + 450, halfH * 7 / 4, 0, 0, 0, 255, 0.5, 0.5));
		ScoreboardDataGroup->Clear();
		int strLen = std::min(static_cast<int>(records.size()), 10);
		for (int i=0; i<strLen; i++) {
			ScoreboardDataGroup->AddNewObject(new Engine::Label(records[i].date, "pirulen.ttf", 40, halfW - 600, halfH - 260 +i*48, 140, 220, 250, 255));
		}
		for (int i=0; i<strLen; i++) {
			ScoreboardDataGroup->AddNewObject(new Engine::Label(records[i].time, "pirulen.ttf", 40, halfW - 250, halfH - 260 +i*48, 140, 220, 250, 255));
		}
		for (int i=0; i<strLen; i++) {
			ScoreboardDataGroup->AddNewObject(new Engine::Label(records[i].name, "pirulen.ttf", 40, halfW - 20, halfH - 260 +i*48, 140, 220, 250, 255));
		}
		for (int i=0; i<strLen; i++) {
			ScoreboardDataGroup->AddNewObject(new Engine::Label(std::to_string(records[i].score), "pirulen.ttf", 40, halfW + 400, halfH - 260 +i*48, 140, 220, 250, 255));
		}
	} else if (ScoreboardPageState==2 && ScoreboardPagePrevState!=2) {
		ScoreboardButtonGroup->Clear();
		Engine::ImageButton* btn = new Engine::ImageButton("win/dirt.png", "win/floor.png", halfW - 200 - 450, halfH * 7 / 4 - 50, 400, 100);
		btn->SetOnClickCallback(std::bind(&ScoreboardScene::PrevOnClick, this));
		ScoreboardButtonGroup->AddNewControlObject(btn);
		ScoreboardButtonGroup->AddNewObject(new Engine::Label("Prev", "pirulen.ttf", 48, halfW - 450, halfH * 7 / 4, 0, 0, 0, 255, 0.5, 0.5));
		ScoreboardDataGroup->Clear();
		int strLen = std::min(static_cast<int>(records.size()-10), 10);
		for (int i=0; i<strLen; i++) {
			ScoreboardDataGroup->AddNewObject(new Engine::Label(records[i+10].date, "pirulen.ttf", 40, halfW - 600, halfH - 260 +i*48, 140, 220, 250, 255));
		}
		for (int i=0; i<strLen; i++) {
			ScoreboardDataGroup->AddNewObject(new Engine::Label(records[i+10].time, "pirulen.ttf", 40, halfW - 250, halfH - 260 +i*48, 140, 220, 250, 255));
		}
		for (int i=0; i<strLen; i++) {
			ScoreboardDataGroup->AddNewObject(new Engine::Label(records[i+10].name, "pirulen.ttf", 40, halfW - 20, halfH - 260 +i*48, 140, 220, 250, 255));
		}
		for (int i=0; i<strLen; i++) {
			ScoreboardDataGroup->AddNewObject(new Engine::Label(std::to_string(records[i+10].score), "pirulen.ttf", 40, halfW + 400, halfH - 260 +i*48, 140, 220, 250, 255));
		}
	}
	if (ScoreboardPageState != ScoreboardPagePrevState) ScoreboardPagePrevState = ScoreboardPageState;
}
void ScoreboardScene::BackOnClick(int stage) {
	Engine::GameEngine::GetInstance().ChangeScene("stage-select");
}
void ScoreboardScene::NextOnClick() {
	ScoreboardPageState = 2;
}
void ScoreboardScene::PrevOnClick() {
	ScoreboardPageState = 1;
}
