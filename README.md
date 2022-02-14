![CytosBanner](https://user-images.githubusercontent.com/38842891/153798655-ac94c671-d9c0-44b1-9365-65ecdf51c36e.png)

**Cytos:** *The best* performant open source agar.io server & client combined in one project.

# Main UI Preview
![image](https://user-images.githubusercontent.com/38842891/153813559-8c7aaad1-5e20-4dd5-b894-6dff02670873.png)

* Built with Electron and native c++ addons.
* Highly parallelized "server".
* Accelerated WebGL rendering.

# Benchmarks
* 12800 x 12800 map
* 369 max cells per player
* 1000 bots which randomly split and feed if the load is below 75% so the engine is not overloaded too much
* 10000 pellets (not rendered if viewport is greater than a threshold but still handled)

### Benchmark 1 - Desktop
| Specs  | Result |
| ------ | ------ |
| Ryzen 7 5800x & 3070Ti, using 8 threads (bind to core)  | Way overkill with the hardware, average 20-30% load with ~40,000 cells being rendered at 75FPS (~150FPS with vsync disabled)  |

https://user-images.githubusercontent.com/38842891/153801485-576679fc-9768-4d7f-b844-cc04afaad05d.mp4

### Benchmark 2 - Laptop
| Specs  | Result |
| ------ | ------ |
| i5-8250U (Surface Pro 7), using 8 threads (bind to thread) | Still very much playable even though load jumps between 60% to 200% with ~15,000 cells being rendered at 40-60FPS

https://user-images.githubusercontent.com/38842891/153805872-456811a0-5cf5-48d8-ae30-80a1c3bbb804.mp4

# Architecture
**There is NO networking at all in this project**, but the concept of a server and client still exists:
* A Web Worker is used as the server, and `cytos-addon.node` is loaded which has a native libuv loop integrated into the browser. It runs the physics solving and viewport querying at 25 TPS and a buffer is serialized and sent to the main page with `self.postMessage`. The addon also contains a thread pool to accelerate calculations.
* Main page is used as the client, and another native module, `gfx-addon.node`, is loaded to parse the buffer got from the worker/server and store the state; then during `requestAnimationFrame` callback the module is called to generate the WebGL buffer.

# Optimizations & Parallelism
A lot of techniques are borrowed from my older project [OgarX](https://github.com/Yuu6883/OgarX/blob/master/DEEP_DIVE.md), but there are some in-depth native optimization that was not possible in pure JS:
* Thread pool with hardware binding,
* Memory pooling, done in both server and client to minimize memory allocation and increase cache coherence.
* Packed struct and aligned memory: Cell struct is exact 64 bytes, and address is aligned to 64 bytes. This enables high performance multithread with much reduced false sharing.
* Implemented LooseQuadTree from this [paper](https://www.mathematik.tu-clausthal.de/fileadmin/AG-StochastischeOptimierung/papers/LooseOctreePaper.pdf).
* Multi-stage collision/eat solving with atomic exchanged operations instead of mutexes to avoid locking overhead.
* Ejected cells and pellets are now stored in separate 2D grids (also thread-safe), enabling more efficient querying due to mono-size.
* Game mode config is *all* templated in order for the compiler to pickup any possible optimization route. This means the entire engine is header only and build time is slower.
* Almost every part of the engine is now parallelized, e.g. cell updating, player splits & ejects, and every physics solving stage.

All those changes brings about **3-4x single thread speedup** and **~20x speedup with 8 threads.**

# <a href="https://github.com/Yuu6883/Cytos/releases/tag/v1.0.0">Download</a>
Installer currently only available for Windows, but building for other platforms would be possible since `electron` and `cmake-js` are cross platform (see Developement section below)
**Windows will show a warning popup because the installer not a signed binary.**

# Development
Requirements: 
* cmake > 3.10
* MSVC / gcc / clang 
* npm
* cmake-js installed globablly with `npm i -g cmake-js` (or could be added as a dependency later...)

```bash
git clone https://github.com/Yuu6883/Cytos.git
cd Cytos
./addon.sh
cd source-js
npm i
npm run build
cp ../build/Release/*.node build/
npm start
```

# Additional Features
* Save & Restore: hit `CTRL S` to save a server state into a buffer stored in IndexedDB and hit `ALT X` to restore from it (per game mode)
* Extensive timing & metrics: hit `F1` to toggle the profiling panel.
* Hit `F4` for the ultimate easter egg:

![image](https://user-images.githubusercontent.com/38842891/153817035-8b6d7228-67e3-409d-9859-1f32659a2cec.png)
