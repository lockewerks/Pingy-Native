// Settings I/O: Hand-rolled JSON serialization and parsing.
// "Just use nlohmann/json" they said. "It's header-only" they said.
// It's also 25,000 lines. My entire app is 3,100 lines. Absolutely not.
#include "settings_io.h"

static WStr GetAppDataDir() {
    // Use environment variable to avoid ole32/shell32 forwarding issues
    wchar_t buf[MAX_PATH];
    DWORD len = GetEnvironmentVariableW(L"APPDATA", buf, MAX_PATH);
    if (len > 0 && len < MAX_PATH) {
        WStr dir = WStr(buf) + L"\\Pingy";
        CreateDirectoryW(dir.c_str(), nullptr);
        return dir;
    }
    return WStr(L".");
}

WStr SettingsIO::GetSavePath() {
    return GetAppDataDir() + L"\\pingy_config.json";
}

static Str WtoUTF8(const WStr& ws) {
    if (ws.empty()) return Str();
    int len = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), ws.size(), nullptr, 0, nullptr, nullptr);
    Str s(len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), ws.size(), s.data(), len, nullptr, nullptr);
    return s;
}

static WStr UTF8toW(const Str& s) {
    if (s.empty()) return WStr();
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), s.size(), nullptr, 0);
    // Build WStr by allocating and filling
    wchar_t* buf = (wchar_t*)_halloc((len + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), s.size(), buf, len);
    buf[len] = 0;
    WStr ws(buf, len);
    _hfree(buf);
    return ws;
}

static Str EscapeJson(const WStr& s) {
    Str utf8 = WtoUTF8(s);
    Str out;
    for (int i = 0; i < utf8.size(); i++) {
        char c = utf8[i];
        if (c == '"') { out += "\\\""; }
        else if (c == '\\') { out += "\\\\"; }
        else if (c == '\n') { out += "\\n"; }
        else out += c;
    }
    return out;
}

// Raw Win32 file I/O -- no CRT streams
static bool WriteFileContents(const WStr& path, const char* data, DWORD len) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD written;
    WriteFile(h, data, len, &written, nullptr);
    CloseHandle(h);
    return written == len;
}

static Str ReadFileContents(const WStr& path) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return Str();
    DWORD size = GetFileSize(h, nullptr);
    if (size == INVALID_FILE_SIZE || size == 0) { CloseHandle(h); return Str(); }
    Str buf(size, '\0');
    DWORD bytesRead;
    ReadFile(h, buf.data(), size, &bytesRead, nullptr);
    CloseHandle(h);
    buf.resize(bytesRead);
    return buf;
}

// Artisanal, hand-crafted JSON. Locally sourced string concatenation.
// No schema validation. No pretty-printing options. Just vibes.
void SettingsIO::Save(const SaveData& data) {
    Str json = "{\n";
    json += "  \"settings\": {\n";
    json += "    \"timeoutMs\": "; json += IntToStr(data.settings.timeoutMs); json += ",\n";
    json += "    \"ttl\": "; json += IntToStr(data.settings.ttl); json += ",\n";
    json += "    \"historyLength\": "; json += IntToStr(data.settings.historyLength); json += "\n";
    json += "  },\n";
    json += "  \"targets\": [\n";
    for (int i = 0; i < data.targets.size(); i++) {
        json += "    {\"host\": \""; json += EscapeJson(data.targets[i].host); json += "\", ";
        json += "\"displayName\": \""; json += EscapeJson(data.targets[i].displayName); json += "\"}";
        if (i + 1 < data.targets.size()) json += ",";
        json += "\n";
    }
    json += "  ]\n}\n";

    WriteFileContents(GetSavePath(), json.c_str(), (DWORD)json.size());
}

static void SkipWhitespace(const Str& s, int& pos) {
    while (pos < s.size() && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\n' || s[pos] == '\r'))
        pos++;
}

static Str ParseString(const Str& s, int& pos) {
    if (pos >= s.size() || s[pos] != '"') return Str();
    pos++;
    Str result;
    while (pos < s.size() && s[pos] != '"') {
        if (s[pos] == '\\' && pos + 1 < s.size()) {
            pos++;
            if (s[pos] == '"') result += '"';
            else if (s[pos] == '\\') result += '\\';
            else if (s[pos] == 'n') result += '\n';
            else result += s[pos];
        } else {
            result += s[pos];
        }
        pos++;
    }
    if (pos < s.size()) pos++;
    return result;
}

static int ParseInt(const Str& s, int& pos) {
    SkipWhitespace(s, pos);
    int sign = 1;
    if (pos < s.size() && s[pos] == '-') { sign = -1; pos++; }
    int val = 0;
    while (pos < s.size() && s[pos] >= '0' && s[pos] <= '9') {
        val = val * 10 + (s[pos] - '0');
        pos++;
    }
    return val * sign;
}

// Helper: find substring in Str starting from 'from', return index or -1
static int FindInStr(const Str& s, const char* needle, int from = 0) {
    return s.find(needle, from);
}

// Helper: find char in Str starting from 'from', return index or -1
static int FindCharInStr(const Str& s, char ch, int from = 0) {
    return s.find(ch, from);
}

SaveData SettingsIO::Load() {
    Str content = ReadFileContents(GetSavePath());

    if (!content.empty()) {
        SaveData data;
        int pos = FindInStr(content, "\"timeoutMs\"");
        if (pos != -1) {
            pos = FindCharInStr(content, ':', pos); if (pos != -1) { pos++; data.settings.timeoutMs = ParseInt(content, pos); }
        }
        pos = FindInStr(content, "\"ttl\"");
        if (pos != -1) {
            pos = FindCharInStr(content, ':', pos); if (pos != -1) { pos++; data.settings.ttl = ParseInt(content, pos); }
        }
        pos = FindInStr(content, "\"historyLength\"");
        if (pos != -1) {
            pos = FindCharInStr(content, ':', pos); if (pos != -1) { pos++; data.settings.historyLength = ParseInt(content, pos); }
        }

        pos = FindInStr(content, "\"targets\"");
        if (pos != -1) {
            pos = FindCharInStr(content, '[', pos);
            if (pos != -1) {
                pos++;
                while (pos < content.size()) {
                    SkipWhitespace(content, pos);
                    if (pos < content.size() && content[pos] == ']') break;
                    if (pos < content.size() && content[pos] == ',') { pos++; continue; }

                    int objStart = FindCharInStr(content, '{', pos);
                    if (objStart == -1) break;
                    int objEnd = FindCharInStr(content, '}', objStart);
                    if (objEnd == -1) break;

                    Str obj = content.substr(objStart, objEnd - objStart + 1);
                    SaveEntry entry;

                    int hp = FindInStr(obj, "\"host\"");
                    if (hp != -1) {
                        hp = FindCharInStr(obj, '"', hp + 6);
                        if (hp != -1) entry.host = UTF8toW(ParseString(obj, hp));
                    }
                    int dp = FindInStr(obj, "\"displayName\"");
                    if (dp != -1) {
                        dp = FindCharInStr(obj, '"', dp + 13);
                        if (dp != -1) entry.displayName = UTF8toW(ParseString(obj, dp));
                    }

                    if (!entry.host.empty()) data.targets.push_back(static_cast<SaveEntry&&>(entry));
                    pos = objEnd + 1;
                }
            }
        }

        if (!data.targets.empty()) return data;
    }

    // No config? No problem. Here are the four horsemen of "is the internet working?"
    SaveData defaults;
    {
        SaveEntry e; e.host = L"8.8.8.8"; e.displayName = L"Google DNS Primary"; defaults.targets.push_back(static_cast<SaveEntry&&>(e));
    }
    {
        SaveEntry e; e.host = L"8.8.4.4"; e.displayName = L"Google DNS Secondary"; defaults.targets.push_back(static_cast<SaveEntry&&>(e));
    }
    {
        SaveEntry e; e.host = L"1.1.1.1"; e.displayName = L"Cloudflare Primary"; defaults.targets.push_back(static_cast<SaveEntry&&>(e));
    }
    {
        SaveEntry e; e.host = L"1.0.0.1"; e.displayName = L"Cloudflare Secondary"; defaults.targets.push_back(static_cast<SaveEntry&&>(e));
    }
    {
        SaveEntry e; e.host = L"9.9.9.9"; e.displayName = L"Quad9"; defaults.targets.push_back(static_cast<SaveEntry&&>(e));
    }
    {
        SaveEntry e; e.host = L"208.67.222.222"; e.displayName = L"OpenDNS"; defaults.targets.push_back(static_cast<SaveEntry&&>(e));
    }
    Save(defaults);
    return defaults;
}
