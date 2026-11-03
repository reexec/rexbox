# 작업 지시: 타이틀이 직접 쓰는 디바이스 푸시 버퍼 (issue #51)

설계: [20261103-i051](../design/20261103-i051-device-push-buffer.md)

1. `d3d8_push.{h,cpp}`: `SetupDevicePushBuffer`, `FlushDevicePushBuffer`, `D3D_MakeRequestedSpace_4__LTCG_eax2`, `CDevice_KickOff_0__LTCG_edx1`.
2. `CreateDevice`가 디바이스마다 푸시 버퍼를 준비하고, `Clear`, `Swap`, `Draw*`, `BeginPush`가 먼저 flush한다.
3. `d3d8_ltcg.{h,cpp}`(issue #50): 공개 서명을 유지하는 LTCG 변형 12개를 표로 다시 framing한다.
4. 빌드, probe, reEXCEED 실행. 해석하지 않은 메서드 목록을 작업 로그에 남긴다.

---

# Work order: the device push buffer the title writes itself (issue #51)

Design: [20261103-i051](../design/20261103-i051-device-push-buffer.md)

1. `d3d8_push.{h,cpp}`: `SetupDevicePushBuffer`, `FlushDevicePushBuffer`, `D3D_MakeRequestedSpace_4__LTCG_eax2`, `CDevice_KickOff_0__LTCG_edx1`.
2. `CreateDevice` sets up each device's push buffer; `Clear`, `Swap`, `Draw*` and `BeginPush` flush first.
3. `d3d8_ltcg.{h,cpp}` (issue #50): the twelve LTCG variants that keep the public signature, re-framed from a table.
4. Build, probes, a reEXCEED run; the uninterpreted methods go into the work log.
