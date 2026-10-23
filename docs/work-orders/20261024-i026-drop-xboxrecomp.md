# 작업 지시: xboxrecomp 의존 제거 (issue #26)

설계: [20261024-i026-drop-xboxrecomp](../design/20261024-i026-drop-xboxrecomp.md)

1. 작업 브랜치 `i026-drop-xboxrecomp`(`main`에서). 커밋 제목 끝에 `(#26)`.
2. **기준 출력 확보(구현 전).** 소비자 reJSRF와 reCT3에서 지금의 rexbox로 재생성한다. `gen/<variant>/`(특히 `gen/`, `hle_overrides.c`)와 `check_hle_pops`, `find_pointer_targets`의 출력을 scratch에 보관한다. 이것이 6단계 비교의 기준이다.
3. **CMake(설계 §3.1, §3.3).**
   * `src/CMakeLists.txt`: `rexbox_recomp`에서 `recomp_abi.c`, 헤더 include 경로, `/wd4310`, `RECOMP_XMM_PORTABLE`를 뺀다.
   * `cmake/rexbox_recomp.cmake`: `recomp_abi.c`를 `GEN_DIR` include로 컴파일한다(엄격 경고 + `/wd4310`). `RECOMP_XMM_PORTABLE`를 생성 코드 쪽에 붙인다. 해시 검사와 `src/kernel` include를 지운다. `GEN_DIR/recomp_types.h`가 없으면 실패하게 한다. `rexbox_recomp`와의 링크 순환을 선언한다.
   * `src/recomp/`: `recomp_bridge.cpp`의 lifter 콜백 `extern "C"` 껍데기를 `recomp_abi.c`로 옮긴다. 본체는 `recomp_callbacks.h`에 선언한 `rexbox_bridge_*` 함수로 남긴다. `rexbox_hle_enter`는 그대로 둔다(설계 §3.1).
   * `CMakeLists.txt`: `REXBOX_XBOXRECOMP_DIR`와 그 검사를 지운다. `include(rexbox_license)`와 `rexbox_check_license` 호출을 지우고, `cmake/rexbox_license.cmake`를 삭제한다.
4. **스크립트(설계 §3.2).** `scripts/recomp/`에 `hle_overrides.py`, `hle_pops.py`, `pointer_targets.py`를 만들어 각 함수를 옮긴다(평범한 데이터 인자, 이름 규칙은 인자). `regenerate.py`, `check_hle_pops.py`, `find_pointer_targets.py`를 지운다. `test_hle_overrides.py`, `test_hle_pops.py`, `test_pointer_targets.py`(합성 입력)를 더한다.
5. **submodule과 CI(설계 §3.4).** `git rm third_party/xboxrecomp`, `.gitmodules` 정리. `ci.yml`과 `release.yml`의 주석을 고치고 Python 테스트 단계를 더한다.
6. **검증(설계 §6).**
   * rexbox: xboxrecomp 없는 clone에서 Win32 x86 Debug/Release 빌드, CTest, Linux 프리셋 빌드, Python 테스트.
   * 소비자 두 타이틀: 로컬에서 rexbox submodule을 이 브랜치로 두고 파이프라인을 이식한다(이식 코드는 소비자 issue에서 커밋한다). 재생성 출력을 2단계 기준과 바이트 비교하고, wrapper 출력을 비교한다. 앱을 링크하고(MSVC, GNU ld) 이전 진행 지점까지 실행한다.
7. **문서(설계 §3.4, §3.5).** `ARCHITECTURE.md`, `README.md`, `AGENTS.md`(레이어·라이선스 규칙), `docs/PROJECT_CHARTER.md`(단계 3), `THIRD_PARTY_NOTICES.md`, `docs/guides/win32-setup-and-build.md`, `docs/guides/linux-setup-and-build.md`, 필요하면 `docs/kb/static-recompilation-and-hle.md`를 고친다.
8. 소비자 issue(reJSRF, reCT3)를 만들어 #26에 링크한다. 각 issue 본문에 설계 §4의 할 일 표를 옮긴다.
9. 작업 로그 `docs/work-logs/20261024-i026-drop-xboxrecomp.md`. 3단계 링크 순환의 결정과 검증한 타이틀을 적는다. 머지할 때 릴리스 노트에 설계 §4의 소비자 할 일을 적는다.

---

# Work order: removing the xboxrecomp dependency (issue #26)

Design: [20261024-i026-drop-xboxrecomp](../design/20261024-i026-drop-xboxrecomp.md)

1. Task branch `i026-drop-xboxrecomp` (from `main`); commit titles end with `(#26)`.
2. **Capture a baseline (before implementing).** In the consumers reJSRF and reCT3, regenerate with the current rexbox and keep `gen/<variant>/` (especially `gen/` and `hle_overrides.c`) and the output of `check_hle_pops` and `find_pointer_targets` in scratch. Step 6 compares against this.
3. **CMake (design 3.1, 3.3).**
   * `src/CMakeLists.txt`: remove from `rexbox_recomp` the file `recomp_abi.c`, the header include path, `/wd4310` and `RECOMP_XMM_PORTABLE`.
   * `cmake/rexbox_recomp.cmake`: compile `recomp_abi.c` with `GEN_DIR` on the include path (strict warnings plus `/wd4310`). Put `RECOMP_XMM_PORTABLE` on the generated-code side. Drop the hash check and the `src/kernel` include. Fail without `GEN_DIR/recomp_types.h`. Declare the link cycle with `rexbox_recomp`.
   * `src/recomp/`: move the `extern "C"` shells of the lifter callbacks from `recomp_bridge.cpp` to `recomp_abi.c`. Their bodies stay as the `rexbox_bridge_*` functions declared in `recomp_callbacks.h`. `rexbox_hle_enter` stays where it is (design 3.1).
   * `CMakeLists.txt`: drop `REXBOX_XBOXRECOMP_DIR` and its check. Drop `include(rexbox_license)` and the `rexbox_check_license` calls, and delete `cmake/rexbox_license.cmake`.
4. **Scripts (design 3.2).** Add `hle_overrides.py`, `hle_pops.py` and `pointer_targets.py` under `scripts/recomp/` and move the functions there (plain-data arguments; naming passed in as arguments). Delete `regenerate.py`, `check_hle_pops.py` and `find_pointer_targets.py`. Add `test_hle_overrides.py`, `test_hle_pops.py` and `test_pointer_targets.py` (synthetic inputs).
5. **Submodule and CI (design 3.4).** `git rm third_party/xboxrecomp` and clean up `.gitmodules`. Fix the comments in `ci.yml` and `release.yml` and add a Python test step.
6. **Verify (design 6).**
   * rexbox: from a clone without xboxrecomp, the Win32 x86 Debug/Release builds, CTest, the Linux preset builds, and the Python tests.
   * Both consumer titles: with the rexbox submodule on this branch locally, port the pipeline (the port is committed under the consumer issue). Compare the regenerated output byte for byte with the step 2 baseline, and compare the wrapper output. Link the application (MSVC, GNU ld) and run it to the previous point.
7. **Documents (design 3.4, 3.5).** Update `ARCHITECTURE.md`, `README.md`, `AGENTS.md` (layer and license rules), `docs/PROJECT_CHARTER.md` (stage 3), `THIRD_PARTY_NOTICES.md`, `docs/guides/win32-setup-and-build.md`, `docs/guides/linux-setup-and-build.md`, and `docs/kb/static-recompilation-and-hle.md` if needed.
8. Open consumer issues (reJSRF, reCT3) linked to #26, each carrying the work table of design section 4.
9. Work log `docs/work-logs/20261024-i026-drop-xboxrecomp.md`, recording the step 3 link-cycle decision and which titles verified the change. At merge time, the release notes carry the consumer work of design section 4.
