# 작업 로그: P8 텍스처의 팔레트별 캐시 — 완료 (issue #16)

작업 지시: [20261021-i016-p8-palette-variants](../work-orders/20261021-i016-p8-palette-variants.md) · 설계: [20261021-i016-p8-palette-variants](../design/20261021-i016-p8-palette-variants.md) · 소비자: reexec/reCT3#11

## 한 일

1. 소비자 도시 장면의 표시 스레드를 `REXBOX_SAMPLE`로 샘플링했다. vblank 대기를 빼면 텍스처 디코딩(`SwizzleOffset`과 그 안의 `std::min`/`max`, `DecodePalettedLevel`)이 약 4분의 1이었다.
2. 임시 계측으로 업로드 원인을 셌다. 같은 P8 헤더가 한 프레임에 최대 7개 팔레트로 그려지고, 팔레트가 바뀔 때마다 다시 올라갔다(프레임당 최대 42회). 이것이 `setup`과 함께 움직였다.
3. `d3d8_gl_textures.cpp`: 헤더 항목에 변형 목록을 두고, P8은 팔레트 `Data`·`Common`마다 변형 하나(헤더당 최대 16, LRU 교체). `MarkTextureWritten`·`ForgetTexture`·`TextureCacheBytes`는 모든 변형을 다룬다.

## 검증

| 검증 | 결과 |
|---|---|
| Win32 x86 Release, CTest | 오류 0, 통과 |
| 소비자 도시 장면(300프레임 구간) | 바꾸기 전 22.0~26.1 fps, `setup` 5.9~12.4 ms → 바꾼 뒤 27.9~30.0 fps, `setup` 1.2~2.3 ms. CPU 19.3 → 9.5~12.8 ms |
| P8 업로드(임시 계측) | 프레임당 13.9~42.3회 → 0~0.4회(새 변형의 첫 사용만). 약 980개 헤더에 변형 약 1,060개 |
| 화면 | frame 1500, 2100: 같은 차 텍스처가 색마다 따로 그려지고(검정, 파랑) 지형·간판이 정상 |

---

# Work log: caching P8 textures per palette — done (issue #16)

Work order: [20261021-i016-p8-palette-variants](../work-orders/20261021-i016-p8-palette-variants.md) · Design: [20261021-i016-p8-palette-variants](../design/20261021-i016-p8-palette-variants.md) · Consumer: reexec/reCT3#11

Done: the presenting thread in the consumer's city scenes was sampled with `REXBOX_SAMPLE`; apart from the vblank wait, about a quarter of the samples were texture decoding (`SwizzleOffset` with its `std::min`/`max`, `DecodePalettedLevel`). Temporary counters showed the same P8 header drawn with up to 7 palettes in a frame and re-uploaded at every palette switch (up to 42 per frame), moving with `setup`. `d3d8_gl_textures.cpp` now keeps a variant list per header entry, one variant per palette `Data` and `Common` for P8 (up to 16 per header, least recently used replaced); `MarkTextureWritten`, `ForgetTexture` and `TextureCacheBytes` handle every variant.

Verification: Win32 x86 Release with no errors and CTest passing. In the consumer's city scenes (300-frame spans) 22.0–26.1 fps with `setup` at 5.9–12.4 ms became 27.9–30.0 fps with `setup` at 1.2–2.3 ms, and CPU time went from 19.3 ms to 9.5–12.8 ms. P8 uploads (temporary counters) went from 13.9–42.3 per frame to 0–0.4 (only a new variant's first use), with about 1,060 variants for about 980 headers. In frames 1500 and 2100 the same car texture draws separately in each colour (black, blue), and the geometry and signs are correct.
