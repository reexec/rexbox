# 작업 지시: P8 텍스처의 팔레트별 캐시 (issue #16)

설계: [20261021-i016-p8-palette-variants](../design/20261021-i016-p8-palette-variants.md) · 소비자: reexec/reCT3#11

1. 작업 브랜치 `i016-p8-palette-variants`. 커밋 제목 끝에 `(#16)`.
2. `src/hle/d3d8/d3d8_gl_textures.cpp`: 헤더 항목과 변형 목록(설계 §3), `ResolveTexture`의 변형 조회와 상한, `MarkTextureWritten`·`ForgetTexture`·`TextureCacheBytes`의 모든 변형 처리.
3. 검증: Win32 x86 빌드, CTest, 소비자 실행에서 설계 §4의 측정과 프레임 덤프.
4. 작업 로그 `docs/work-logs/20261021-i016-p8-palette-variants.md`.

---

# Work order: caching P8 textures per palette (issue #16)

Design: [20261021-i016-p8-palette-variants](../design/20261021-i016-p8-palette-variants.md) · Consumer: reexec/reCT3#11

(1) Task branch `i016-p8-palette-variants`; commit titles end with `(#16)`. (2) `src/hle/d3d8/d3d8_gl_textures.cpp`: header entries with variant lists (design section 3), variant lookup and the bound in `ResolveTexture`, and every variant handled by `MarkTextureWritten`, `ForgetTexture` and `TextureCacheBytes`. (3) Verify: Win32 x86 build, CTest, and design section 4's measurements and frame dumps in the consumer. (4) Work log `docs/work-logs/20261021-i016-p8-palette-variants.md`.
