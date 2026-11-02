# 작업 로그: render state로 쓴 combiner 상수 (issue #42)

작업 지시: [20261103-i042-combiner-factors](../work-orders/20261103-i042-combiner-factors.md) · 설계: [20261103-i042-combiner-factors](../design/20261103-i042-combiner-factors.md) · 소비자: reexec/reCT3#35

## 한 일

1. reCT3 해변 장면(프레임 2155)을 캡처했다(`REXBOX_CAPTURE_FRAME`). 진단(커밋하지 않음)으로 다음을 확인했다. 불꽃 사각형(#650~#728)은 픽셀 소유가 0이다. DXT3 알파는 최댓값 15다. 첫 정점의 clip 좌표는 화면 안 택시 위치다(ndc x −0.05~−0.17, w 약 330). 그리기마다 바뀌는 render state는 10(`COMBINER_FACTOR0`)뿐이고, 18은 0이다.
2. lift 코드에서 `sub_000A4710`이 0xA60, 0xA80을 `SetRenderState_Simple`(`000F3590`)로 쓰는 것을 찾았다.
3. 설계 §2대로 구현했다. d3d8 probe 확인 3개(220 → 223).

## 검증

| 검증 | 결과 |
|---|---|
| d3d8 probe | 223/223 |
| reCT3 Linux x64 Release | 경고 0, CTest 7/7 |
| reCT3 데모 프레임 | 해변(2155) 택시 뒷바퀴에 불꽃. 잔디 언덕(3720) 바퀴 자국의 불꽃 꼬리(초록빛), 프레임 3960 택시를 둘러싼 불꽃. 그 밖의 장면에 눈에 띄는 변화 없음 |
| 사용자 확인 | 불꽃 확인 |

## 남은 것

* 잔디 위 불꽃 꼬리의 초록빛이 원본과 같은지는 원본 영상과 비교하지 않았다.

---

# Work log: combiner constants written as render states (issue #42)

Work order: [20261103-i042-combiner-factors](../work-orders/20261103-i042-combiner-factors.md) · Design: [20261103-i042-combiner-factors](../design/20261103-i042-combiner-factors.md) · Consumer: reexec/reCT3#35

Done: capturing reCT3's beach scene (frame 2155, `REXBOX_CAPTURE_FRAME`) with diagnostics (not committed) showed the flame quads (#650–#728) owning no pixel, DXT3 alpha peaking at 15, the first vertex's clip position on screen at the taxi (ndc x −0.05 to −0.17, w about 330), and render state 10 (`COMBINER_FACTOR0`) the only one changing per draw, 18 being 0. The lifted `sub_000A4710` writes 0xA60 and 0xA80 through `SetRenderState_Simple` (`000F3590`). Implemented as design section 2; three d3d8 probe checks (220 → 223).

Verified: d3d8 probe 223/223; reCT3 Linux x64 Release with 0 warnings and CTest 7/7; reCT3 demo frames: flames at the taxi's rear wheel on the beach (2155), a flame trail along the wheel tracks on the grassy hill (3720, greenish) and flames around the taxi at frame 3960, no visible change elsewhere; the user confirmed the flames.

Left open: whether the trail's green tint on grass matches the original was not compared with original footage.
