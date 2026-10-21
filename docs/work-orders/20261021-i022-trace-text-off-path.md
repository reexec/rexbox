# 작업 지시: 추적이 꺼져 있을 때 D3D8 추적 문장을 만들지 않는다 (issue #22)

설계: [20261021-i022-trace-text-off-path](../design/20261021-i022-trace-text-off-path.md) · 소비자: reexec/reCT3#17

1. 작업 브랜치 `i022-trace-text-off-path`(`main`에서). 커밋 제목 끝에 `(#22)`.
2. `src/hle/d3d8/d3d8_state.cpp`: `SetModelView`의 비교와 추적, `SetVertexShaderInput`의 추적을 `D3d8TraceEnabled()` 안으로. `src/hle/d3d8/d3d8_gl_textures.cpp`: 업로드 추적도 같게.
3. `src/hle/d3d8/d3d8_trace.h`: `TraceOnce` 주석.
4. 검증: 설계 §4.
5. 작업 로그 `docs/work-logs/20261021-i022-trace-text-off-path.md`.

---

# Work order: no D3D8 trace text while tracing is off (issue #22)

Design: [20261021-i022-trace-text-off-path](../design/20261021-i022-trace-text-off-path.md) · Consumer: reexec/reCT3#17

(1) Task branch `i022-trace-text-off-path` (from `main`); commit titles end with `(#22)`. (2) `src/hle/d3d8/d3d8_state.cpp`: `SetModelView`'s comparison and trace and `SetVertexShaderInput`'s trace go inside `D3d8TraceEnabled()`; `src/hle/d3d8/d3d8_gl_textures.cpp`: the upload trace likewise. (3) `src/hle/d3d8/d3d8_trace.h`: `TraceOnce`'s comment. (4) Verify: design section 4. (5) Work log `docs/work-logs/20261021-i022-trace-text-off-path.md`.
