# 작업 로그: D3D8 펜스 export — 완료 (issue #19)

작업 지시: [20261021-i019-fence-exports](../work-orders/20261021-i019-fence-exports.md) · 설계: [20261021-i019-fence-exports](../design/20261021-i019-fence-exports.md) · 소비자: reexec/reCT3#14

## 한 일

1. 소비자의 lift된 4531 라이브러리에서 `D3DDevice_InsertFence`(`000F5050`)가 `D3D_SetFence(0)`이고 `D3DDevice_BlockOnFence`(`000F4F40`)가 `D3D_BlockOnTime(f, 0)`임을 확인했다. `D3DDevice_IsFencePending`은 소비자에 없다(XbSymbolDatabase 4039 서명이 4531에 쓰인다).
2. `d3d8_device.cpp`: `D3D_SetFence` 옆에 세 export를 두었다. `InsertFence`는 `D3D_SetFence`와 같은 카운터를 올려 돌려주고, `BlockOnFence`는 바로 돌아오며, `IsFencePending`은 0이다.
3. kb `xbox-d3d8-api.md` §5에 펜스 항목을 더했다.
4. 작업 중 `main`이 v0.0.7(`0aa0126`, Swap의 flip 대기열)로 올라가 브랜치를 그 위로 옮겼다.

## 검증

| 검증 | 결과 |
|---|---|
| Win32 x86 Release 빌드(소비자 트리) | 오류 0 |
| CTest(소비자 트리) | 5/5(rexbox kernel, dsound, d3d8 probe 포함) |
| 기준: 이 변경 없는 소비자 exe(HLE export 314개), `REXBOX_INPUT=keyboard`, Start | 스레드 2가 `stopped at unimplemented XDK function D3D8 D3DDevice_InsertFence (000F5050)`, 70초에 종료 |
| 이 변경(export 317개), `REXBOX_HLE_FALLBACK` 없음, 240초 | 미구현 HLE 정지 없음. frame 1200 모드 선택 → 1500 West Coast → 1800 로딩 팁 → 2100부터 운전(제한 시간 50 → 36, 교통 이동), 60 fps |

`IsFencePending`은 소비자가 링크하지 않아 실행으로 확인하지 못했다.

---

# Work log: the D3D8 fence exports — done (issue #19)

Work order: [20261021-i019-fence-exports](../work-orders/20261021-i019-fence-exports.md) · Design: [20261021-i019-fence-exports](../design/20261021-i019-fence-exports.md) · Consumer: reexec/reCT3#14

Done: the consumer's lifted 4531 library confirms that `D3DDevice_InsertFence` (`000F5050`) is `D3D_SetFence(0)` and `D3DDevice_BlockOnFence` (`000F4F40`) is `D3D_BlockOnTime(f, 0)`; the consumer has no `D3DDevice_IsFencePending` (4531 would use the XbSymbolDatabase 4039 signature). `d3d8_device.cpp` gained the three exports next to `D3D_SetFence`: `InsertFence` increments and returns `D3D_SetFence`'s counter, `BlockOnFence` returns at once, and `IsFencePending` returns 0. The kb `xbox-d3d8-api.md` section 5 gained a fence entry. `main` moved to v0.0.7 (`0aa0126`, Swap's flip queue) during the task, and the branch was rebased onto it.

Verification: Win32 x86 Release builds with no errors in the consumer tree; CTest 5/5 (including the rexbox kernel, dsound and d3d8 probes). Baseline: the consumer exe without this change (314 HLE exports), `REXBOX_INPUT=keyboard`, Start: thread 2 is `stopped at unimplemented XDK function D3D8 D3DDevice_InsertFence (000F5050)` and the process ends at 70 s. With this change (317 exports), no `REXBOX_HLE_FALLBACK`, 240 s: no unimplemented-HLE stop; frame 1200 shows mode selection, 1500 West Coast, 1800 a loading tip, and from 2100 on the taxi drives (time limit 50 down to 36, traffic moving) at 60 fps. `IsFencePending` is not linked by the consumer, so no run exercised it.
