#pragma once
#include "containers.h"
#include <float.h>

// Data structures for ping results and target state.
// Features a circular buffer with O(1) running statistics,
// and move constructors written by hand because std::move is part
// of <utility> and <utility> is part of the CRT and the CRT is dead to me.

struct PingResult {
    INT64 timestamp = 0;  // Unix milliseconds
    int latencyMs = -1;     // -1 = timeout/failure
    bool success = false;
    int ttl = -1;
};

struct PingTarget {
    WStr host;
    WStr displayName;

    // Circular buffer
    Vec<PingResult> historyBuffer;
    int historyHead = 0;
    int historyCount = 0;

    // Running stats
    float currentLatency = -1.0f;
    float averageLatency = 0.0f;
    float minLatency = 0.0f;
    float maxLatency = 0.0f;
    float packetLossPercent = 0.0f;
    bool isActive = true;

    int successCount_ = 0;
    INT64 latencySum_ = 0;

    PingTarget() = default;
    PingTarget(const PingTarget& o) = default;
    PingTarget& operator=(const PingTarget& o) = default;

    // Move constructor: static_cast<T&&> instead of std::move because I'm
    // too proud to include a single standard library header. This is the way.
    PingTarget(PingTarget&& o) noexcept
        : host(static_cast<WStr&&>(o.host)),
          displayName(static_cast<WStr&&>(o.displayName)),
          historyBuffer(static_cast<Vec<PingResult>&&>(o.historyBuffer)),
          historyHead(o.historyHead), historyCount(o.historyCount),
          currentLatency(o.currentLatency), averageLatency(o.averageLatency),
          minLatency(o.minLatency), maxLatency(o.maxLatency),
          packetLossPercent(o.packetLossPercent), isActive(o.isActive),
          successCount_(o.successCount_), latencySum_(o.latencySum_)
    {
        o.historyHead = 0; o.historyCount = 0;
        o.successCount_ = 0; o.latencySum_ = 0;
    }

    PingTarget& operator=(PingTarget&& o) noexcept {
        if (this != &o) {
            host = static_cast<WStr&&>(o.host);
            displayName = static_cast<WStr&&>(o.displayName);
            historyBuffer = static_cast<Vec<PingResult>&&>(o.historyBuffer);
            historyHead = o.historyHead; historyCount = o.historyCount;
            currentLatency = o.currentLatency; averageLatency = o.averageLatency;
            minLatency = o.minLatency; maxLatency = o.maxLatency;
            packetLossPercent = o.packetLossPercent; isActive = o.isActive;
            successCount_ = o.successCount_; latencySum_ = o.latencySum_;
            o.historyHead = 0; o.historyCount = 0;
            o.successCount_ = 0; o.latencySum_ = 0;
        }
        return *this;
    }

    void InitializeHistory(int capacity) {
        historyBuffer.resize(capacity);
        historyHead = 0;
        historyCount = 0;
        currentLatency = -1.0f;
        averageLatency = 0.0f;
        minLatency = 0.0f;
        maxLatency = 0.0f;
        packetLossPercent = 0.0f;
        isActive = true;
        successCount_ = 0;
        latencySum_ = 0;
    }

    void AddResult(const PingResult& result) {
        if (historyBuffer.empty()) return;
        int cap = historyBuffer.size();

        // If buffer full, subtract oldest from running stats
        if (historyCount == cap) {
            auto& oldest = historyBuffer[historyHead];
            if (oldest.success) {
                successCount_--;
                latencySum_ -= oldest.latencyMs;
            }
        } else {
            historyCount++;
        }

        historyBuffer[historyHead] = result;
        historyHead = (historyHead + 1) % cap;

        if (result.success) {
            successCount_++;
            latencySum_ += result.latencyMs;
            currentLatency = (float)result.latencyMs;
        } else {
            currentLatency = -1.0f;
        }

        averageLatency = successCount_ > 0 ? (float)latencySum_ / successCount_ : 0.0f;
        packetLossPercent = historyCount > 0 ? (1.0f - (float)successCount_ / historyCount) * 100.0f : 0.0f;

        RecomputeMinMax();
    }

    void RecomputeMinMax() {
        minLatency = FLT_MAX;
        maxLatency = 0.0f;
        int cap = historyBuffer.size();
        int start = (historyHead - historyCount + cap) % cap;
        for (int i = 0; i < historyCount; i++) {
            int idx = (start + i) % cap;
            auto& r = historyBuffer[idx];
            if (r.success) {
                if (r.latencyMs < minLatency) minLatency = (float)r.latencyMs;
                if (r.latencyMs > maxLatency) maxLatency = (float)r.latencyMs;
            }
        }
        if (minLatency == FLT_MAX) minLatency = 0.0f;
    }

    PingResult GetHistoryAt(int chronologicalIndex) const {
        int cap = historyBuffer.size();
        int start = (historyHead - historyCount + cap) % cap;
        int idx = (start + chronologicalIndex) % cap;
        return historyBuffer[idx];
    }
};

struct AppSettings {
    int timeoutMs = 500;
    int ttl = 128;
    int historyLength = 600;
};

struct SaveEntry {
    WStr host;
    WStr displayName;

    SaveEntry() = default;
    SaveEntry(const SaveEntry&) = default;
    SaveEntry& operator=(const SaveEntry&) = default;
    SaveEntry(SaveEntry&& o) noexcept : host(static_cast<WStr&&>(o.host)), displayName(static_cast<WStr&&>(o.displayName)) {}
    SaveEntry& operator=(SaveEntry&& o) noexcept { host = static_cast<WStr&&>(o.host); displayName = static_cast<WStr&&>(o.displayName); return *this; }
};

struct SaveData {
    Vec<SaveEntry> targets;
    AppSettings settings;
};
