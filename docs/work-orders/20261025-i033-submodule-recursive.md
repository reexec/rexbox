# 작업 지시: submodule 누락 메시지에 `--recursive` (issue #33)

소비자: nworkers/reJSRF#121. 설계 변경은 없다.

`CMakeLists.txt`는 SDL, imgui, smaa, XbSymbolDatabase가 체크아웃되어 있지 않으면 `third_party/<이름> is missing; run: git submodule update --init`으로 멈춘다. 소비자 프로젝트(reJSRF, reCT3)에서는 rexbox 자체가 submodule이라 사용자는 이미 최상위에서 그 명령을 실행했고, 그 명령은 rexbox 안의 중첩 submodule을 초기화하지 않는다. 단독 빌드와 중첩 체크아웃 모두에서 동작하는 명령은 `git submodule update --init --recursive`다.

1. 작업 브랜치 `i033-submodule-recursive`. 커밋 제목 끝에 `(#33)`.
2. `CMakeLists.txt`의 네 메시지가 `git submodule update --init --recursive`를 안내한다.
3. `docs/guides/win32-setup-and-build.md`의 §2와 영문 절차 2도 같은 명령으로 고친다.
4. 검증: 소비자 reJSRF의 Win32 x86 Release 구성이 이 CMakeLists.txt로 통과한다.
5. 작업 로그 `docs/work-logs/20261025-i033-submodule-recursive.md`.

---

# Work order: `--recursive` in the submodule-missing messages (issue #33)

Consumer: nworkers/reJSRF#121. No design change.

`CMakeLists.txt` stops with `third_party/<name> is missing; run: git submodule update --init` when SDL, imgui, smaa or XbSymbolDatabase is not checked out. In a consumer project (reJSRF, reCT3) rexbox is itself a submodule, so the user has already run exactly that command at the top level, and it did not initialize rexbox's nested submodules. The command that works in both the standalone and the nested case is `git submodule update --init --recursive`.

(1) Task branch `i033-submodule-recursive`; commit titles end with `(#33)`. (2) The four messages in `CMakeLists.txt` name `git submodule update --init --recursive`. (3) Section 2 and English step 2 of `docs/guides/win32-setup-and-build.md` name the same command. (4) Verify: the consumer reJSRF's Win32 x86 Release configure passes with this `CMakeLists.txt`. (5) Work log `docs/work-logs/20261025-i033-submodule-recursive.md`.
