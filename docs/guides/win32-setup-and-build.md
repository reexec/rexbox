# Win32 개발 환경 준비와 단독 빌드

근거: [설계 i001](../design/20261020-i001-project-bootstrap.md), [작업 로그 i001](../work-logs/20261020-i001-project-bootstrap.md)

rexbox 단독 빌드는 라이브러리, 도구(`rexbox_xbe_info`, `rexbox_xdk_symbols`), 게임 없이 도는 probe 세 개를 만든다. 게임을 실행하려면 타이틀 프로젝트(reJSRF, reCT3)를 빌드한다.

## 1. 필요한 것

* Windows 10/11
* Visual Studio 2022 또는 2026, "C++를 사용한 데스크톱 개발" 워크로드(x86/x64 MSVC 도구 포함)
* CMake 3.20 이상(Visual Studio 내장 CMake도 가능)
* Git
* 재컴파일 파이프라인을 쓰는 타이틀 프로젝트라면 Python 3.10 이상과 `pip install capstone`

## 2. 저장소 받기

```powershell
git clone --recurse-submodules https://github.com/reexec/rexbox.git
cd rexbox
# submodule 없이 받았다면(xboxrecomp, XbSymbolDatabase, SDL, imgui, smaa):
git submodule update --init
```

## 3. 빌드

```powershell
.\scripts\build_win32_x86.ps1                       # Debug
.\scripts\build_win32_x86.ps1 -Configuration Release
```

PowerShell 실행 정책이 `.ps1`을 막으면 같은 인자로 `scripts\build_win32_x86.bat`을 쓴다. 결과는 `build\win32_x86\<디렉터리>\<구성>\`에 생긴다.

## 4. 확인

```powershell
.\build\win32_x86\tools\Debug\rexbox_kernel_probe.exe   # "83 check(s), 0 failure(s)", exit code 0
.\build\win32_x86\tools\Debug\rexbox_dsound_probe.exe   # "55 check(s), 0 failure(s)", exit code 0
.\build\win32_x86\tools\Debug\rexbox_d3d8_probe.exe     # "163 check(s), 0 failure(s)", exit code 0
ctest --test-dir build\win32_x86 -C Debug --output-on-failure   # the three probes as the unit tests, in one command (the one CI uses)
.\build\win32_x86\tools\Debug\rexbox_xbe_info.exe <어떤 default.xbe>   # 헤더, 인증서, 섹션, 커널 import
python scripts\recomp\xbe_variant.py <어떤 default.xbe>              # MD5와 인증서 신원(타이틀 프로파일용)
```

## 5. 다른 lifter 체크아웃으로 컴파일 확인

소비자 프로젝트가 자기 xboxrecomp를 고정한다면 그 헤더로 `rexbox_recomp`가 컴파일되는지 본다.

```powershell
cmake -S . -B build\abi_check -G "Visual Studio 18 2026" -A Win32 -DREXBOX_XBOXRECOMP_DIR=<그 체크아웃> -DREXBOX_BUILD_TOOLS=OFF
cmake --build build\abi_check --config Debug --target rexbox_recomp
```

---

# Win32 development setup and standalone build

Based on: [design i001](../design/20261020-i001-project-bootstrap.md), [work log i001](../work-logs/20261020-i001-project-bootstrap.md)

A standalone build yields the libraries, the tools (`rexbox_xbe_info`, `rexbox_xdk_symbols`) and the three probes that run without a game; to run a game, build a title project (reJSRF, reCT3).

1. **Requirements**: Windows 10/11; Visual Studio 2022 or 2026 with the "Desktop development with C++" workload; CMake 3.20+; Git; Python 3.10+ with `pip install capstone` if a title project will run the recompilation pipeline.
2. **Get the repository**: `git clone --recurse-submodules https://github.com/reexec/rexbox.git` (or `git submodule update --init` after a plain clone).
3. **Build**: `.\scripts\build_win32_x86.ps1` for Debug, `-Configuration Release` for Release; `scripts\build_win32_x86.bat` takes the same arguments when the execution policy blocks `.ps1` files. Output goes under `build\win32_x86\`.
4. **Check**: `ctest --test-dir build\win32_x86 -C Debug --output-on-failure` runs the three probes as the unit tests (the command CI uses); run by hand, the three probes print `83 check(s), 0 failure(s)`, `55 check(s), 0 failure(s)` and `163 check(s), 0 failure(s)` and exit 0; `rexbox_xbe_info.exe <default.xbe>` prints an XBE's header, certificate, sections and kernel imports; `python scripts\recomp\xbe_variant.py <default.xbe>` prints its MD5 and certificate identity for a title profile.
5. **Compile against another lifter checkout**: configure a second tree with `-DREXBOX_XBOXRECOMP_DIR=<checkout> -DREXBOX_BUILD_TOOLS=OFF` and build the `rexbox_recomp` target.
