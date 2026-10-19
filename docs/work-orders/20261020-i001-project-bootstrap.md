# 작업 지시: reJSRF에서 rexbox 분리 (issue #1)

설계: [20261020-i001](../design/20261020-i001-project-bootstrap.md)

1. 이력 추출: reJSRF(v0.0.42, `f5939ee`)를 새로 clone해 `git filter-repo`로 설계 §2의 경로만 남기고 `src/rexbox/`를 `src/`로 옮긴다. 커밋 제목의 issue 번호를 `nworkers/reJSRF#N`으로 바꾼다. 결과를 `main`으로 두고 원격 `git@github.com:reexec/rexbox.git`을 단다.
2. submodule: SDL, imgui, smaa, XbSymbolDatabase는 같은 pin, xboxrecomp는 upstream v0.15.0으로 바꾼다.
3. 작업 브랜치 `i001-project-bootstrap`에서 루트 `CMakeLists.txt`, `cmake/rexbox_host_flags.cmake`, `cmake/rexbox_license.cmake`, `cmake/rexbox_recomp.cmake`, `tools/CMakeLists.txt`를 만들고 `src/`·`src/platform/`의 CMake를 `REXBOX_*` 변수로 고친다.
4. 코드: `SetApplicationIdentity` 계약, D3D8 기본 창 제목, `REJSRF_*` 이름, upstream v0.15.0 ABI 심볼(`g_eflags`, `xbox_Cpuid`, `xbox_PortIn/Out`, preempt).
5. 스크립트: `regenerate.py`, `xbe_variant.py`, `check_hle_pops.py`, `find_pointer_targets.py`, `resolve_samples.py`를 경로 인자로 일반화한다.
6. 문서: `AGENTS.md`(reJSRF 규칙), `CLAUDE.md`, `README.md`, `ARCHITECTURE.md`, `docs/PROJECT_CHARTER.md`, `docs/CODING_STYLE.md`·`docs/kb/` 손질, `docs/guides/`, `THIRD_PARTY_NOTICES.md`, `CREDITS.md`, `LICENSE`, `VERSION`(0.0.1), `.gitignore`, CI.
7. 검증: 설계 §7.

완료 기준: Win32 x86 Debug/Release 단독 빌드가 경고 없이 되고 probe가 통과하며, reCT3가 submodule로 가져가 빌드된다.

---

# Work order: splitting rexbox out of reJSRF (issue #1)

Design: [20261020-i001](../design/20261020-i001-project-bootstrap.md)

(1) Extract the history: fresh-clone reJSRF (v0.0.42, `f5939ee`), keep only the paths of design §2 with `git filter-repo`, move `src/rexbox/` to `src/`, rewrite issue numbers in commit titles to `nworkers/reJSRF#N`, keep the result as `main` with the remote `git@github.com:reexec/rexbox.git`. (2) Submodules: the same pins, xboxrecomp at upstream v0.15.0. (3) On the task branch `i001-project-bootstrap`, write the root `CMakeLists.txt`, the three `cmake/rexbox_*.cmake` helpers and `tools/CMakeLists.txt`, and switch the `src/` CMake files to `REXBOX_*` variables. (4) Code: the `SetApplicationIdentity` contract, the D3D8 default window title, the `REJSRF_*` names, the upstream v0.15.0 ABI symbols. (5) Scripts generalized to path arguments. (6) Documents: `AGENTS.md` (reJSRF's rules), `CLAUDE.md`, `README.md`, `ARCHITECTURE.md`, the charter, the adjusted coding style and kb, guides, third-party notices, credits, license, `VERSION` 0.0.1, `.gitignore`, CI. (7) Verify: design §7.

Done when the Win32 x86 Debug and Release standalone builds pass without warnings with the probes at 0 failures, and reCT3 builds it as a submodule.
