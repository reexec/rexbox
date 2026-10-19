# 작업 로그: reJSRF에서 rexbox 분리 (issue #1)

설계: [20261020-i001](../design/20261020-i001-project-bootstrap.md) · 작업 지시: [20261020-i001](../work-orders/20261020-i001-project-bootstrap.md)

## 한 일

1. 이력 추출: reJSRF v0.0.42(`f5939ee`)의 새 clone에 `git filter-repo`로 설계 §2의 경로만 남기고 `src/rexbox/`→`src/`로 옮겼다. 41개 커밋이 남았고 제목의 `(#N)`은 `(nworkers/reJSRF#N)`이 됐다. 추출한 트리가 reJSRF 작업 트리의 같은 경로와 바이트 단위로 같음을 `diff -rq`로 확인했다. `main`으로 push.
2. submodule: SDL `829a65d`, imgui `c1aaac6`, smaa `71c806a`, XbSymbolDatabase `20eced5`는 같은 pin, xboxrecomp는 upstream `v0.15.0`(`6376eb8`)으로 바꾸고 `.gitmodules`의 URL을 upstream으로 고쳤다.
3. CMake: 루트 `CMakeLists.txt`(플랫폼 선택과 부모 내보내기, `REXBOX_XBOXRECOMP_DIR`, `REXBOX_BUILD_TOOLS`, SDL·imgui·smaa·XbSymbolDatabase), `cmake/rexbox_host_flags.cmake`, `cmake/rexbox_license.cmake`(xboxrecomp 체크아웃 기준 상대 경로 가드), `cmake/rexbox_recomp.cmake`(`rexbox_add_recompiled_library`), `src/CMakeLists.txt`·`src/platform/CMakeLists.txt`의 `REXBOX_*` 변수, 타이틀 도구를 뺀 `tools/CMakeLists.txt`.
4. 코드: `platform::SetApplicationIdentity`(`paths.h`, `paths_sdl.cpp`), D3D8 기본 창 제목 `"rexbox"`, `REXBOX_RECOMP_CALL_PROFILE`·`REXBOX_RECOMP_ABI_CHECK` 이름, `recomp_abi.c`에 `g_eflags`(시작값 `0x202`)와 `g_xbox_guest_cpu_preempt`, `recomp_bridge.cpp`에 `xbox_Cpuid`·`xbox_PortIn`·`xbox_PortOut`·`xbox_GuestCpuPreempt`. upstream 헤더의 C4310은 `rexbox_recomp`에서만 억제.
5. 스크립트: `regenerate.py`(`run(...)` 진입점, 경로 인자), `xbe_variant.py`(신원 출력, `md5: null` = 미등록), `check_hle_pops.py`, `find_pointer_targets.py`, `resolve_samples.py`(map 파일 이름에서 실행 파일 이름).
6. 문서: `AGENTS.md`(reJSRF 규칙을 라이브러리 성격에 맞게), `CLAUDE.md`, `README.md`, `ARCHITECTURE.md`, 헌장, 코딩 스타일·kb 손질, 가이드, 고지, 크레딧, `VERSION` 0.0.1, `.gitignore`, CI.

reJSRF 저장소는 읽기만 했고 바꾸지 않았다(사용자 지시). reJSRF의 전환은 설계 §6에 적힌 대로 별도 작업이다.

## 검증

| 검증 | 결과 |
|---|---|
| Win32 x86 Debug / Release 단독 빌드 | 우리 타깃 경고 0, 오류 0 |
| `rexbox_kernel_probe` / `rexbox_dsound_probe` / `rexbox_d3d8_probe` | 56 / 48 / 163 check, 0 failure (Debug·Release) |
| `REXBOX_XBOXRECOMP_DIR`=reJSRF의 fork 체크아웃(`1991221`, 읽기만)으로 `rexbox_recomp` 컴파일 | 통과 |
| 추출 트리 vs reJSRF 작업 트리 | `diff -rq` 차이 없음 |
| 소비자 빌드(reCT3 issue #1) | reCT3가 submodule `633e897`로 Debug·Release 빌드, probe 통과, 앱 smoke 통과 |
| 스크립트 | `py_compile` 통과; `xbe_variant.py`가 실제 XBE의 MD5·인증서 출력 |
| 게임 실행 | 해당 없음(타이틀 프로젝트의 검증). reJSRF가 이 저장소로 전환될 때 reJSRF에서 확인한다 |

## 남은 것

* reJSRF의 전환(설계 §6): 별도 issue.
* 앱의 설정 코드(`settings*.cpp`)는 두 소비자에 사본으로 있다. 이름과 기본 해상도를 매개변수로 만들어 rexbox로 올리는 것은 후속 issue 후보.

---

# Work log: splitting rexbox out of reJSRF (issue #1)

Design: [20261020-i001](../design/20261020-i001-project-bootstrap.md) · Work order: [20261020-i001](../work-orders/20261020-i001-project-bootstrap.md)

## Done

1. History extracted: on a fresh clone of reJSRF v0.0.42 (`f5939ee`), `git filter-repo` kept the paths of design §2 and moved `src/rexbox/` to `src/`; 41 commits remain, with `(#N)` in titles rewritten to `(nworkers/reJSRF#N)`. The extracted tree was compared byte for byte with reJSRF's working tree (`diff -rq`, no differences). Pushed as `main`.
2. Submodules: SDL `829a65d`, imgui `c1aaac6`, smaa `71c806a` and XbSymbolDatabase `20eced5` at the same pins; xboxrecomp moved to upstream `v0.15.0` (`6376eb8`) with the `.gitmodules` URL pointed at upstream.
3. CMake: the root `CMakeLists.txt` (platform selection exported to the parent, `REXBOX_XBOXRECOMP_DIR`, `REXBOX_BUILD_TOOLS`, SDL, imgui, smaa, XbSymbolDatabase), `cmake/rexbox_host_flags.cmake`, `cmake/rexbox_license.cmake` (the guard relative to the xboxrecomp checkout), `cmake/rexbox_recomp.cmake` (`rexbox_add_recompiled_library`), `REXBOX_*` variables in `src/` and `src/platform/`, and `tools/CMakeLists.txt` without the title tools.
4. Code: `platform::SetApplicationIdentity` (`paths.h`, `paths_sdl.cpp`), the D3D8 default window title `"rexbox"`, the `REXBOX_RECOMP_CALL_PROFILE` and `REXBOX_RECOMP_ABI_CHECK` names, `g_eflags` (reset `0x202`) and `g_xbox_guest_cpu_preempt` in `recomp_abi.c`, `xbox_Cpuid`, `xbox_PortIn`, `xbox_PortOut` and `xbox_GuestCpuPreempt` in `recomp_bridge.cpp`; the upstream header's C4310 silenced for `rexbox_recomp` only.
5. Scripts: `regenerate.py` (a `run(...)` entry point with path arguments), `xbe_variant.py` (identity printing; `md5: null` means unregistered), `check_hle_pops.py`, `find_pointer_targets.py`, `resolve_samples.py` (the executable's name from the map file's).
6. Documents: `AGENTS.md` (reJSRF's rules adapted to a library), `CLAUDE.md`, `README.md`, `ARCHITECTURE.md`, the charter, the adjusted coding style and kb, the guide, the notices, the credits, `VERSION` 0.0.1, `.gitignore`, CI.

The reJSRF repository was read only, never changed (user instruction); its migration is the separate task of design §6.

## Verification

| check | result |
|---|---|
| Win32 x86 Debug and Release standalone builds | 0 warnings on our targets, 0 errors |
| `rexbox_kernel_probe` / `rexbox_dsound_probe` / `rexbox_d3d8_probe` | 56 / 48 / 163 checks, 0 failures (Debug and Release) |
| `rexbox_recomp` compiled with `REXBOX_XBOXRECOMP_DIR` at reJSRF's fork checkout (`1991221`, read only) | passes |
| Extracted tree against reJSRF's working tree | `diff -rq`: no differences |
| Consumer build (reCT3 issue #1) | reCT3 builds Debug and Release with the submodule at `633e897`; probes and app smoke pass |
| Scripts | `py_compile` passes; `xbe_variant.py` prints a real XBE's MD5 and certificate |
| Running a game | not applicable (a title project's verification); checked in reJSRF when it moves to this repository |

## Left open

* reJSRF's migration (design §6): a separate issue.
* The app's settings code (`settings*.cpp`) exists as a copy in both consumers; lifting it into rexbox with its names and default resolution as parameters is a follow-up issue candidate.
