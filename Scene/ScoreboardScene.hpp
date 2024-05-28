#ifndef SCOREBOARDSCENE_HPP
#define SCOREBOARDSCENE_HPP
#include <allegro5/allegro_audio.h>
#include <string>
#include <vector>
#include "Engine/IScene.hpp"

class NewRecord { public:
    int score;
	std::string name;
    NewRecord(): score(0), name("") {}
	void reset() { score = 0; name = ""; }
};
struct Record {
	int score;
	std::string date, time, name;
	Record() {}
	Record(std::string date, std::string time, std::string name, int score): date(date), time(time), name(name), score(score) {}
	bool operator<(const Record& other) const { return this->score > other.score; }		// ? for sort
};

class ScoreboardScene final : public Engine::IScene {
private:
	float ticks;
	std::shared_ptr<ALLEGRO_SAMPLE_INSTANCE> bgmInstance;
	int halfW;
	int halfH;
    int from;   // 0: StartScene, 1: afterPlay
public:
	int ScoreboardPagePrevState;
	int ScoreboardPageState;
	Group *ScoreboardButtonGroup;
	Group *ScoreboardDataGroup;
	std::vector<Record> records;
	explicit ScoreboardScene() = default;
	void Initialize() override;
	void Terminate() override;
	void Update(float deltaTime) override;
	void BackOnClick(int stage);
	void NextOnClick();
	void PrevOnClick();
};

extern NewRecord newRecord;

#endif // SCOREBOARDSCENE_HPP
