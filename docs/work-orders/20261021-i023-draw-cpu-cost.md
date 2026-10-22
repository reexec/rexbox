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
3. 검증: Win32 x86 Release, CTest, 소비자 운전 구간의 짝 실행, 프레임 덤프 비교.
4. 작업 로그 `docs/work-logs/20261021-i023-draw-cpu-cost.md`.

---

# Work order: the CPU cost of one D3D8 draw (issue #23)

Design: [20261021-i023-draw-cpu-cost](../design/20261021-i023-draw-cpu-cost.md) · Consumer: reexec/reCT3#20

(1) Task branch `i023-draw-cpu-cost` (from `main`); commit titles end with `(#23)`; every commit is pushed. (2) Land the steps one at a time, each measured in the consumer against the baseline build in alternating runs (design section 4): (i) `UniformCache` looked up by location (`d3d8_gl_programs.*`); (ii) push-buffer fragment indices converted in one pass into a reused buffer (`d3d8_gl_buffers.cpp`); (iii) program lookup skips hashing and searching when the key equals the previous lookup's (`d3d8_gl_programs.cpp`), in place of the snapshot memo of design section 3 item 1 (section 3.1); (iv) render- and texture-state reads through host pointers to the blocks found once (`d3d8_globals.cpp`); (v) diagnostic: the profile counts ranges compared again within a frame (`again`), which decides whether to skip vertex compares (design section 3 item 4); (vi) a VAO per range (design section 3 item 5), after measuring (i)–(iv). (3) Verify: Win32 x86 Release, CTest, paired runs over the consumer's driving span, frame-dump comparison. (4) Work log `docs/work-logs/20261021-i023-draw-cpu-cost.md`.
