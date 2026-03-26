#include "ping_manager.h"
#include "settings_io.h"

PingManager::PingManager() {}

PingManager::~PingManager() {
    Shutdown();
}

void PingManager::Initialize() {
    SaveData data = SettingsIO::Load();
    settings_ = data.settings;

    for (int i = 0; i < data.targets.size(); i++) {
        PingTarget target;
        target.host = data.targets[i].host;
        target.displayName = data.targets[i].displayName;
        target.InitializeHistory(settings_.historyLength);
        targets_.push_back(static_cast<PingTarget&&>(target));

        PingWorker* w = new PingWorker(targets_.back().host, settings_.timeoutMs, settings_.ttl);
        workers_.push_back(w);
    }

    isRunning_ = false;
}

void PingManager::Shutdown() {
    for (int i = 0; i < workers_.size(); i++) {
        workers_[i]->Stop();
    }
    Save();
    for (int i = 0; i < workers_.size(); i++) {
        delete workers_[i];
    }
    workers_.clear();
}

bool PingManager::Update() {
    if (!isRunning_) return false;
    bool anyNew = false;
    int count = targets_.size();
    if (workers_.size() < count) count = workers_.size();
    for (int i = 0; i < count; i++) {
        PingResult result;
        while (workers_[i]->TryDequeue(result)) {
            targets_[i].AddResult(result);
            anyNew = true;
        }
    }
    return anyNew;
}

void PingManager::StartAll() {
    if (isRunning_) return;
    isRunning_ = true;
    for (int i = 0; i < workers_.size(); i++) {
        workers_[i]->Start();
    }
}

void PingManager::StopAll() {
    if (!isRunning_) return;
    isRunning_ = false;
    for (int i = 0; i < workers_.size(); i++) {
        workers_[i]->Stop();
        delete workers_[i];
    }
    workers_.clear();
    // Recreate workers so they can be started again
    for (int i = 0; i < targets_.size(); i++) {
        PingWorker* w = new PingWorker(targets_[i].host, settings_.timeoutMs, settings_.ttl);
        workers_.push_back(w);
    }
}

void PingManager::ToggleRunning() {
    if (isRunning_) StopAll();
    else StartAll();
}

void PingManager::AddTarget(const WStr& host, const WStr& displayName) {
    for (int i = 0; i < targets_.size(); i++)
        if (targets_[i].host == host) return;

    PingTarget target;
    target.host = host;
    target.displayName = displayName;
    target.InitializeHistory(settings_.historyLength);
    targets_.push_back(static_cast<PingTarget&&>(target));

    PingWorker* worker = new PingWorker(host, settings_.timeoutMs, settings_.ttl);
    if (isRunning_) worker->Start();
    workers_.push_back(worker);

    Save();
}

void PingManager::RemoveTarget(const WStr& host) {
    for (int i = 0; i < targets_.size(); i++) {
        if (targets_[i].host == host) {
            workers_[i]->Stop();
            delete workers_[i];
            targets_.erase(i);
            workers_.erase(i);
            break;
        }
    }
    Save();
}

void PingManager::UpdateSettings(int timeoutMs, int ttl) {
    settings_.timeoutMs = timeoutMs;
    settings_.ttl = ttl;
    for (int i = 0; i < workers_.size(); i++)
        workers_[i]->UpdateSettings(timeoutMs, ttl);
    Save();
}

void PingManager::Save() {
    SaveData data;
    data.settings = settings_;
    for (int i = 0; i < targets_.size(); i++) {
        SaveEntry e;
        e.host = targets_[i].host;
        e.displayName = targets_[i].displayName;
        data.targets.push_back(static_cast<SaveEntry&&>(e));
    }
    SettingsIO::Save(data);
}
