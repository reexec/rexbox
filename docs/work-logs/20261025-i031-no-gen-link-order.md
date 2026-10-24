# 작업 로그: 생성 코드 없는 트리의 GNU ld 링크 순서 (issue #31)

reJSRF의 Linux CI(nworkers/reJSRF#119)가 잡았다. 생성 코드가 없는 트리(게임 데이터 없는 CI)에서 Linux 앱이 `rexbox_recomp_get_gpr` 등 미정의 참조 17개로 링크되지 않았다. WSL2 GCC 13에서 재현했다.

## 원인

`rexbox_add_recompiled_library`의 생성 코드 없는 분기는 레지스터 접근 함수 대체 라이브러리 `<target>_abi`(`recomp_abi_none.c`)를 타이틀 라이브러리에 PUBLIC으로 붙일 뿐, 그 함수를 **쓰는** `rexbox_recomp`에는 의존 관계를 만들지 않았다. 링크 줄에서 `<target>_abi`가 `rexbox_recomp`보다 앞에 놓이고, 정적 라이브러리를 순서대로 한 번만 훑는 GNU ld는 아직 참조되지 않은 심볼을 버린다. MSVC는 순서를 가리지 않아 v0.0.12의 Win32 확인(`rexbox_recomp_probe`)에서 드러나지 않았다. 생성 코드가 있는 분기는 `<target>` ↔ 타이틀 순환을 양방향으로 선언해 이미 피하고 있었다.

## 한 일

* `cmake/rexbox_recomp.cmake`: 생성 코드 없는 분기에서 `target_link_libraries(rexbox_recomp PUBLIC ${target}_abi)`를 더해 `rexbox_recomp`가 `<target>_abi`보다 앞에 오게 했다.

## 검증

| 구성 | 결과 |
|---|---|
| rexbox Linux x64 Debug(WSL2, GCC 13) | 경고 0, 미정의 참조 0, CTest 4/4(`rexbox_recomp_probe` 포함) |
| reJSRF Linux x64 Debug, `REJSRF_RECOMP_GEN_DIR` 없음 | 수정 전 미정의 참조 17개 → 수정 뒤 0, 앱 링크. smoke(디렉터리 인자)에 종료 코드 1 |
| rexbox Win32 x86 Debug | 경고 0, `rexbox_recomp_probe` 8/8 |

---

# Work log: GNU ld link order in a tree without generated code (issue #31)

Found by reJSRF's Linux CI (nworkers/reJSRF#119): in a tree without generated code (a CI without game data) the Linux application failed to link with 17 undefined references such as `rexbox_recomp_get_gpr`; reproduced on WSL2 with GCC 13.

## Cause

The no-generated-code branch of `rexbox_add_recompiled_library` linked the accessor stand-in `<target>_abi` (`recomp_abi_none.c`) PUBLIC into the title library but declared no dependency from `rexbox_recomp`, which **uses** those functions. On the link line `<target>_abi` preceded `rexbox_recomp`, and GNU ld, scanning static archives once in order, discards symbols nothing has referenced yet. MSVC ignores order, so v0.0.12's Win32 check (`rexbox_recomp_probe`) did not show it; the with-generated-code branch already avoided it by declaring the `<target>` ↔ title cycle both ways.

## Done

* `cmake/rexbox_recomp.cmake`: the no-generated-code branch adds `target_link_libraries(rexbox_recomp PUBLIC ${target}_abi)`, so `rexbox_recomp` precedes `<target>_abi`.

## Verification

* rexbox Linux x64 Debug (WSL2, GCC 13): 0 warnings, 0 undefined references, CTest 4/4 (`rexbox_recomp_probe` included).
* reJSRF Linux x64 Debug with no `REJSRF_RECOMP_GEN_DIR`: 17 undefined references before, 0 after; the app links, and the smoke check (a directory argument) exits with 1.
* rexbox Win32 x86 Debug: 0 warnings, `rexbox_recomp_probe` 8/8.
