#include "CLevelWindow.h"

#include "CAvaraApp.h"
#include "CAvaraGame.h"
#include "CLevelDescriptor.h"
#include "CNetManager.h"
#include "Preferences.h"
#include "Resource.h"

CLevelWindow::CLevelWindow(CApplication *app) : CWindow(app, "Levels") {
    // Searches "levels/" directory alongside application.
    // will eventually use level search API
    levelSets = LevelDirNameListing();

    json sets = app->Get(kRecentSets);
    for (json::iterator it = sets.begin(); it != sets.end(); ++it) {
        recentSets.push_back(it.value());
    }
    json levels = app->Get(kRecentLevels);
    for (json::iterator itLev = levels.begin(); itLev != levels.end(); ++itLev) {
        recentLevels.push_back(itLev.value());
    }

    setLayout(new nanogui::BoxLayout(nanogui::Orientation::Vertical, nanogui::Alignment::Fill, 10, 10));

    // TODO: check load permission: theNet->PermissionQuery(kAllowLoadBit, 0)

    recentsBox = new nanogui::DescComboBox(this, recentLevels, recentSets);
    recentsBox->setCaption("Recents");
    recentsBox->setCallback([this](int selectedIdx) {
        this->SelectLevel(recentSets[selectedIdx], recentLevels[selectedIdx]);
        recentsBox->setCaption("Recents");
    });

    setBox = new nanogui::ComboBox(this, levelSets);
    setBox->setCallback([this, app](int selectedIdx) {
        this->SelectSet(selectedIdx);
        levelBox->setSelectedIndex(0);
    });
    setBox->popup()->setSize(nanogui::Vector2i(300, 600));

    levelBox = new nanogui::DescComboBox(this, levelNames, levelIntros);
    levelBox->popup()->setSize(nanogui::Vector2i(500, 350));
    levelBox->setEnabled(false);

    loadBtn = new nanogui::Button(this, "Load Level");
    loadBtn->setCallback([this] { this->SendLoad(); });

    startBtn = new nanogui::Button(this, "Start Game");
    startBtn->setCallback([app] { ((CAvaraAppImpl *)app)->GetGame()->SendStartCommand(); });

    if (recentSets.size() > 0) {
        SelectLevel(recentSets[0], recentLevels[0]);
    } else {
        SelectSet(0);
        levelBox->setSelectedIndex(0);
    }
}

CLevelWindow::~CLevelWindow() {}

bool CLevelWindow::DoCommand(int theCommand) {
    return false;
}

void CLevelWindow::AddRecent(std::string set, std::string levelName) {
    if (json::accept("[\"" + set + "\", \"" + levelName + "\"]")) {
        // remove level if it is already in recents
        for (unsigned i = 0; i < recentSets.size(); i++) {
            if (recentSets[i].compare(set) == 0 && recentLevels[i].compare(levelName) == 0) {
                recentSets.erase(recentSets.begin() + i);
                recentLevels.erase(recentLevels.begin() + i);
                break;
            }
        }

        recentSets.insert(recentSets.begin(), set);
        recentLevels.insert(recentLevels.begin(), levelName);

        if (recentSets.size() > 10) {
            recentSets.pop_back();
            recentLevels.pop_back();
        }

        recentsBox->setItems(recentLevels, recentSets);
        recentsBox->setCaption("Recents");
        recentsBox->setNeedsLayout();

        mApplication->Set(kRecentSets, recentSets);
        mApplication->Set(kRecentLevels, recentLevels);
    } else {
        SDL_Log("AddRecent ignoring bad set/level name.");
    }
}

void CLevelWindow::SelectLevel(std::string set, std::string levelName) {
    SelectSet(set);

    int levelIndex = 0;
    auto levelIt = std::find(levelNames.begin(), levelNames.end(), levelName);
    if (levelIt != levelNames.end()) {
        levelIndex = std::distance(levelNames.begin(), levelIt);
    }

    levelBox->setSelectedIndex(levelIndex);
}

void CLevelWindow::SelectSet(int selected) {
    SelectSet(levelSets[selected]);
}

void CLevelWindow::SelectSet(std::string set) {
    if (set.length() < 1)
        return;
    std::vector<std::string>::iterator itr = std::find(levelSets.begin(), levelSets.end(), set);
    int level_idx = 0;
    if (itr != levelSets.end()) {
        level_idx = std::distance(levelSets.begin(), itr);
        setBox->setSelectedIndex(level_idx);
    }
    levelNames.clear();
    levelIntros.clear();
    levelTags.clear();

    nlohmann::json ledis = LoadLevelListFromJSON(set);
    for (auto &ld : ledis.items()) {
        levelNames.push_back(ld.value()["Name"]);
        levelIntros.push_back(ld.value()["Message"]);
        levelTags.push_back(ld.value()["Alf"]);
    }
    levelBox->setItems(levelNames, levelIntros);
    levelBox->setEnabled(true);
    levelBox->setNeedsLayout();
}

void CLevelWindow::SendLoad() {
    std::string set = levelSets[setBox->selectedIndex()];
    std::string title = levelNames[levelBox->selectedIndex()];
    std::string tag = levelTags[levelBox->selectedIndex()];
    ((CAvaraAppImpl *)gApplication)->GetNet()->SendLoadLevel(set, tag);
}
