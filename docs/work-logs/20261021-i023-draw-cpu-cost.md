# 작업 로그: D3D8 그리기 하나의 CPU 비용 — 1차 (issue #23)

작업 지시: [20261021-i023-draw-cpu-cost](../work-orders/20261021-i023-draw-cpu-cost.md) · 설계: [20261021-i023-draw-cpu-cost](../design/20261021-i023-draw-cpu-cost.md) · 소비자: reexec/reCT3#20

## 한 일

| 커밋 | 단계 |
|---|---|
| `35ca8f8` | `UniformCache`를 위치로 바로 찾음. push-buffer 조각의 16비트 인덱스를 재사용 버퍼에서 한 번에 변환. 프로그램 map 조회가 앞 조회와 키가 같으면 해시·탐색을 건너뜀(`LastFound`). 렌더·텍스처 상태 블록의 호스트 포인터와 슬롯 수를 주소를 찾을 때 보관. 프로파일에 같은 프레임 안 재비교 수(`again`) |
| `7634f9e` | 캐시된 정점 범위마다 VAO. 인덱스는 `GL_COPY_WRITE_BUFFER`로 올리고 element 버퍼는 그리기 직전에 VAO마다 바인딩. 버퍼를 지우면 모든 VAO의 포인터·element 기억을 지움. `glDeleteVertexArrays` 추가 |
| `c46c9f2` | combiner 상수는 정의나 픽셀 셰이더 상수(새 버전 번호)가 바뀔 때만 계산. 단계마다 앞 sampler 재사용 |

설계에서 바꾼 것(§3.1): 스냅샷 메모는 `ResolveTexture`의 render target 사본 갱신과 프레임당 해시 검사를 건너뛰게 되어 그림이 틀릴 수 있으므로, 키는 매번 만들고 프로그램 조회만 앞 결과를 쓴다. 정점 비교 생략(§3의 4)은 재비교가 프레임당 31~39개(전체의 4%)뿐이라 하지 않았다. 쓰기 추적은 조회마다 시스템 호출이고, 프레임당 한 번으로 묶으면 프레임 안의 쓰기를 놓친다.

## 검증

소비자 운전 구간(draws 약 810), 기준 v0.0.9와 번갈아 바로 이은 짝 실행:

| 상태 | 빌드 | fps | CPU | D3D8 단계 | setup | constants | indices | upload | attributes |
|---|---|---|---|---|---|---|---|---|---|
| 느린 기계(s2) | v0.0.9 | 18.5 | 32.0 | 22.7 | 5.9 | 4.4 | 3.4 | 2.4 | 2.3 |
| | VAO까지 | 20.7 | 28.0 | 17.2 | 4.2 | 4.0 | 1.4 | 3.0 | 0.5 |
| 빠른 기계(s4 짝 1) | v0.0.9 | 28.2 | 23.5 | 13.7 | 3.74 | 2.70 | 1.87 | 1.62 | 1.32 |
| | `c46c9f2`까지 | 27.8 | 21.7 | 11.5 | 2.84 | 2.29 | 0.95 | 2.43 | 0.34 |

* D3D8 그리기 단계는 16~24%, 표시 스레드 CPU는 프레임당 1.8~4 ms 줄었다.
* fps는 느린 상태에서 +12%이고, 빠른 상태에서는 그대로다. 바쁜 시간이 vblank 둘(33.3 ms) 근처여서, 줄어든 시간이 vblank 대기로 바뀌었다.
* upload가 0.8 ms 늘었다: VAO 바인딩(`glBindVertexArray`)이 그 단계로 옮겨 왔다. attributes의 감소(1.0 ms)가 더 크다.
* GL 호출: v0.0.9 운전에서 프레임당 약 8000번(`glVertexAttribPointer` 약 3000번)이었다.
* 그림: 어트랙트 데모 frame 1500, 운전 frame 2700의 덤프. 같은 장면이면 움직이는 물체만 다르고, 깨진 곳이 없다.
* Win32 x86 Release 빌드 오류 0, CTest reCT3 5/5, rexbox 3/3.
* 측정 조건: 같은 날 다른 프로젝트 빌드와 VS Code 색인이 CPU를 100% 쓴 구간이 있었다. 그때의 실행은 버리고, `cl.exe`가 없을 때만 실행했다.

## 남은 것

* 30 fps 고정(설계 기대)에 이르지 못했다. 다음 후보:
  * constants의 행렬 계산과 조명
  * VAO 바인딩 비용(같은 범위가 이어지면 생략)
  * `UniformCache::Changed`의 남은 비교
  * present 안의 드라이버 제출(`ZwSetEvent`, 표본 6%)
* 60 fps에는 렌더 스레드(설계 §3.2)가 필요하다.
* `REXBOX_D3D8_PROFILE` 자체가 단계마다 `steady_clock::now`를 불러 프레임당 약 1 ms를 쓴다(QPC 표본 1.5%). 비교는 같은 조건이라 상쇄된다.

---

# Work log: the CPU cost of one D3D8 draw — first pass (issue #23)

Work order: [20261021-i023-draw-cpu-cost](../work-orders/20261021-i023-draw-cpu-cost.md) · Design: [20261021-i023-draw-cpu-cost](../design/20261021-i023-draw-cpu-cost.md) · Consumer: reexec/reCT3#20

Done:

* `35ca8f8`: `UniformCache` looked up by location; push-buffer fragments' 16-bit indices converted in one pass into a reused buffer; the program-map lookups return the previous result without hashing and searching when the key matches (`LastFound`); the render- and texture-state blocks' host pointers and slot count kept when the addresses are found; the profile counts ranges compared again within a frame (`again`).
* `7634f9e`: a VAO per cached vertex range; indices uploaded through `GL_COPY_WRITE_BUFFER` and the element buffer bound per VAO just before the draw; deleting a buffer makes every VAO send its pointers and element binding again; `glDeleteVertexArrays` added.
* `c46c9f2`: combiner constants formed only when the definition or the pixel-shader constants (a new version number) change; each stage reuses its previous sampler.

Changed from the design (section 3.1): the snapshot memo would skip `ResolveTexture`'s render-target copy refresh and per-frame hash check and could draw wrong pictures, so the key is still built on every draw and only the program lookup reuses the previous result. Skipping vertex compares (section 3 item 4) was dropped: only 31–39 ranges per frame (4%) are compared again, write tracking costs a system call per query, and one query per frame would miss writes within the frame.

Verification over the consumer's driving span (about 810 draws), alternating back-to-back runs against v0.0.9:

| State | Build | fps | CPU | D3D8 phases | setup | constants | indices | upload | attributes |
|---|---|---|---|---|---|---|---|---|---|
| slow machine (s2) | v0.0.9 | 18.5 | 32.0 | 22.7 | 5.9 | 4.4 | 3.4 | 2.4 | 2.3 |
| | up to the VAOs | 20.7 | 28.0 | 17.2 | 4.2 | 4.0 | 1.4 | 3.0 | 0.5 |
| fast machine (s4 pair 1) | v0.0.9 | 28.2 | 23.5 | 13.7 | 3.74 | 2.70 | 1.87 | 1.62 | 1.32 |
| | up to `c46c9f2` | 27.8 | 21.7 | 11.5 | 2.84 | 2.29 | 0.95 | 2.43 | 0.34 |

* The D3D8 draw phases fell by 16–24% and the presenting thread's CPU by 1.8–4 ms per frame.
* fps rose 12% in the slow state and stayed level in the fast one: the busy time sits near two vblanks (33.3 ms), so the saved time turned into vblank wait.
* upload grew 0.8 ms because the VAO binds (`glBindVertexArray`) moved into that phase; attributes fell further (1.0 ms).
* GL calls: v0.0.9 made about 8000 per frame while driving (about 3000 `glVertexAttribPointer`).
* Pictures: dumps of attract-demo frame 1500 and driving frame 2700; matching scenes differ only in moving objects, with nothing broken.
* Win32 x86 Release with no build errors; CTest reCT3 5/5, rexbox 3/3.
* Conditions: another project's build and VS Code's indexing kept the CPU at 100% for parts of the day; those runs were discarded and runs started only with no `cl.exe` running.

Left over:

* A steady 30 fps (the design's expectation) is not reached. Next candidates: the matrix and lighting work in constants, the VAO bind cost (skipped when the same range repeats), the remaining compares in `UniformCache::Changed`, and the driver's submission inside present (`ZwSetEvent`, 6% of samples).
* 60 fps needs the render thread (design section 3.2).
* `REXBOX_D3D8_PROFILE` itself calls `steady_clock::now` per phase, about 1 ms per frame (QPC 1.5% of samples); both builds pay it, so comparisons are unaffected.
