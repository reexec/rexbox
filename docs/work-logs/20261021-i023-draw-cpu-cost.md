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

## 2차 (2026-11-03, 브랜치 `i023-draw-cpu-cost-2`, v0.0.19 위로 rebase)

기계: Ryzen 5 5600X, RTX 4090, Win32 x86 Release, 소비자 reCT3 v0.0.18. 이 기계에서 꾸준한 운전은 이미 60 fps다(그리기 단계 약 2.5 ms, `other` 약 2.4 ms).

1. 진단 `REXBOX_D3D8_SLOW_FRAME`이 측정을 망쳤다. 긴 프레임마다 stderr에 한 줄을 썼고, 리디렉션된 파일로 쓰는 데 한 번에 7~35 ms가 들었다. 그 쓰기가 다음 프레임을 길게 만들어 60 fps 운전이 30 fps에 갇혔다(**확인됨**: 같은 exe로 보고 없음 59.8 fps, 보고 있음 29.8 fps). 보고를 모아 600프레임마다 쓰고, 그 쓰기로 길어진 프레임에는 표시를 붙였다(`b496593`). 보고에 프레임의 텍스처·프로그램 시간을 더했다(`515fa4b`).
2. 그 결과 꾸준한 운전에는 긴 프레임이 없다. 600프레임마다 한 번 보이던 약 50 ms는 프로파일러가 보고를 쓰는 시간이다. 긴 프레임은 운전이 시작되는 몇 초에 몰린다. 텍스처 112개 15 ms(한 실행은 56 ms), 프로그램 25개 13 ms, 이어서 프로그램 22개 15 ms였다. 프로그램은 캐시에서 불러와도 하나에 1~2 ms가 든다.
3. [설계 20261103-i023-program-warmup](../design/20261103-i023-program-warmup.md): 표시 스레드가 vblank를 기다리는 시간에 캐시의 프로그램을 미리 올린다. 파일 읽기와 파일 시각 갱신은 읽기 스레드가 한다. 첫 구현은 표시 스레드에서 파일을 읽었다. 첫 실행(디스크)에서 항목당 약 6 ms가 걸려 부팅 프레임 160개가 33 ms를 넘었고, 그 실행은 영상 구간에서 `indirect call to 00000000 (returns to 000C967E)`로 멈췄다. 읽기 스레드로 옮긴 뒤 여섯 번 실행에서 멈춤은 없었다.

검증(같은 exe, `REXBOX_SHADER_WARM=0`과 번갈아 3쌍, 운전 진입):

| | 켬 | 끔 |
|---|---|---|
| 장면 시작의 캐시 프로그램 74개 | 3.9~4.9 ms | 40~42 ms |
| 운전 시작 최악 프레임 | 50~51 ms(한 실행은 텍스처 56 ms로 107 ms) | 58~61 ms |
| 프로그램 22개 프레임(36~38 ms) | 없음 | 매번 |
| 미리 올리기 | 1024개, GL 시간 약 230 ms(부팅 중 대기 시간) | 없음 |
| 부팅 구간 | 59.1~59.3 fps, 33 ms 넘는 프레임 2~3개 | 같음 |

* 빌드 경고 0(생성 코드의 기존 D9025 제외), reCT3 CTest 7/7.

## 남은 것 (2차)

* 운전 시작 프레임의 나머지: 텍스처 업로드(112개 15 ms, 가끔 56 ms)와 setup의 나머지 약 8 ms.
* 카메라 도입부의 그리기 1000~1500개 프레임(그리기 단계 11~15 ms).
* `000C967E`의 null 간접 호출은 미리 올리기로 부팅 타이밍이 흔들린 실행에서 한 번 나왔다. 원인은 모른다(reCT3 #18과 같은 계열로 **추정**).

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

Second pass (2026-11-03, branch `i023-draw-cpu-cost-2` rebased onto v0.0.19; Ryzen 5 5600X, RTX 4090, Win32 x86 Release, consumer reCT3 v0.0.18, where steady driving already runs at 60 fps with about 2.5 ms of draw phases and 2.4 ms of `other`):

1. The `REXBOX_D3D8_SLOW_FRAME` diagnostic spoiled the measurement: it wrote a line to stderr per long frame, each write to the redirected file took 7–35 ms and lengthened the next frame, holding 60 fps driving at 30 fps (**confirmed**: the same exe at 59.8 fps without reports, 29.8 with them). Reports are now held and written every 600 frames, with the frame that pays the write marked (`b496593`), and gain the frame's texture and program times (`515fa4b`).
2. Steady driving then has no long frames; the ~50 ms frame every 600 is the profiler writing its report. Long frames gather in the first seconds of driving: 112 textures in 15 ms (56 ms in one run), 25 programs in 13 ms, then 22 programs in 15 ms; a program costs 1–2 ms even from the cache.
3. [Design 20261103-i023-program-warmup](../design/20261103-i023-program-warmup.md): the presenting thread makes cached programs ahead in its vertical-blank wait, while a reader thread reads the files and updates file times. The first version read files on the presenting thread: on a first run (disk) an entry took about 6 ms, 160 boot frames went over 33 ms, and that run stopped in the movie with `indirect call to 00000000 (returns to 000C967E)`; after the move to a reader thread six runs had no stop.

Verification (same exe, alternating with `REXBOX_SHADER_WARM=0`, three pairs into driving): the 74 cached programs at the scene start took 3.9–4.9 ms with warming against 40–42 ms without; the worst frame at the start of driving was 50–51 ms against 58–61 ms (one warmed run reached 107 ms on a 56 ms texture burst); the frame with 22 programs (36–38 ms) appeared in every run without warming and in none with it; warming made 1024 programs in about 230 ms of GL time during boot waits; the boot span ran at 59.1–59.3 fps with 2–3 frames over 33 ms either way. Build with no warnings (apart from the generated code's existing D9025), reCT3 CTest 7/7.

Left over (second pass): the rest of the driving-start frame, texture uploads (112 in 15 ms, sometimes 56 ms) and about 8 ms more of setup; the camera-intro frames of 1000–1500 draws (11–15 ms of draw phases); the null indirect call at `000C967E`, seen once in a run whose boot timing warming disturbed, cause unknown (**inferred** to be of the same family as reCT3 #18).
