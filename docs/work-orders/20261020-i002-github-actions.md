# 작업 지시: GitHub Actions (issue #2)

설계: [20261020-i002](../design/20261020-i002-github-actions.md)

1. CTest: 루트 `enable_testing()`, `tools/CMakeLists.txt`에 probe 세 개 `add_test`.
2. `.github/workflows/ci.yml`: 빌드 뒤 `ctest`로 단위 테스트.
3. `.github/workflows/release.yml`: `v*` 태그에서 VERSION·main 검사, 빌드·테스트, 릴리스 노트가 있으면 `gh release create`.
4. `AGENTS.md` Git 규칙에 Actions 동작과 릴리스 노트 선행 조건 추가. 가이드에 `ctest` 명령 추가.
5. 검증: 설계 §3. 작업 브랜치 `i001-project-bootstrap`에서 이어서 작업한다(규칙: `main`이 아니면 새 브랜치를 만들지 않는다).

---

# Work order: GitHub Actions (issue #2)

Design: [20261020-i002](../design/20261020-i002-github-actions.md)

(1) CTest: `enable_testing()` at the root and `add_test` for the three probes in `tools/CMakeLists.txt`. (2) `ci.yml`: unit tests through `ctest` after the build. (3) `release.yml`: on `v*` tags check VERSION and main, build and test, and `gh release create` when the release note exists. (4) Add the Actions behavior and the release-note precondition to `AGENTS.md`'s Git rules; add the `ctest` command to the guide. (5) Verify as in design §3, continuing on the task branch `i001-project-bootstrap` (the rules: no new branch while off `main`).
