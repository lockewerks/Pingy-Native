#pragma once
#include "data.h"
#include "ping_worker.h"

class PingManager {
public:
    PingManager();
    ~PingManager();

    void Initialize();
    void Shutdown();
    bool Update(); // returns true if new data arrived

    void StartAll();
    void StopAll();
    void ToggleRunning();
    bool IsRunning() const { return isRunning_; }

    void AddTarget(const WStr& host, const WStr& displayName);
    void RemoveTarget(const WStr& host);
    void UpdateSettings(int timeoutMs, int ttl);

    Vec<PingTarget>& Targets() { return targets_; }
    const Vec<PingTarget>& Targets() const { return targets_; }
    AppSettings& Settings() { return settings_; }
    const AppSettings& Settings() const { return settings_; }

private:
    void Save();

    Vec<PingTarget> targets_;
    Vec<PingWorker*> workers_;
    AppSettings settings_;
    bool isRunning_ = false;
};
