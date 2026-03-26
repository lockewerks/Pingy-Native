// The part of this app that actually does something useful.
// One thread per ping target, raw ICMP via Windows API, DNS resolution
// the hard way. Everything else in this codebase exists to make these
// numbers look pretty on screen.
#include "ping_worker.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

PingWorker::PingWorker(const WStr& host, int timeoutMs, int ttl)
    : host_(host), timeoutMs_(timeoutMs), ttl_(ttl) {
    InitializeCriticalSection(&queueLock_);
}

PingWorker::~PingWorker() {
    Stop();
    DeleteCriticalSection(&queueLock_);
}

void PingWorker::Start() {
    InterlockedExchange(&running_, 1);
    thread_ = CreateThread(nullptr, 0, ThreadProc, this, 0, nullptr);
}

void PingWorker::Stop() {
    InterlockedExchange(&running_, 0);
    if (thread_) {
        WaitForSingleObject(thread_, 3000);
        CloseHandle(thread_);
        thread_ = nullptr;
    }
}

void PingWorker::UpdateSettings(int timeoutMs, int ttl) {
    InterlockedExchange(&timeoutMs_, timeoutMs);
    InterlockedExchange(&ttl_, ttl);
}

bool PingWorker::TryDequeue(PingResult& out) {
    EnterCriticalSection(&queueLock_);
    if (queue_.empty()) {
        LeaveCriticalSection(&queueLock_);
        return false;
    }
    out = queue_[0];
    queue_.erase(0);
    LeaveCriticalSection(&queueLock_);
    return true;
}

DWORD WINAPI PingWorker::ThreadProc(LPVOID param) {
    PingWorker* self = (PingWorker*)param;
    self->WorkerLoop();
    return 0;
}

void PingWorker::WorkerLoop() {
    // Resolve hostname to IP
    ULONG destAddr = INADDR_NONE;
    {
        // Convert wide string to narrow for getaddrinfo
        int len = WideCharToMultiByte(CP_UTF8, 0, host_.c_str(), -1, nullptr, 0, nullptr, nullptr);
        char* hostNarrow = (char*)_halloc(len + 1);
        WideCharToMultiByte(CP_UTF8, 0, host_.c_str(), -1, hostNarrow, len, nullptr, nullptr);
        hostNarrow[len] = 0;

        struct addrinfo hints = {}, *result = nullptr;
        hints.ai_family = AF_INET;
        if (getaddrinfo(hostNarrow, nullptr, &hints, &result) == 0 && result) {
            destAddr = ((struct sockaddr_in*)result->ai_addr)->sin_addr.S_un.S_addr;
            freeaddrinfo(result);
        }
        _hfree(hostNarrow);
    }

    if (destAddr == INADDR_NONE) {
        // DNS resolution failed — we'll keep generating failure packets like a
        // passive-aggressive reminder that you typo'd the hostname
        while (InterlockedCompareExchange(&running_, 1, 1)) {
            PingResult r;
            r.timestamp = (INT64)GetTickCount64();
            r.latencyMs = -1;
            r.success = false;
            r.ttl = -1;
            EnterCriticalSection(&queueLock_);
            queue_.push_back(r);
            LeaveCriticalSection(&queueLock_);
            Sleep((DWORD)InterlockedCompareExchange(&timeoutMs_, 0, 0) ? InterlockedCompareExchange(&timeoutMs_, 0, 0) : 500);
            // Re-read running_ properly
            if (!InterlockedCompareExchange(&running_, 1, 1)) break;
        }
        return;
    }

    HANDLE hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) return;

    char sendData[32] = {}; // 32 bytes of absolutely nothing, sent repeatedly into the void
    DWORD replySize = sizeof(ICMP_ECHO_REPLY) + sizeof(sendData) + 8;
    char* replyBuf = (char*)_halloc(replySize);

    while (InterlockedCompareExchange(&running_, 1, 1)) {
        ULONGLONG startTime = GetTickCount64();
        int timeout = timeoutMs_;
        int ttl = ttl_;

        IP_OPTION_INFORMATION opts = {};
        opts.Ttl = (UCHAR)ttl;
        opts.Flags = IP_FLAG_DF;

        PingResult result;
        result.timestamp = (INT64)startTime;

        DWORD ret = IcmpSendEcho(hIcmp, destAddr, sendData, sizeof(sendData),
                                  &opts, replyBuf, replySize, (DWORD)timeout);
        if (ret > 0) {
            PICMP_ECHO_REPLY reply = (PICMP_ECHO_REPLY)replyBuf;
            if (reply->Status == IP_SUCCESS) {
                result.latencyMs = (int)reply->RoundTripTime;
                result.success = true;
                result.ttl = reply->Options.Ttl;
            } else {
                result.latencyMs = -1;
                result.success = false;
                result.ttl = -1;
            }
        } else {
            result.latencyMs = -1;
            result.success = false;
            result.ttl = -1;
        }

        EnterCriticalSection(&queueLock_);
        queue_.push_back(result);
        LeaveCriticalSection(&queueLock_);

        ULONGLONG elapsed = GetTickCount64() - startTime;
        int sleepMs = (int)(timeout - (int)elapsed);
        if (sleepMs < 10) sleepMs = 10;
        if (running_) Sleep((DWORD)sleepMs);
    }

    _hfree(replyBuf);
    IcmpCloseHandle(hIcmp);
}
