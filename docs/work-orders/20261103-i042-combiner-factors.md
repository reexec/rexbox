# 작업 지시: render state로 쓴 combiner 상수 (issue #42)

설계: [20261103-i042-combiner-factors](../design/20261103-i042-combiner-factors.md)

1. 작업 브랜치 `i042-combiner-factors`(issue #43도 이 브랜치에서). 커밋 제목 끝에 `(#42)`.
2. `d3d8_psh.h/.cpp`: factor 상태 목록과 `StageConstants`의 factor 인자. `d3d8_shader.h/.cpp`: 쓰임 비트. `d3d8_globals.cpp`: `SetRenderStateValue`에서 표시. `d3d8_draw.cpp`: 호출.
3. d3d8 probe 확인 3개. 소비자 reCT3로 데모 프레임 비교.
4. 작업 로그, ARCHITECTURE.

---

# Work order: combiner constants written as render states (issue #42)

Design: [20261103-i042-combiner-factors](../design/20261103-i042-combiner-factors.md)

(1) Task branch `i042-combiner-factors` (issue #43 shares it); commit titles end with `(#42)`. (2) `d3d8_psh.h/.cpp`: the factor states and `StageConstants`' factor arguments; `d3d8_shader.h/.cpp`: the written bits; `d3d8_globals.cpp`: marking in `SetRenderStateValue`; `d3d8_draw.cpp`: the call. (3) Three d3d8 probe checks; demo frames compared through the consumer reCT3. (4) Work log, ARCHITECTURE.
