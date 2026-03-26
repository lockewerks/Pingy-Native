#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <icmpapi.h>
#include "data.h"

class PingWorker {
public:
    PingWorker(const WStr& host, int timeoutMs, int ttl);
    ~PingWorker();

    void Start();
    void Stop();
    void UpdateSettings(int timeoutMs, int ttl);
    bool TryDequeue(PingResult& out);
    const WStr& Host() const { return host_; }

private:
    static DWORD WINAPI ThreadProc(LPVOID param);
    void WorkerLoop();

    WStr host_;
    volatile LONG running_ = 0;
    volatile LONG timeoutMs_;
    volatile LONG ttl_;
    HANDLE thread_ = nullptr;

    CRITICAL_SECTION queueLock_;
    Vec<PingResult> queue_;
};
