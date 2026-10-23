# 작업 로그: xboxrecomp 의존 제거 (issue #26)

작업 지시: [20261024-i026-drop-xboxrecomp](../work-orders/20261024-i026-drop-xboxrecomp.md) · 설계: [20261024-i026-drop-xboxrecomp](../design/20261024-i026-drop-xboxrecomp.md) · 소비자: nworkers/reJSRF#117, reexec/reCT3#22

## 한 일

1. **기준 출력.** 바꾸기 전의 두 타이틀(reJSRF는 rexbox v0.0.9, reCT3는 v0.0.10 submodule)로 세 변형(jsrf_us, jsrf_jp, ct3_jp)을 재생성했다. 같은 출력에서 `check_hle_pops --all`(세 변형)과 `find_pointer_targets`(jsrf_us, ct3_jp)의 출력도 받아 두었다. 두 버전 사이에 `scripts/`, `cmake/`, `src/recomp/`는 같고, HLE의 `AddExport`·`Return` 줄도 바뀌지 않았다.
2. **ABI 껍데기(설계 §3.1).** `recomp_bridge.cpp`의 `extern "C"` 콜백 19개를 rexbox 이름 `rexbox_bridge_*`로 바꾸었다. 선언은 새 내부 헤더 `src/recomp/recomp_callbacks.h`에 두었다. `recomp_abi.c`에는 lifter 이름의 껍데기(`recomp_lookup_manual` … `recomp_guest_longjmp`)를 두어 그 함수로 넘긴다. `rexbox_hle_enter`는 브리지에 남았다. 설계의 "`recomp_bridge.h`에 선언"은 C++ 전용 헤더라 별도 C 헤더로 바꾸었고, 설계 문서도 그에 맞췄다.
3. **CMake(설계 §3.1, §3.3).**
   * `rexbox_recomp`에는 `recomp_bridge.cpp`와 `recomp_runtime.cpp`만 남았다.
   * `rexbox_add_recompiled_library`는 `recomp_abi.c`를 OBJECT 타깃 `<target>_abi`로 컴파일한다. include는 `GEN_DIR`과 `src/recomp`, 경고는 `rexbox_strict_warnings`에 `/wd4310`을 더한 것이다. 그 오브젝트는 생성 코드와 같은 아카이브에 들어간다.
   * `GEN_DIR/recomp_types.h`가 없으면 configure가 실패한다.
   * `RECOMP_XMM_PORTABLE`은 생성 코드 타깃에 PUBLIC으로 붙어 타이틀로 전파된다. `RECOMP_CALL_PROFILE`과 `RECOMP_ABI_CHECK`는 `<target>_abi`에도 붙어, 그 빌드에서 해당 콜백 선언이 확인된다.
   * 해시 검사, `src/kernel` include, `REXBOX_XBOXRECOMP_DIR`, `cmake/rexbox_license.cmake`(`rexbox_check_license`)를 지웠다.
4. **링크 순환(설계 §7).** 별도 순환 선언은 필요 없었다. `recomp_abi.c`의 오브젝트가 생성 코드 아카이브 안에 있고, 생성 코드는 반드시 `g_eax` 같은 상태를 참조한다. 그래서 링커가 그 오브젝트를 `rexbox_recomp`보다 먼저 끌어오고, `rexbox_recomp`가 부르는 레지스터 접근 함수는 이미 정의되어 있다. GNU ld에서 아래 링크 시험으로 확인했다.
5. **스크립트(설계 §3.2).** `hle_overrides.py`, `hle_pops.py`, `pointer_targets.py`와 각 `test_*.py`(합성 입력 6개)를 더하고 `regenerate.py`, `check_hle_pops.py`, `find_pointer_targets.py`를 지웠다. 생성되는 `hle_overrides.c`의 머리 주석 두 줄만 바뀌었다(스크립트 이름, `sub_X_gen` → "the lifter's name").
6. **submodule.** `third_party/xboxrecomp`와 `.gitmodules` 항목을 지웠다. CI(`ci.yml`, `release.yml`)의 Win32 작업에 Python 테스트 단계를 더했다.
7. **문서(설계 §3.4, §3.5).** `ARCHITECTURE.md`, `README.md`, `AGENTS.md`(레이어·라이선스 규칙), `docs/PROJECT_CHARTER.md`(단계 3), `THIRD_PARTY_NOTICES.md`, `docs/guides/win32-setup-and-build.md`(§5), `docs/kb/external-references.md`, 공개 헤더 주석 두 곳을 고쳤다. `docs/guides/linux-setup-and-build.md`와 `docs/kb/static-recompilation-and-hle.md`는 고칠 내용이 없었다.
8. **소비자 이식.** 두 타이틀에 issue(reJSRF #117, reCT3 #22)와 브랜치(`i117-lifter-pipeline`, `i022-lifter-pipeline`)를 만들고 rexbox submodule을 이 브랜치로 두었다. 고친 것:
   * 공통 `scripts/recomp/lifter.py`: xboxrecomp 다섯 단계, `functions.json`, `/* ret N */`, `recomp_dispatch.c` 읽기, `sub_%08X` 이름 규칙.
   * `regenerate.py`, `check_hle_pops.py`, `find_pointer_targets.py` wrapper.
   * CMake: `REXBOX_XBOXRECOMP_DIR`와 `rexbox_check_license` 호출을 지웠다(reJSRF는 `tools/`의 두 곳 포함). 라이선스 가드는 타이틀로 옮기지 않았다. 타이틀 타깃은 생성 코드와 그 옆의 MIT 헤더만 컴파일하므로, 각 타이틀의 규칙이 "xboxrecomp의 LGPL 파일을 소스 목록에 넣지 않는다"로 바뀌었다.
9. **사고와 복구.** rexbox의 `third_party/xboxrecomp` 작업 사본을 지우자 reCT3의 `.git/modules/third_party/xboxrecomp`가 깨졌다. 그 저장소가 `objects/info/alternates`로 지운 사본의 객체를 빌려 쓰고 있었기 때문이다. 복구 방법: 같은 커밋(`6376eb8`)을 가진 로컬 클론 `E:/MYWORK/Projects/xboxrecomp`를 alternates로 잠시 가리키고, `git repack -a -d`로 객체를 자기 저장소로 옮긴 뒤 alternates를 지웠다. `fsck --connectivity-only`가 깨끗하다. 다른 alternates는 reCT3의 rexbox submodule → `E:/MYWORK/Projects/rexbox/.git/objects` 하나이며 건드리지 않았다.

## 검증

| 검증 | 결과 |
|---|---|
| rexbox 단독, xboxrecomp 없는 새 configure(`build/i026_fresh`), Win32 x86 Debug | 빌드 성공, 경고 0, CTest 3/3 |
| rexbox Win32 x86 Release(`build/win32_x86`) | 성공, 경고 0 |
| `python -m unittest discover -s scripts/recomp -p "test_*.py"` | 6/6 |
| 재생성 비교: 이식한 파이프라인 대 기준(같은 xboxrecomp, 같은 XBE) | ct3_jp 45개 같음, jsrf_us·jsrf_jp 각 61개 같음. 나머지 5개는 예상한 차이뿐이다(override 머리 주석, 출력 폴더 경로, disasm 캐시의 시각). lift된 C(`gen/recomp_*.c`, `recomp_dispatch.c`, `recomp_types.h`)는 바이트 단위로 같다 |
| `check_hle_pops --all`(세 변형), `find_pointer_targets`(jsrf_us, ct3_jp) | 기준과 바이트 단위로 같다(불일치 0, 후보 0과 6) |
| reCT3 Win32 x86 Release(upstream v0.15.0 헤더) | `rect3_recomp_abi`의 `recomp_abi.c` /W4 경고 0, `rect3.exe` 링크 |
| reCT3 `drive_west_coast.ps1`(100초, 숨긴 창, `REXBOX_D3D8_PROFILE=120`) | West Coast 운전 도달: 프레임당 그리기 약 790, 59 fps. 정지·fault·ABI 보고 없음 |
| reJSRF Win32 x86 Release(fork 헤더) | `rejsrf_recomp_abi`의 `recomp_abi.c` 경고 0, `rejsrf.exe`·검사 도구 링크 |
| `rejsrf_decomp_check` / `rejsrf_d3d8_check` | 129 case 0 difference / 14018 case 0 difference |
| reJSRF 실행(숨긴 창, 40초 watchdog) | 부팅, 경고 화면(frame 2000 덤프), 프로그램 셰이더 그리기가 이어지는 프레임 루프. watchdog로 종료(exit 4). 정지·fault 없음 |
| GNU ld 링크(WSL Ubuntu 24.04): 합성 타이틀(생성 함수 하나 + upstream `recomp_types.h`), `RecompExecutor::Cpu().Get(eax)` | x86(`-m32`), x64 모두 링크·실행 `eax 1234` |

검증하지 못한 것:

* reJSRF는 입력을 넣는 실행 스크립트가 없어 게임플레이까지 가지 않았다. 부팅과 프레임 루프, 그리고 lift된 원본을 실제로 실행하는 두 검사 도구로 확인했다.
* 두 타이틀은 아직 Linux 빌드가 없다. 그래서 GNU ld 확인은 실제 타이틀이 아니라 합성 타이틀로 했다.

---

# Work log: removing the xboxrecomp dependency (issue #26)

Work order: [20261024-i026-drop-xboxrecomp](../work-orders/20261024-i026-drop-xboxrecomp.md) · Design: [20261024-i026-drop-xboxrecomp](../design/20261024-i026-drop-xboxrecomp.md) · Consumers: nworkers/reJSRF#117, reexec/reCT3#22

## Done

1. **Baseline.** Before any change, regenerated three variants (jsrf_us, jsrf_jp, ct3_jp) with both titles as they were (reJSRF on rexbox v0.0.9, reCT3 on v0.0.10), and kept the output of `check_hle_pops --all` (three variants) and `find_pointer_targets` (jsrf_us, ct3_jp) on it. `scripts/`, `cmake/` and `src/recomp/` are the same in both versions, and no HLE `AddExport` or `Return` line differs.
2. **ABI shells (design 3.1).** The 19 `extern "C"` callbacks in `recomp_bridge.cpp` became `rexbox_bridge_*`, declared in the new internal header `src/recomp/recomp_callbacks.h`. `recomp_abi.c` holds the shells under the lifter's names (`recomp_lookup_manual` … `recomp_guest_longjmp`) that forward to them. `rexbox_hle_enter` stays in the bridge. The design said "declared in `recomp_bridge.h`", but that header is C++-only, so a separate C header was used and the design updated.
3. **CMake (design 3.1, 3.3).**
   * `rexbox_recomp` keeps only `recomp_bridge.cpp` and `recomp_runtime.cpp`.
   * `rexbox_add_recompiled_library` compiles `recomp_abi.c` as the OBJECT target `<target>_abi`, with `GEN_DIR` and `src/recomp` on the include path and `rexbox_strict_warnings` plus `/wd4310`. Its object goes into the generated code's archive.
   * Configure fails without `GEN_DIR/recomp_types.h`.
   * `RECOMP_XMM_PORTABLE` is PUBLIC on the generated-code target and so reaches the title. `RECOMP_CALL_PROFILE` and `RECOMP_ABI_CHECK` also go on `<target>_abi`, so those builds check the matching callback declarations.
   * Gone: the hash check, the `src/kernel` include, `REXBOX_XBOXRECOMP_DIR` and `cmake/rexbox_license.cmake` (`rexbox_check_license`).
4. **Link cycle (design 7).** No extra cycle declaration was needed. `recomp_abi.c`'s object sits in the generated code's archive, and generated code always references its state (`g_eax`, ...). So the linker pulls that object before it reaches `rexbox_recomp`, and the register accessors `rexbox_recomp` calls are already defined. The GNU ld link check below confirms this.
5. **Scripts (design 3.2).** Added `hle_overrides.py`, `hle_pops.py` and `pointer_targets.py`, with `test_*.py` for each (6 synthetic tests). Deleted `regenerate.py`, `check_hle_pops.py` and `find_pointer_targets.py`. Only the two header comment lines of the generated `hle_overrides.c` changed: the script name, and `sub_X_gen` became "the lifter's name".
6. **Submodule.** Removed `third_party/xboxrecomp` and its `.gitmodules` entry. Added a Python test step to the Win32 jobs of CI (`ci.yml`, `release.yml`).
7. **Documents (design 3.4, 3.5).** Updated `ARCHITECTURE.md`, `README.md`, `AGENTS.md` (layer and license rules), `docs/PROJECT_CHARTER.md` (stage 3), `THIRD_PARTY_NOTICES.md`, `docs/guides/win32-setup-and-build.md` (section 5), `docs/kb/external-references.md` and two public header comments. `docs/guides/linux-setup-and-build.md` and `docs/kb/static-recompilation-and-hle.md` needed nothing.
8. **Consumer ports.** Both titles got an issue (reJSRF #117, reCT3 #22) and a branch (`i117-lifter-pipeline`, `i022-lifter-pipeline`) with their rexbox submodule on this branch. Changes:
   * A shared `scripts/recomp/lifter.py`: the five xboxrecomp steps, reading `functions.json`, `/* ret N */` and `recomp_dispatch.c`, and the `sub_%08X` naming.
   * The `regenerate.py`, `check_hle_pops.py` and `find_pointer_targets.py` wrappers.
   * CMake: no `REXBOX_XBOXRECOMP_DIR`, no `rexbox_check_license` calls (reJSRF also had two in `tools/`). The license guard was not moved into the titles: their targets compile only the generated code and the MIT header next to it, so each title's rule became "xboxrecomp's LGPL files never go into a source list".
9. **Incident and repair.** Deleting rexbox's `third_party/xboxrecomp` checkout broke reCT3's `.git/modules/third_party/xboxrecomp`, which borrowed that checkout's objects through `objects/info/alternates`. The repair: point the alternates for a moment at the local clone `E:/MYWORK/Projects/xboxrecomp`, which has the same commit (`6376eb8`); `git repack -a -d` to copy the objects into the repository itself; then delete the alternates. `fsck --connectivity-only` is clean. The only other alternates (reCT3's rexbox submodule → `E:/MYWORK/Projects/rexbox/.git/objects`) were left as they are.

## Verification

See the table above. Summary:
* rexbox standalone, from a fresh configure with no xboxrecomp (Win32 x86 Debug): builds with 0 warnings, CTest 3/3. The Release build also has 0 warnings. Python tests 6/6.
* The moved pipeline against the baseline: the lifted C is byte-identical for all three variants, and the only other differences are the expected ones (override header comments, output root paths, disassembler-cache timing). The `check_hle_pops` and `find_pointer_targets` output is byte-identical.
* reCT3 (upstream v0.15.0 header): links, and reaches West Coast driving with no stop (about 790 draws per frame, 59 fps).
* reJSRF (fork header): links. `rejsrf_decomp_check` gives 129 cases with 0 differences, `rejsrf_d3d8_check` 14018 cases with 0 differences, and a 40 s hidden run boots and keeps its frame loop going.
* GNU ld (WSL Ubuntu 24.04), with a synthetic title (one generated function and the upstream header): x86 (`-m32`) and x64 both link and read `eax 1234` back through `RecompExecutor`.

Not verified:
* reJSRF gameplay: it has no input-driving script.
* A GNU ld link of a real title: neither title builds on Linux yet.
