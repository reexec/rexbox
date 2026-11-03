# 작업 지시: D3D8 그리기 하나의 CPU 비용 (issue #23)

설계: [20261021-i023-draw-cpu-cost](../design/20261021-i023-draw-cpu-cost.md) · 소비자: reexec/reCT3#20

1. 작업 브랜치 `i023-draw-cpu-cost`(`main`에서). 커밋 제목 끝에 `(#23)`. 커밋마다 push한다.
2. 단계를 하나씩 넣고 그때마다 소비자에서 기준 빌드와 번갈아 측정한다(설계 §4).
   1. `UniformCache`: 위치로 바로 찾기(`d3d8_gl_programs.*`).
   2. push-buffer 조각 인덱스: 재사용 버퍼에서 한 번에 변환(`d3d8_gl_buffers.cpp`).
   3. 프로그램 조회: 앞 조회와 키가 같으면 해시·탐색 생략(`d3d8_gl_programs.cpp`). 설계 §3의 1(스냅샷 메모) 대신이다(§3.1).
   4. 렌더·텍스처 상태 읽기: 블록의 호스트 포인터를 한 번 구해 둔다(`d3d8_globals.cpp`).
   5. 진단: 프로파일에 같은 프레임 안 재비교 수(`again`)를 더한다. 정점 비교 생략(설계 §3의 4)은 이 수를 보고 정한다.
   6. 범위별 VAO(설계 §3의 5): 1~4의 측정 뒤.
   7. 2차(브랜치 `i023-draw-cpu-cost-2`): 1차 뒤 빠른 상태의 프레임은 대부분 vblank 둘(33.3 ms)이고, 120프레임마다 120~170 ms의 긴 프레임이 평균을 28 fps로 끌어내린다. 그리기 비용을 더 줄여도 16.7 ms 아래가 아니면 fps는 오르지 않으므로, 먼저 긴 프레임을 찾는다: 진단 `REXBOX_D3D8_SLOW_FRAME=<ms>`(그 프레임의 단계, 텍스처 업로드, 만든 프로그램).
   8. 2차 계속(2026-11-03, v0.0.19 위로 rebase): 진단 보고를 600프레임마다 모아 쓴다(보고가 측정하는 프레임을 늦추지 않게). 보고에 텍스처·프로그램 시간을 더한다. 그다음 [설계 20261103-i023-program-warmup](../design/20261103-i023-program-warmup.md): 프로그램 캐시를 표시 스레드의 vblank 대기 시간에 미리 올린다(`d3d8_gl_program_cache.*`, `d3d8_vblank.*`, `d3d8_device.cpp`, 프로파일의 프로그램 줄). 측정은 `REXBOX_SHADER_WARM=0`과 번갈아.
3. 검증: Win32 x86 Release, CTest, 소비자 운전 구간의 짝 실행, 프레임 덤프 비교.
4. 작업 로그 `docs/work-logs/20261021-i023-draw-cpu-cost.md`.

---

# Work order: the CPU cost of one D3D8 draw (issue #23)

Design: [20261021-i023-draw-cpu-cost](../design/20261021-i023-draw-cpu-cost.md) · Consumer: reexec/reCT3#20

(1) Task branch `i023-draw-cpu-cost` (from `main`); commit titles end with `(#23)`; every commit is pushed. (2) Land the steps one at a time, each measured in the consumer against the baseline build in alternating runs (design section 4): (i) `UniformCache` looked up by location (`d3d8_gl_programs.*`); (ii) push-buffer fragment indices converted in one pass into a reused buffer (`d3d8_gl_buffers.cpp`); (iii) program lookup skips hashing and searching when the key equals the previous lookup's (`d3d8_gl_programs.cpp`), in place of the snapshot memo of design section 3 item 1 (section 3.1); (iv) render- and texture-state reads through host pointers to the blocks found once (`d3d8_globals.cpp`); (v) diagnostic: the profile counts ranges compared again within a frame (`again`), which decides whether to skip vertex compares (design section 3 item 4); (vi) a VAO per range (design section 3 item 5), after measuring (i)–(iv). (vii) Second pass (branch `i023-draw-cpu-cost-2`): after the first pass most fast-state frames take two vblanks (33.3 ms) and long frames of 120–170 ms in every 120 pull the average to 28 fps; cutting the draw cost further raises fps only below 16.7 ms, so the long frames come first, with the diagnostic `REXBOX_D3D8_SLOW_FRAME=<ms>` (that frame's phases, texture uploads and programs made). (viii) Second pass continued (2026-11-03, rebased onto v0.0.19): the diagnostic's lines are held and written every 600 frames (so the report does not delay the frames it measures) and gain the texture and program times; then [design 20261103-i023-program-warmup](../design/20261103-i023-program-warmup.md): the program cache is warmed in the presenting thread's vertical-blank wait (`d3d8_gl_program_cache.*`, `d3d8_vblank.*`, `d3d8_device.cpp`, the profile's program line), measured against `REXBOX_SHADER_WARM=0` in alternating runs. (3) Verify: Win32 x86 Release, CTest, paired runs over the consumer's driving span, frame-dump comparison. (4) Work log `docs/work-logs/20261021-i023-draw-cpu-cost.md`.
