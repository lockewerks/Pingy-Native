# Pingy

**A 16KB real-time network ping visualizer for Windows that absolutely did not need to be this overengineered.**

Pingy monitors multiple hosts simultaneously and renders their latency history as smooth, interpolated curves on a 3D perspective graph. It's `ping` if `ping` went to art school, dropped out, and got really into the demoscene.

## What The Hell Is This

It's a native Windows desktop app that:

- Pings multiple hosts concurrently on background threads
- Renders real-time latency data using **Direct2D** with **Catmull-Rom spline interpolation**
- Displays the results on a **3D perspective-projected graph** with parallax depth between targets
- Shows live stats per target: current, average, min, max latency + packet loss
- Saves your targets and settings to a JSON config (hand-parsed, obviously)
- Ships as a **16KB standalone executable** via [Crinkler](https://github.com/runestubbe/Crinkler) (demoscene compressing linker) with zero external dependencies

## The Absurd Technical Decisions

Let me be clear: **none of this was necessary.**

| Decision | Justification |
|----------|--------------|
| Zero CRT dependency | Who needs a C runtime when you can hand-roll `memcpy` like it's 1985? |
| Custom `Vec<T>`, `WStr`, `Str` | STL is for people who value their time. I am not that person. |
| Polynomial approximations for `sin`, `cos`, `tan`, `sqrt` | The math library was too many kilobytes. Taylor series it is. |
| x86 inline assembly for 64-bit division | Because `_aulldiv` isn't going to implement itself. Oh wait, it literally is. |
| Hand-written JSON serializer/parser | No third-party dependencies means no third-party dependencies. I *meant* it. |
| 3D perspective projection for a ping graph | Your latency data deserves depth. Literally. |
| Crinkler as the actual linker | The executable needed to be smaller than a particularly ambitious JPEG. And it is. 16KB. |
| Custom entry point bypassing CRT init | `_entry()` loads `ole32.dll` manually via `GetProcAddress` like a goddamn animal. |
| `Sleep(1)` render loop | ~1000fps cap. Your ping graph runs smoother than most AAA games. |

## Features

- **Multi-target monitoring** — Add any hostname or IP address, watch them all at once
- **3D perspective graph** — Catmull-Rom splines with per-target Z-depth parallax
- **Live statistics** — Current/avg/min/max latency and packet loss percentage per target
- **Responsive layout** — Switches between wide (sidebar left) and tall (sidebar bottom) modes
- **Per-monitor DPI awareness** — Looks crisp on your 4K monitor and your shitty second display
- **Configurable timeout & TTL** — Slider controls because I have *standards*
- **Persistent config** — Targets and settings survive app restarts
- **Color-coded latency** — Green (good), yellow (meh), red (your ISP is lying to you)
- **Default targets** — Starts with Google DNS, Cloudflare, Quad9, and OpenDNS because those are the four horsemen of "is the internet working?"

## Building

### Prerequisites

- **Windows 10+** (this is a Win32 app, sorry macOS/Linux people, you have `mtr`)
- **Visual Studio Build Tools** with C++ workload (need `cl.exe` and `rc.exe`)
- A terminal with the VS developer environment loaded

### Build

```batch
build.bat
```

That's it. One script. It compiles everything with `/O1` (optimize for size), links with [Crinkler](https://github.com/runestubbe/Crinkler) (a demoscene compressing linker), and produces a ~16KB executable. Sixteen. Kilobytes.

The build script:
1. Compiles `crt_mini.cpp` separately (my CRT replacement — can't use `/GL` with intrinsics)
2. Compiles all other source files with whole program optimization
3. Links with Crinkler, custom entry point (`_entry`), no default libs
4. Merges `.rdata` into `.text` because every byte counts when you've lost the plot

### Output

| File | Size | What |
|------|------|------|
| `build\PingyCrinkled.exe` | ~16 KB | Release build via Crinkler |

## Architecture

```
src/
  main.cpp              — 20 lines. WinMain creates App, runs it. Done.
  app.cpp               — Window creation, message loop, input dispatch
  renderer.cpp          — Direct2D/DirectWrite wrapper. 11 text formats because I'm fancy.
  graph.cpp             — The crown jewel. 3D perspective camera, spline interpolation, grid rendering.
  ping_worker.cpp       — One thread per target. ICMP via IcmpSendEcho. DNS via getaddrinfo.
  ping_manager.cpp      — Manages workers, dequeues results, maintains state
  settings_io.cpp       — "JSON library at home." Hand-built serialization and parsing.
  layout.cpp            — Responsive layout math. 56 lines of surprisingly clean code.
  ui_sidebar.cpp        — Target list with stats, scrolling, hover states
  ui_settings.cpp       — Timeout/TTL sliders
  ui_add_dialog.cpp     — Modal dialog for adding targets
  ui_text_input.cpp     — Text field with cursor, selection, clipboard support
  containers.h          — Vec<T>, WStr, Str — my bootleg STL, heap-allocated with love
  data.h                — PingTarget with circular buffer, O(1) running statistics
  crt_mini.cpp          — Custom CRT: entry point, heap, memcpy/memset, trig approximations, x86 asm
  math_util.h           — Catmull-Rom, lerp, vector math
  theme.h               — Colors, dimensions, graph constants. The entire visual identity in 78 lines.
```

## How It Works (the short version)

1. `_entry()` initializes the heap, Winsock, and COM — all without CRT
2. Creates a maximized window with Direct2D rendering
3. Loads saved targets (or defaults to Google/Cloudflare/Quad9/OpenDNS DNS servers)
4. Spawns a worker thread per target that loops: resolve DNS, open ICMP handle, ping, sleep, repeat
5. Main thread polls results from lock-free queues, updates circular buffers with O(1) stats
6. Renders at ~1000fps: 3D perspective grid, Catmull-Rom spline curves per target, sidebar with live stats
7. You stare at the pretty lines and feel something about your network latency

## Default Targets

| Host | Name |
|------|------|
| `8.8.8.8` | Google DNS Primary |
| `8.8.4.4` | Google DNS Secondary |
| `1.1.1.1` | Cloudflare Primary |
| `1.0.0.1` | Cloudflare Secondary |
| `9.9.9.9` | Quad9 |
| `208.67.222.222` | OpenDNS |

## FAQ

**Q: Why not just use `ping` in a terminal?**
A: Because `ping` doesn't have a 3D perspective graph with Catmull-Rom spline interpolation, Karen.

**Q: Why no CRT?**
A: Because the C runtime adds ~100KB and I took that personally.

**Q: Does it run on Linux/macOS?**
A: No. It uses Win32, Direct2D, ICMP via IPHLPAPI, and hand-written x86 assembly. Porting this would require rewriting approximately everything.

**Q: Why is the executable so small?**
A: Zero CRT, no STL, no external dependencies, aggressive compiler flags, and Crinkler — a demoscene compressing linker designed for 4K intros. I brought a nuclear weapon to a knife fight.

**Q: Is this a demoscene production?**
A: It has the spirit of one. Custom entry point, hand-rolled math, inline assembly, placement new, and a compressing linker. It just happens to also be useful.

**Q: Can I add my own targets?**
A: Yes. Click the "+ ADD TARGET" button. Type a hostname or IP. Hit Enter. Rocket science.

## License

Do whatever you want with it. Seriously. If you want to use this code to monitor your grandma's WiFi latency with a 3D perspective graph, go for it. That's exactly what this is for.
