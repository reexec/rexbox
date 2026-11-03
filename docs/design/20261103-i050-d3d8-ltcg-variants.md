# 설계: D3D8LTCG의 레지스터 규약 변형 (issue #50)

상태: **일부 구현됨**(`Direct3D_CreateDevice_4__LTCG_eax1_ecx3`). 작업 지시: [20261103-i050](../work-orders/20261103-i050-d3d8-ltcg-variants.md) · 작업 로그: [20261103-i050](../work-logs/20261103-i050-d3d8-ltcg-variants.md). 소비자: reexec/reEXCEED#4.

## 1. 요구사항

Pump It Up: Exceed는 XDK 5849의 D3D8LTCG를 링크한다. XbSymbolDatabase는 그중 22개를 `<이름>_<스택 바이트>__LTCG_<레지스터><인자 번호>...` 형식의 변형으로 식별한다. HLE에는 변형이 하나도 없어 부팅이 `Direct3D_CreateDevice_4__LTCG_eax1_ecx3`에서 멈춘다.

## 2. 규약 (Exceed에서 확인)

* **확인됨**(lift된 `0x00177A70`과 호출 지점 `0x000766DF`): 인자 번호는 공개 API가 아니라 **LTCG가 줄인 서명**의 번호다. `Direct3D_CreateDevice_4__LTCG_eax1_ecx3`은 `(BehaviorFlags = eax, pPresentationParameters = 스택 인자 1개, ppReturnedDeviceInterface = ecx)`이고, Adapter·DeviceType·hFocusWindow는 상수로 접혔다. `ret 4`, 결과는 eax.
* 그러므로 변형마다 공개 함수에 대응하는 표가 필요하며, lift 본문으로 하나씩 확인한다(**추정**: 다른 변형도 같은 규칙).

| 변형 (Exceed) | |
|---|---|
| `Direct3D_CreateDevice_4__LTCG_eax1_ecx3` | 구현됨 |
| `D3D_MakeRequestedSpace_4__LTCG_eax2` 외 20개 | 예정(목록은 reEXCEED 분석 문서 §9) |

## 3. 설계

* `HleCall`에 `Eax()`, `Ebx()`, `Esi()`, `Edi()`를 더한다.
* 공개 export의 본문을 평범한 인자의 핵심 함수로 옮기고(`CreateDevice(adapter, device_type, behavior, present, out)` → HRESULT), 공개 export와 변형 export는 인자를 모아 그 함수를 부르는 얇은 함수가 된다. 변형은 `Return(result, 스택 인자 수)`.

## 4. 검증

* probe·CTest가 그대로 통과(d3d8 probe 223).
* reEXCEED: `Direct3D_CreateDevice` 로그가 640x480, 백 버퍼 2, behavior `0x40`을 보이고 다음 정지점으로 넘어간다.

---

# Design: D3D8LTCG's register-convention variants (issue #50)

Status: **partly implemented** (`Direct3D_CreateDevice_4__LTCG_eax1_ecx3`). Work order: [20261103-i050](../work-orders/20261103-i050-d3d8-ltcg-variants.md) · Work log: [20261103-i050](../work-logs/20261103-i050-d3d8-ltcg-variants.md). Consumer: reexec/reEXCEED#4.

## 1. Requirements

Pump It Up: Exceed links XDK 5849's D3D8LTCG, of which XbSymbolDatabase identifies 22 functions as `<Name>_<stack bytes>__LTCG_<reg><param>...` variants. The HLE has none, so the boot stops at `Direct3D_CreateDevice_4__LTCG_eax1_ecx3`.

## 2. The convention (checked in Exceed)

**Confirmed** from the lifted `0x00177A70` and its call site `0x000766DF`: the parameter numbers count the LTCG function's **reduced signature**, not the public API's. `Direct3D_CreateDevice_4__LTCG_eax1_ecx3` is `(BehaviorFlags = eax, pPresentationParameters = the one stack argument, ppReturnedDeviceInterface = ecx)`, with Adapter, DeviceType and hFocusWindow folded into constants; `ret 4`, the result in eax. Each variant therefore needs its own mapping onto the public function, confirmed from its lifted body (**inferred**: the others follow the same rule). The table above tracks them.

## 3. Design

`HleCall` gains `Eax()`, `Ebx()`, `Esi()` and `Edi()`. A public export's body moves into a plain-argument core (`CreateDevice(adapter, device_type, behavior, present, out)` returning the HRESULT); the public export and each variant become thin functions that gather their arguments and call it, the variant with `Return(result, its stack argument count)`.

## 4. Verification

The probes and CTest pass unchanged (d3d8 probe 223); in reEXCEED the `Direct3D_CreateDevice` log shows 640x480, two back buffers and behavior `0x40`, and the run moves on to the next stop.
