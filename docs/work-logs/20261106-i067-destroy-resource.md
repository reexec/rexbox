# 작업 로그: D3D_DestroyResource가 라이브러리 본문으로 해제한다 (issue #67)

설계: [20261106-i067](../design/20261106-i067-destroy-resource.md) · 소비자: reexec/reEXCEED#9. 작업 지시는 설계 §2로 갈음한다.

## 한 일

* 추적: 진단용 임시 코드(커밋하지 않음)로 64 KiB 이상 할당과 해제를 게스트 호출 경로와 함께 기록했다.
  * 살아 있는 큰 블록 183개(41.6 MiB)가 모두 `XMemAlloc` → `MmAllocateContiguousMemoryEx`에서 왔고, 해제는 5번뿐이었다.
  * 가장 큰 몫은 인라인 텍스처 생성(`0x00076D50`)의 데이터였다.
  * 정지 덤프에서 남은 텍스처 329개 가운데 175개만 게임의 슬롯 표가 가리켰다.
* `include/rexbox/runtime/hle_registry.h`, `src/runtime/hle_registry.cpp`: `SetLiftedOriginalRunner`, `RunLiftedOriginal`.
* `src/recomp/recomp_bridge.cpp`: `t_running_slot`, `RunSlotExport`, `RunRunningSlotOriginal`.
* `src/hle/d3d8/d3d8_resource.{h,cpp}`: `DestroyResourceHostSide`, `DestroyReleasedResource`(`D3D_DestroyResource` 변수 바인딩, `CallGuest`).
* `src/hle/d3d8/d3d8_query.cpp`: export가 원래 본문을 실행한다.

## 검증

* Win32 x86 Debug·Release(reEXCEED 트리): 오류 0, 경고 0. CTest 7/7.
* reEXCEED: #68과 함께 곡 선택 → 본게임(BGA) → GAME OVER 영상 → SELECT MODE, 181초 동안 메모리 고갈과 멈춤 없음.

---

# Work log: D3D_DestroyResource frees through the library body (issue #67)

Design: [20261106-i067](../design/20261106-i067-destroy-resource.md) · Consumer: reexec/reEXCEED#9. The design's section 2 stands in for a work order.

Done:
- Tracing, with temporary diagnostic code (not committed) that logged allocations and frees of 64 KiB and more with their guest call paths:
  - the 183 big blocks still live (41.6 MiB) all came from `XMemAlloc` → `MmAllocateContiguousMemoryEx`, with 5 frees in total;
  - most of the memory was the data of inline texture creation (`0x00076D50`);
  - in a stop dump, only 175 of the 329 textures left were still referenced by the game's slot table.
- `include/rexbox/runtime/hle_registry.h` and `src/runtime/hle_registry.cpp`: `SetLiftedOriginalRunner` and `RunLiftedOriginal`.
- `src/recomp/recomp_bridge.cpp`: `t_running_slot`, `RunSlotExport` and `RunRunningSlotOriginal`.
- `src/hle/d3d8/d3d8_resource.{h,cpp}`: `DestroyResourceHostSide` and `DestroyReleasedResource` (the `D3D_DestroyResource` variable binding, `CallGuest`).
- `src/hle/d3d8/d3d8_query.cpp`: the export runs the original body.

Verification:
- Win32 x86 Debug and Release (in the reEXCEED tree): 0 errors and 0 warnings, CTest 7/7.
- In reEXCEED, with #68: song select → gameplay (BGA) → the GAME OVER video → SELECT MODE, with no memory exhaustion and no stop in 181 s.
