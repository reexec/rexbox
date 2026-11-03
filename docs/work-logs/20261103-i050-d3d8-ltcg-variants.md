# 작업 로그: D3D8LTCG의 레지스터 규약 변형 (issue #50)

설계: [20261103-i050](../design/20261103-i050-d3d8-ltcg-variants.md) · 작업 지시: [20261103-i050](../work-orders/20261103-i050-d3d8-ltcg-variants.md) · 소비자: reexec/reEXCEED#4

## 한 일 (1차)

* `HleCall`에 `Eax()`, `Ebx()`, `Esi()`, `Edi()`를 더했다.
* `Direct3D_CreateDevice`의 본문을 `CreateDevice(adapter, device_type, behavior, present, out)`으로 옮기고, `Direct3D_CreateDevice_4__LTCG_eax1_ecx3`(eax = BehaviorFlags, 스택 = pPresentationParameters, ecx = ppReturnedDeviceInterface, `ret 4`)를 더했다. 대응은 reEXCEED의 lift 본문 `0x00177A70`과 호출 지점에서 확인했다.

## 검증

* Win32 x86 Debug(reEXCEED 트리): 오류 0, 경고 0. CTest 7/7(probe 4개, reEXCEED smoke 2개, `reexceed_decomp_check`).
* reEXCEED: `[d3d8] Direct3D_CreateDevice: 640x480 format 07, 2 back buffer(s), multisample 0011, depth on format 2A, flags 00000001, interval 1, behavior 00000040`, 창 1280x960(OpenGL 3.3). 다음 정지점은 `D3D_MakeRequestedSpace_4__LTCG_eax2`(`0x00179470`).

## 남은 것

* `D3D_MakeRequestedSpace`는 단순 변형이 아니다. Exceed의 게임 코드(함수 7개)와 식별되지 않은 D3D 내부 함수가 디바이스 푸시 버퍼(`[0x185A88]` → 디바이스 → `pPut`)에 NV2A 메서드를 직접 쓰고, 모자라면 이것을 부른다. 디바이스 푸시 버퍼를 해석하는 설계가 먼저 필요하다(소비자와 방향 결정 대기).

---

# Work log: D3D8LTCG's register-convention variants (issue #50)

Design: [20261103-i050](../design/20261103-i050-d3d8-ltcg-variants.md) · Work order: [20261103-i050](../work-orders/20261103-i050-d3d8-ltcg-variants.md) · Consumer: reexec/reEXCEED#4

## Done (first pass)

`HleCall` gains `Eax()`, `Ebx()`, `Esi()` and `Edi()`. `Direct3D_CreateDevice`'s body moves into `CreateDevice(adapter, device_type, behavior, present, out)`, and `Direct3D_CreateDevice_4__LTCG_eax1_ecx3` (eax = BehaviorFlags, the stack = pPresentationParameters, ecx = ppReturnedDeviceInterface, `ret 4`) is added; the mapping was confirmed from reEXCEED's lifted body `0x00177A70` and its call site.

## Verification

Win32 x86 Debug (in the reEXCEED tree): 0 errors, 0 warnings; CTest 7/7 (the four probes, reEXCEED's two smoke tests, `reexceed_decomp_check`). In reEXCEED: `[d3d8] Direct3D_CreateDevice: 640x480 format 07, 2 back buffer(s), multisample 0011, depth on format 2A, flags 00000001, interval 1, behavior 00000040`, a 1280x960 window (OpenGL 3.3); the next stop is `D3D_MakeRequestedSpace_4__LTCG_eax2` (`0x00179470`).

## Open

`D3D_MakeRequestedSpace` is no plain variant: Exceed's game code (seven functions) and unidentified D3D internals write NV2A methods straight into the device push buffer (`[0x185A88]` → the device → `pPut`) and call it when room runs out. A design for interpreting the device push buffer comes first (direction pending with the consumer).
