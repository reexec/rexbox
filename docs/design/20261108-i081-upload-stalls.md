# 설계: 구역 진입 프레임의 텍스처 업로드와 새 프로그램 지연 (issue #81)

상태: 설계(1단계: 진단). 소비자: reexec/reCT3#41. 이전: [i080 느린 프레임의 대기](20261108-i080-slow-frame-waits.md).

## 1. 요구사항

#80의 느린 프레임 보고로 Crazy Taxi 3 주행을 보면, 구역에 들어설 때 텍스처 업로드와 새 프로그램이 크기에 비해 훨씬 오래 걸린다.

* 프레임 2554: 텍스처 7개, 20 KiB, 8.3 ms. 프레임 2553: 1개, 3 KiB, 2.8 ms. 프레임 2574: 4개, 9 KiB, 7.2 ms. KiB는 GL로 보낸 양이고, DXT는 압축된 채 올라간다.
* 프레임 2653: 프로그램 2개를 만들었다. 만드는 데는 0.0 ms로 셌지만, 그 프레임의 setup 단계가 27.9 ms였다.

의심하는 원인은 둘이다.
* 텍스처를 같은 GL 이름에 다시 정하는 경우(`glTexImage2D`/`glCompressedTexImage2D`). 앞서 큐에 들어간 GPU 작업이 아직 그 텍스처를 쓰면 드라이버가 기다릴 수 있다.
* 프로그램의 실제 컴파일·링크가 만들 때가 아니라 첫 draw에서 일어나는 경우.

## 2. 1단계: 느린 프레임의 업로드·프로그램 상세

`REXBOX_D3D8_SLOW_FRAME`이 켜져 있을 때만 모은다. 느린 프레임 줄 아래에 덧붙이고, 느리지 않은 프레임의 기록은 버린다.

* 텍스처 업로드마다 한 줄(한 프레임 16줄까지, 넘으면 개수만):
  ```
    texture 80123450 fmt 0C 256x256 9 lv 43 KiB: reupload(hash) same-size, decode 0.0 ms, gl 2.7 ms
  ```
  * 헤더 주소, 형식, 크기, 레벨 수, GL로 보낸 바이트.
  * 이유: `new`(GL 이름이 처음 생김), `reupload(written)`, `reupload(fields)`(형식·크기·데이터 주소가 바뀜), `reupload(hash)`(샘플 해시가 바뀜).
  * 다시 올릴 때 크기·형식이 같았는지(`same-size`/`resized`).
  * 시간: CPU 디코딩(`DecodeTextureLevel`, `DecodePalettedLevel`, 자르기)과 GL 호출(`glTexImage*`, `glCompressedTexImage2D`, `glTexParameteri`)을 따로 잰다.
* 프로그램: 그 프레임에서 처음으로 draw에 쓰인 프로그램마다 한 줄.
  ```
    program 37: first draw 13.8 ms (made 0.0 ms, from the binary cache)
  ```
  * 첫 draw의 GL draw 호출 시간과, 그 draw의 setup 단계 시간을 함께 적는다.
  * 만든 방법(컴파일, 바이너리 캐시, 미리 데우기)도 적는다.
* 비용: 켜졌을 때만 시계를 읽고 기록한다. 기본 경로는 플래그 하나를 본다.

## 3. 2단계 (1단계 결과에 따라)

* 같은 크기로 다시 올리는 텍스처가 GL 시간을 쓰면: 같은 이름에 다시 정하지 않는다. 새 GL 이름으로 올리고 예전 이름은 몇 프레임 뒤에 지운다. 또는 `glTexSubImage2D`로 바꾼다.
* 프로그램의 첫 draw가 길면: 드라이버의 늦은 컴파일을 앞당긴다. 예를 들어 만들 때나 데울 때 작은 화면 밖 draw를 한 번 한다.
* 결정은 이 문서를 고쳐 기록한다.

## 4. 검증

* d3d8 probe, CTest.
* reCT3 주행(`REXBOX_D3D8_SLOW_FRAME=20`)에서 구역 진입 프레임의 상세를 읽는다.

---

# Design: texture uploads and new programs stalling area-entry frames (issue #81)

Status: design (step 1: a diagnostic). Consumer: reexec/reCT3#41. Earlier: [i080 slow-frame waits](20261108-i080-slow-frame-waits.md).

## 1. Requirement

With #80's slow-frame report, a Crazy Taxi 3 drive shows texture uploads and new programs taking far longer than their size when entering an area:

* frame 2554: 7 textures, 20 KiB, 8.3 ms; frame 2553: 1 texture, 3 KiB, 2.8 ms; frame 2574: 4 textures, 9 KiB, 7.2 ms. The KiB are what went to GL; DXT goes up compressed.
* frame 2653: 2 programs made, counted at 0.0 ms for making them, but that frame's setup phase took 27.9 ms.

There are two suspects:
* re-specifying a texture on the same GL name (`glTexImage2D`/`glCompressedTexImage2D`); if GPU work queued earlier still uses the texture, the driver may wait;
* a program's real compile or link happening at its first draw rather than when it is made.

## 2. Step 1: upload and program detail for slow frames

Collected only with `REXBOX_D3D8_SLOW_FRAME` set. The detail is appended below a slow frame's line, and the records of frames that were not slow are dropped.

* One line per texture upload (up to 16 per frame; beyond that, only the count):
  ```
    texture 80123450 fmt 0C 256x256 9 lv 43 KiB: reupload(hash) same-size, decode 0.0 ms, gl 2.7 ms
  ```
  * The header address, format, size, level count and bytes sent to GL.
  * The reason: `new` (the GL name was just made), `reupload(written)`, `reupload(fields)` (format, size or data address changed), `reupload(hash)` (the sampled hash changed).
  * For a re-upload, whether the size and format stayed the same (`same-size`/`resized`).
  * The time, split into CPU decoding (`DecodeTextureLevel`, `DecodePalettedLevel`, cropping) and the GL calls (`glTexImage*`, `glCompressedTexImage2D`, `glTexParameteri`).
* Programs: one line for each program first used by a draw in that frame.
  ```
    program 37: first draw 13.8 ms (made 0.0 ms, from the binary cache)
  ```
  * The first draw's GL draw-call time and that draw's setup-phase time.
  * How it was made (compiled, from the binary cache, warmed).
* Cost: the clock is read and records kept only when enabled; the default path checks one flag.

## 3. Step 2 (depending on step 1)

* If same-size re-uploads spend the GL time: stop re-specifying the same name. Upload to a new GL name and delete the old one a few frames later, or switch to `glTexSubImage2D`.
* If a program's first draw is long: bring the driver's late compile forward, for example with one small off-screen draw when the program is made or warmed.
* The decision is recorded by revising this document.

## 4. Verification

* The d3d8 probe, CTest.
* A reCT3 drive (`REXBOX_D3D8_SLOW_FRAME=20`), reading the detail of the area-entry frames.
