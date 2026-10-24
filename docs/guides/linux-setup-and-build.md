# Linux 개발 환경 준비와 단독 빌드

근거: [설계 i004](../design/20261020-i004-linux-preparation.md), [작업 로그 i004](../work-logs/20261020-i004-linux-preparation.md)

Linux 단독 빌드는 Win32와 같이 라이브러리, 도구, probe 네 개를 만든다(게임 실행은 타이틀 프로젝트). Ubuntu 24.04(WSL2 포함)에서 확인했다.

## 1. 필요한 것

```bash
sudo apt-get install -y build-essential cmake ninja-build git \
  libasound2-dev libpulse-dev libx11-dev libxext-dev libxcursor-dev libxi-dev libxrandr-dev libxss-dev \
  libxfixes-dev libxkbcommon-dev libwayland-dev libdecor-0-dev libdrm-dev libgbm-dev libgl-dev libegl-dev
# x86 (-m32) 빌드라면 추가로:
sudo dpkg --add-architecture i386 && sudo apt-get update
sudo apt-get install -y gcc-multilib g++-multilib libasound2-dev:i386 libx11-dev:i386 libxext-dev:i386 libgl-dev:i386  # (CI의 목록은 .github/workflows/ci.yml)
```

## 2. 저장소 받기

```bash
git clone --recurse-submodules https://github.com/reexec/rexbox.git
cd rexbox
```

## 3. 빌드와 테스트

```bash
scripts/build_linux.sh x64 debug      # configure + build + ctest (CMakePresets.json의 linux-x64-debug)
scripts/build_linux.sh x64 release
scripts/build_linux.sh x86 debug      # -m32
# 또는 직접:
cmake --preset linux-x64-debug && cmake --build --preset linux-x64-debug && ctest --preset linux-x64-debug
```

결과는 `build/linux-<arch>-<config>/`에 생기고, probe는 `build/linux-x64-debug/tools/rexbox_*_probe`다. x86 빌드는 `rexbox_apply_host_flags()`가 `-msse2 -mfpmath=sse`를 걸어 x87 확장 정밀도 없이 돈다(설계 i004 §3).

## 4. 아직 없는 것

* 샘플링 프로파일러(`REXBOX_SAMPLE`)는 Linux에서 거부된다.
* soft-dirty 페이지 추적이 없는 커널(WSL2)에서는 `TakeWrittenPages`가 실패를 돌려주어 D3D8이 버퍼를 모두 비교한다(정확성은 같다).

---

# Linux development setup and standalone build

Based on: [design i004](../design/20261020-i004-linux-preparation.md), [work log i004](../work-logs/20261020-i004-linux-preparation.md)

A Linux standalone build yields the libraries, the tools and the four probes, as on Win32 (running a game is a title project's business); verified on Ubuntu 24.04 (WSL2 included).

1. **Requirements**: the packages in the first block above (build tools, CMake, Ninja, and the development libraries SDL3 compiles against); for an x86 (`-m32`) build also `gcc-multilib`, `g++-multilib` and the `:i386` development packages after `dpkg --add-architecture i386` (the CI's list is in `.github/workflows/ci.yml`).
2. **Get the repository**: `git clone --recurse-submodules https://github.com/reexec/rexbox.git`.
3. **Build and test**: `scripts/build_linux.sh <x64|x86> <debug|release>` configures, builds and runs `ctest` through the presets in `CMakePresets.json` (`linux-<arch>-<config>`); output goes to `build/linux-<arch>-<config>/`. x86 builds run with `-msse2 -mfpmath=sse` from `rexbox_apply_host_flags()`, so no x87 excess precision (design i004 §3).
4. **Not there yet**: the sampling profiler (`REXBOX_SAMPLE`) is refused on Linux; on a kernel without soft-dirty page tracking (WSL2) `TakeWrittenPages` reports failure and D3D8 compares whole buffers (same correctness).
