#pragma once
#include "data.h"

namespace SettingsIO {
    SaveData Load();
    void Save(const SaveData& data);
    WStr GetSavePath();
}
