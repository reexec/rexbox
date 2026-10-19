# 작업 로그: GitHub Actions (issue #2)

설계: [20261020-i002](../design/20261020-i002-github-actions.md) · 작업 지시: [20261020-i002](../work-orders/20261020-i002-github-actions.md)

## 한 일

1. `CMakeLists.txt`에 `enable_testing()`, `tools/CMakeLists.txt`에 probe 세 개의 `add_test`.
2. `ci.yml`: 빌드 뒤 `ctest --test-dir build\win32_x86 -C <config> --output-on-failure`.
3. `release.yml`: `v*` 태그에서 `VERSION`과 `main` 검사 → 빌드·테스트(Debug/Release) → `docs/release-notes/<tag>.md`가 있으면 `gh release create --verify-tag --notes-file`(소스만).
4. `AGENTS.md` Git 규칙, `docs/guides/win32-setup-and-build.md`(`ctest`), `ARCHITECTURE.md` §5.

## 검증

| 검증 | 결과 |
|---|---|
| `ctest -C Debug` / `-C Release` (로컬, Win32 x86) | 3/3 통과 |
| 워크플로 YAML 파싱 | 통과 |
| Actions 실행 | 브랜치 push 뒤 `ci.yml`은 PR에서, `release.yml`은 첫 태그(`v0.0.1`)에서 돈다. 대기 |

---

# Work log: GitHub Actions (issue #2)

Design: [20261020-i002](../design/20261020-i002-github-actions.md) · Work order: [20261020-i002](../work-orders/20261020-i002-github-actions.md)

## Done

`enable_testing()` at the root and `add_test` for the three probes; `ci.yml` runs `ctest` after the build; `release.yml` checks a `v*` tag against `VERSION` and `main`, builds and tests Debug and Release, and creates the release from `docs/release-notes/<tag>.md` with `gh release create --verify-tag --notes-file` (source only); the Git rules in `AGENTS.md`, the guide (`ctest`) and `ARCHITECTURE.md` §5 updated.

## Verification

Local `ctest -C Debug` and `-C Release` on Win32 x86: 3/3 passed; the workflow YAML parses. The Actions runs themselves come after the push: `ci.yml` on the pull request, `release.yml` on the first tag (`v0.0.1`); pending.
