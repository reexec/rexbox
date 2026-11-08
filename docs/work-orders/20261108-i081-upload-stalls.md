# 작업 지시: 구역 진입 프레임의 텍스처 업로드와 새 프로그램 지연 (issue #81)

설계: [20261108-i081](../design/20261108-i081-upload-stalls.md)

1. 브랜치 `i081-upload-stalls`(#80 브랜치 `i080-slow-frame-waits` 위. #80을 먼저 머지한 뒤 main 위로 옮긴다). 커밋 제목 끝에 `(#81)`.
2. 1단계: `d3d8_gl_textures.cpp`(업로드 이유, 디코딩·GL 시간), 프로그램의 첫 draw 시간(프로그램 캐시와 draw 경로), `d3d8_profile.cpp`(느린 프레임 상세). 설계 §2.
3. 빌드, probe, CTest. reCT3 주행 측정.
4. 2단계: 결과로 설계 §3을 정하고 구현한다.
5. 작업 로그, 릴리스 노트.

---

# Work order: texture uploads and new programs stalling area-entry frames (issue #81)

Design: [20261108-i081](../design/20261108-i081-upload-stalls.md)

(1) Branch `i081-upload-stalls`, on #80's branch `i080-slow-frame-waits` (moved onto main once #80 is merged); commit titles end with `(#81)`. (2) Step 1: `d3d8_gl_textures.cpp` (the upload reason, decode and GL time), the program's first-draw time (program cache and draw path), `d3d8_profile.cpp` (the slow-frame detail); design section 2. (3) Build, probes, CTest; the reCT3 drive measurement. (4) Step 2: settle design section 3 from the results and implement. (5) Work log, release notes.
