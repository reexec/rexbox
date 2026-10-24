# 서드파티 고지 / Third-Party Notices

rexbox는 MIT 라이선스다. 아래 서드파티 코드가 저장소에 포함되거나 옮겨져 있다.

rexbox is MIT licensed. The third-party code below is included in, or ported into, the repository.

## xboxrecomp

* 출처 / Source: https://github.com/sp00nznet/xboxrecomp
* 라이선스 / License: MIT (Copyright (c) 2026 sp00nz). 전문은 아래. / Full text below.
* 포함 방식 / How included: 저장소에 xboxrecomp는 없다. lifter는 타이틀 프로젝트가 고정하고, rexbox는 아래의 옮긴 코드만 가진다([설계 i026](docs/design/20261024-i026-drop-xboxrecomp.md)). / xboxrecomp is not in the repository: the title projects pin the lifter, and rexbox holds only the ported code below ([design i026](docs/design/20261024-i026-drop-xboxrecomp.md)).
* 옮긴 코드 / Ported code:
  * `src/hle/kernel/kernel_exports.cpp` — `tools/xbe_parser/xbe_parser.py`의 `KERNEL_EXPORTS` 표 / the `KERNEL_EXPORTS` table of `tools/xbe_parser/xbe_parser.py`.
  * `src/recomp/recomp_bridge.cpp`의 `rexbox_bridge_cpuid` — `src/kernel/kernel_hal.c`가 보고하는 Pentium III cpuid 값과 같은 값(하드웨어 사실; Intel SDM) / the same Pentium III cpuid values `src/kernel/kernel_hal.c` reports (hardware facts; Intel SDM).
  * `src/recomp/recomp_abi.c` — `templates/runtime/recomp_types.h`가 선언하는 생성 코드 ABI(전역 상태와 콜백의 이름·시그니처)의 구현. 헤더 자체는 가지지 않으며, 타이틀의 생성 코드와 함께 온 헤더로 컴파일된다. / the implementation of the generated-code ABI (the names and signatures of the global state and callbacks) that `templates/runtime/recomp_types.h` declares. The header itself is not held; the file compiles against the one that comes with a title's generated code.

```text
MIT License

Copyright (c) 2026 sp00nz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## XbSymbolDatabase

* 출처 / Source: https://github.com/Cxbx-Reloaded/XbSymbolDatabase
* 라이선스 / License: MIT (Copyright (c) 2023 The XbSymbolDatabase authors). 전문은 `third_party/XbSymbolDatabase/LICENSE`. / Full text in `third_party/XbSymbolDatabase/LICENSE`.
* 포함 방식 / How included: git submodule `third_party/XbSymbolDatabase` (`20eced544726f5558c5a408458f38a086cc4e543`). 라이브러리 `libXbSymbolDatabase`만 빌드해 도구 `rexbox_xdk_symbols`에 링크한다(`REXBOX_BUILD_TOOLS`). 게임 실행 파일에는 들어가지 않는다. / Only the `libXbSymbolDatabase` library is built, linked into the `rexbox_xdk_symbols` tool (`REXBOX_BUILD_TOOLS`); it does not enter a game executable.

## SDL3

* 출처 / Source: https://github.com/libsdl-org/SDL
* 라이선스 / License: zlib (Copyright (C) 1997-2026 Sam Lantinga). 전문은 `third_party/SDL/LICENSE.txt`. / Full text in `third_party/SDL/LICENSE.txt`.
* 포함 방식 / How included: git submodule `third_party/SDL`, `release-3.4.18`. 정적 라이브러리 `SDL3-static`만 빌드해 `rexbox_platform`에 링크한다(표시·입력·오디오 계약의 백엔드 `src/platform/sdl/`). 카메라, GPU, 2D 렌더러, 햅틱, 센서, 대화상자, 트레이 하위 시스템과 테스트·예제·설치는 끈다. / Only the static library `SDL3-static` is built and linked into `rexbox_platform` (the backend of the display, input and audio contracts, `src/platform/sdl/`); the camera, GPU, 2D renderer, haptic, sensor, dialog and tray subsystems, tests, examples and installation are off.

```text
This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
```

## Dear ImGui

* 출처 / Source: https://github.com/ocornut/imgui
* 라이선스 / License: MIT (Copyright (c) 2014-2026 Omar Cornut). 전문은 `third_party/imgui/LICENSE.txt`. / Full text in `third_party/imgui/LICENSE.txt`.
* 포함 방식 / How included: git submodule `third_party/imgui`, `v1.93.0` (`c1aaac65e`). 코어(`imgui.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`)는 정적 라이브러리 `imgui`로 빌드해 소비자의 앱(설정 메뉴)과 `rexbox_platform`에 링크한다. SDL3·OpenGL3 백엔드(`backends/imgui_impl_sdl3.cpp`, `backends/imgui_impl_opengl3.cpp`)는 `rexbox_platform` 안에서 컴파일한다. / The core is built as the static library `imgui`, linked into a consumer's app (its settings menu) and `rexbox_platform`; the SDL3 and OpenGL3 backends compile inside `rexbox_platform`.

```text
The MIT License (MIT)

Copyright (c) 2014-2026 Omar Cornut

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## SMAA

* 출처 / Source: https://github.com/iryoku/smaa
* 라이선스 / License: MIT (Copyright (C) 2013 Jorge Jimenez, Jose I. Echevarria, Belen Masia, Fernando Navarro, Diego Gutierrez). 전문은 `third_party/smaa/LICENSE.txt`. / Full text in `third_party/smaa/LICENSE.txt`.
* 포함 방식 / How included: git submodule `third_party/smaa` (`71c806a`). `SMAA.hlsl`은 빌드 때 바이트 배열로 `rexbox_hle_d3d8`에 들어가 GLSL 3 셰이더로 컴파일되고, `Textures/AreaTex.h`, `Textures/SearchTex.h`는 `d3d8_smaa.cpp`가 그대로 include한다. / `SMAA.hlsl` is embedded in `rexbox_hle_d3d8` as bytes at build time and compiled as GLSL 3 shaders; `d3d8_smaa.cpp` includes `Textures/AreaTex.h` and `Textures/SearchTex.h` as they are.

```text
Copyright (C) 2013 Jorge Jimenez (jorge@iryoku.com)
Copyright (C) 2013 Jose I. Echevarria (joseignacioechevarria@gmail.com)
Copyright (C) 2013 Belen Masia (bmasia@unizar.es)
Copyright (C) 2013 Fernando Navarro (fernandn@microsoft.com)
Copyright (C) 2013 Diego Gutierrez (diegog@unizar.es)

Permission is hereby granted, free of charge, to any person obtaining a copy
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software. As clarification, there is no
requirement that the copyright notice and permission be included in binary
distributions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
```
