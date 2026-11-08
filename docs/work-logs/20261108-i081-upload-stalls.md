# 작업 로그: 구역 진입 프레임의 텍스처 업로드와 새 프로그램 (issue #81)

작업 지시: [20261108-i081](../work-orders/20261108-i081-upload-stalls.md) · 설계: [20261108-i081](../design/20261108-i081-upload-stalls.md)

## 한 일

1. 1단계 `ebecbc5`(Antigravity 구현, Claude 설계·검토):
   * 느린 프레임 아래에 텍스처 업로드마다 한 줄을 붙인다. 내용은 이유(new, reupload(written·fields·hash)), 크기 유지 여부, 디코딩과 GL 시간이다.
   * 새 프로그램마다 첫 draw 시간, 그 draw의 setup 시간, 만든 방법을 한 줄로 붙인다.
   * 업로드를 할지 정하는 조건은 바꾸지 않았다(검토에서 확인).
2. 2단계는 하지 않기로 했다. 측정 결과 업로드와 새 프로그램은 주행 중 끊김의 작은 원인이었다(아래).

## 검증

* rexbox CTest 4/4, reCT3 Win32 x86 Release, CTest 7/7. 모두 코더 보고다.
* reCT3 West Coast 주행(`REXBOX_D3D8_SLOW_FRAME=20`)에서 업로드 604건, 새 프로그램 79개를 기록했다.
  * **주행 중(프레임 2400~6100)** 느린 프레임의 업로드는 183건으로 모두 `new`였다. GL 22.5 ms, 디코딩 12.6 ms를 다 합친 값이다.
  * 텍스처 시간이 2 ms를 넘은 주행 프레임은 4개였다. 주행 시작 때 112개를 올린 프레임(56 ms)과 16×32 텍스처 하나의 GL 12.6 ms(드라이버의 일회성 지연으로 보임) 등이다.
  * 주행 중에는 같은 텍스처를 다시 올리는 일이 없었다. "다시 정할 때의 동기화"라는 의심은 맞지 않았다.
  * 새 프로그램의 첫 GL draw 호출은 모두 0.0 ms였다. 드라이버가 컴파일을 첫 draw로 미루지 않는다. 주행 중 새 프로그램의 setup은 2.6~4.8 ms였다.
  * 주행 밖에서는 640×448 텍스처(`fmt 12`)를 프레임마다 다시 올렸다(같은 크기, 디코딩 약 1.5 ms). 동영상·메뉴 구간이다.

## 남은 것

* 주행 중 느린 프레임의 주원인은 draw가 많은 프레임의 HLE CPU 비용과 present 막힘이다(reexec/reCT3#41의 다음 단계).
* 동영상 구간의 640×448 재업로드 디코딩 비용(필요하면 따로).

---

# Work log: texture uploads and new programs in area-entry frames (issue #81)

Work order: [20261108-i081](../work-orders/20261108-i081-upload-stalls.md) · Design: [20261108-i081](../design/20261108-i081-upload-stalls.md)

## Done

1. Step 1, `ebecbc5` (implemented by Antigravity, designed and reviewed by Claude):
   * one line per texture upload under a slow frame: the reason (new, reupload(written, fields, hash)), whether the size stayed, and the decode and GL time;
   * one line per new program: its first draw's time, that draw's setup time, and how it was made;
   * the conditions that decide an upload are unchanged (checked in review).
2. Step 2 was not done: the measurement showed uploads and new programs to be a small cause of driving stalls (below).

## Verification

* rexbox CTest 4/4; reCT3 Win32 x86 Release, CTest 7/7. All of these are the coder's report.
* A reCT3 West Coast drive (`REXBOX_D3D8_SLOW_FRAME=20`) logged 604 uploads and 79 new programs.
  * **During driving (frames 2400–6100)** the slow frames held 183 uploads, all `new`: 22.5 ms of GL and 12.6 ms of decoding in total.
  * Four driving frames spent over 2 ms on textures, among them the drive's start with 112 uploads (56 ms) and a single 16×32 texture taking 12.6 ms of GL (it looks like a one-off driver delay).
  * No texture was re-uploaded during driving, so the suspected sync on re-specification does not apply.
  * Every new program's first GL draw call took 0.0 ms, so the driver does not defer compiling to the first draw. New programs' setup during driving took 2.6–4.8 ms.
  * Outside driving a 640×448 texture (`fmt 12`) was re-uploaded every frame (same size, about 1.5 ms of decoding): the movie and menu stretches.

## Remaining

* The main causes of driving's slow frames are the HLE CPU cost of draw-heavy frames and the present stalls (the next steps of reexec/reCT3#41).
* The decoding cost of the 640×448 re-uploads in the movie stretches (separately, if needed).
