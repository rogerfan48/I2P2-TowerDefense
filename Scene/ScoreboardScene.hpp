#ifndef SCOREBOARDSCENE_HPP
#define SCOREBOARDSCENE_HPP
#include <allegro5/allegro_audio.h>
#include "Engine/IScene.hpp"

struct NewRecord {
    bool win;
    int score;
    NewRecord(): win(false), score(-1) {}
    NewRecord(bool win, int score): win(win), score(score) {}
};

class ScoreboardScene final : public Engine::IScene {
private:
	float ticks;
	std::shared_ptr<ALLEGRO_SAMPLE_INSTANCE> bgmInstance;
    NewRecord newRecord;
    int from;   // 0: StartScene, 1: afterPlay
public:
	explicit ScoreboardScene() = default;
	void Initialize() override;
	void Terminate() override;
	void Update(float deltaTime) override;
	void BackOnClick(int stage);
};

#endif // SCOREBOARDSCENE_HPP
