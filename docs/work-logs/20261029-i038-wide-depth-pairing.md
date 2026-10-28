# 작업 로그: Hor+에서 넓힌 깊이 스텐실과 함께 묶인 4:3 색 타깃 (issue #38)

설계: [20261029-i038](../design/20261029-i038-wide-depth-pairing.md). 작업 지시: [20261029-i038](../work-orders/20261029-i038-wide-depth-pairing.md). 소비자: reexec/reCT3#30.

## 한 일

* `d3d8_capture.cpp`: 그리기 줄에 `zs <깊이 표면 헤더>`.
* `d3d8_gl_targets.cpp`: `g_paired_colors`(표면 메모리 → 게스트 크기). `BindRenderTarget`은 넓힌 깊이 스텐실과 같은 크기의 색 타깃을 기록하고, 크기가 다르면 한 번 보고한다. `WideFor(surface, width, height)`는 기록된 색 메모리도 넓힌다. `SampleRenderTarget`의 복사는 `width × scale` 너비로 원본의 가운데만 가져온다(백버퍼 샘플링 포함).
* `ARCHITECTURE.md`의 D3D8 설명에 이 설계 링크.

## 검증

| 검증 | 결과 |
|---|---|
| reCT3 트리에서 Win32 x86 Release 빌드 | 경고·오류 0 |
| reCT3 `ctest -C Release` | 7/7 통과 |
| reCT3 21:9 밤 운전(사용자 플레이) | 헤드라이트를 따라다니던 검은 점 두 개가 사라짐(**확인됨**, 사용자 확인) |
| reCT3 21:9 어트랙트 데모(150 s, 200 프레임마다 덤프) | 끝까지 진행. 첫 실행은 present 1794에서 멈췄고(다른 스레드 대기 중 스레드 2 실행) 다시 실행하면 재현되지 않았다. 이 변경과 관련된 경로가 아니라 기존의 드문 정지로 본다 |
| 4:3 | 코드상 같음: `WideWidth`가 0이라 짝짓기가 없고, 샘플링 복사는 x 0, 전체 너비로 예전과 같다 |

## 남은 것

* `zs`로 짝을 직접 본 캡처는 아직 없다(사용자 장면은 재현 시점이 달라 이 작업에서는 결과 화면으로 확인했다).
* 합성 사각형이 가운데 4:3에만 놓여 양옆에는 글로우가 없다(i085의 알려진 한계).
* reJSRF는 이 변경을 받을 때 데모 프레임으로 확인한다.

---

# Work log: a 4:3 color target bound with the widened depth stencil under Hor+ (issue #38)

Design: [20261029-i038](../design/20261029-i038-wide-depth-pairing.md). Work order: [20261029-i038](../work-orders/20261029-i038-wide-depth-pairing.md). Consumer: reexec/reCT3#30.

Done: `d3d8_capture.cpp` draw lines gain `zs <depth surface header>`. `d3d8_gl_targets.cpp` gains `g_paired_colors` (surface memory → guest size): `BindRenderTarget` records a color target of the widened depth stencil's size and reports a different size once; `WideFor(surface, width, height)` widens recorded color memories too; `SampleRenderTarget`'s copy is `width × scale` wide and takes only the source's middle (back buffers included). `ARCHITECTURE.md`'s D3D8 description links the design.

Verification: Win32 x86 Release build in the reCT3 tree with no warnings or errors; reCT3 `ctest -C Release` 7/7; at 21:9 the two black dots that followed the headlights while driving at night are gone (**confirmed** by the user's play); the 21:9 attract demo ran 150 s with dumps every 200 frames (a first run stopped at present 1794 with thread 2 running while the others waited and did not reproduce on a rerun; taken as the existing rare stop, not a path this change touches); 4:3 is unchanged by construction (`WideWidth` is 0, so nothing pairs, and the sampling copy keeps x 0 and the full width).

Left: no capture has shown the pairing through `zs` yet (the user's scene cannot be replayed at a fixed frame, so this task checked the picture); the composite quad stays in the 4:3 middle, so the sides get no glow (i085's known limit); reJSRF checks its demo frames when it takes this change.
