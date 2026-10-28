# 작업 지시: Hor+에서 넓힌 깊이 스텐실과 함께 묶인 4:3 색 타깃 (issue #38)

설계: [20261029-i038](../design/20261029-i038-wide-depth-pairing.md). 소비자: reexec/reCT3#30.

1. `src/hle/d3d8/d3d8_capture.cpp`: 그리기 줄에 `zs <깊이 표면 헤더>`.
2. `src/hle/d3d8/d3d8_gl_targets.cpp`: `g_paired_colors`, `WideFor(surface, width, height)`, `BindRenderTarget`에서 짝 기록(크기가 다르면 한 번 보고), `SampleRenderTarget`의 복사를 가운데 4:3로.
3. 문서: `ARCHITECTURE.md`의 D3D8 행에 이 설계 링크.
4. 검증: Win32 x86 Debug/Release 빌드 경고 0, `ctest`. reCT3에서 21:9 밤 운전 캡처(`zs` 확인, 점 없음, 글로우 제자리), 4:3 데모 프레임 비교.
5. 작업 로그, 머지 때 `VERSION`과 릴리스 노트.

---

# Work order: a 4:3 color target bound with the widened depth stencil under Hor+ (issue #38)

Design: [20261029-i038](../design/20261029-i038-wide-depth-pairing.md). Consumer: reexec/reCT3#30.

1. `src/hle/d3d8/d3d8_capture.cpp`: draw lines gain `zs <depth surface header>`.
2. `src/hle/d3d8/d3d8_gl_targets.cpp`: `g_paired_colors`, `WideFor(surface, width, height)`, recording the pairing in `BindRenderTarget` (a different size is reported once), the 4:3-middle copy in `SampleRenderTarget`.
3. Documents: link this design from the D3D8 row of `ARCHITECTURE.md`.
4. Verify: Win32 x86 Debug and Release builds with zero warnings, `ctest`. In reCT3, a night driving capture at 21:9 (`zs` confirmed, no dots, glow in place) and a comparison of 4:3 demo frames.
5. Work log; at merge, `VERSION` and the release note.
