# 작업 로그: submodule 누락 메시지에 `--recursive` (issue #33)

작업 지시: [20261025-i033](../work-orders/20261025-i033-submodule-recursive.md) · 소비자: nworkers/reJSRF#121

## 한 일

* `CMakeLists.txt`: SDL, imgui, smaa, XbSymbolDatabase의 누락 메시지 네 개가 `git submodule update --init --recursive`를 안내한다. 단독 체크아웃에서는 `--recursive`가 아무 일도 더 하지 않고, 소비자 프로젝트의 중첩 체크아웃에서는 그것이 실제로 필요한 명령이다.
* `docs/guides/win32-setup-and-build.md` §2와 영문 절차 2: 같은 명령.
* 소비자 reJSRF는 자기 빌드 스크립트에서 CMake 전에 체크아웃되지 않은 submodule을 검사한다(nworkers/reJSRF#121). rexbox의 메시지는 CMake까지 온 경우의 마지막 안내다.

## 검증

| 검증 | 결과 |
|---|---|
| 소비자 reJSRF의 Win32 x86 Release 전체 빌드(이 `CMakeLists.txt`, 네 submodule 모두 체크아웃) | 구성과 빌드 완료, 경고 0 |
| reJSRF에서 `third_party/smaa`를 deinit한 상태 | reJSRF의 빌드 스크립트가 CMake 전에 같은 명령으로 멈춤(nworkers/reJSRF#121). rexbox의 메시지 경로 자체는 문구만 바뀌었고 조건은 그대로다 |

---

# Work log: `--recursive` in the submodule-missing messages (issue #33)

Work order: [20261025-i033](../work-orders/20261025-i033-submodule-recursive.md) · Consumer: nworkers/reJSRF#121

Done: the four missing-submodule messages in `CMakeLists.txt` (SDL, imgui, smaa, XbSymbolDatabase) name `git submodule update --init --recursive`; in a standalone checkout `--recursive` does nothing more, and in a consumer's nested checkout it is the command that is actually needed. Section 2 and English step 2 of `docs/guides/win32-setup-and-build.md` name the same command. The consumer reJSRF checks for submodules that are not checked out in its own build scripts before CMake runs (nworkers/reJSRF#121); rexbox's message is the last word for a configure that gets that far.

Verification: the consumer reJSRF's full Win32 x86 Release build with this `CMakeLists.txt` and all four submodules checked out configures and builds with 0 warnings. With `third_party/smaa` deinit-ed in reJSRF, reJSRF's build script stops before CMake with the same command; the message path in rexbox changed only in wording, its condition is unchanged.
