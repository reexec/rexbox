# 설계: D3D_DestroyResource가 라이브러리 본문으로 해제한다 (issue #67)

상태: **구현됨**. 작업 로그: [20261106-i067](../work-logs/20261106-i067-destroy-resource.md). 소비자: reexec/reEXCEED#9.

## 1. 요구사항

`D3D_DestroyResource_0__LTCG_edi1` HLE(#56)는 아무것도 해제하지 않았다. 원래 라이브러리 본문은 `D3D_BlockOnResource`와 할당 기록 정리를 마친 뒤, 데이터와 헤더를 타이틀의 할당자로 해제한다(`XMemFree(data, 0xA1800000)`, `XMemFree(header, 0x24800000)`). `D3DResource_Release` HLE도 참조 수가 0이 되면 HLE가 만든 메모리만 해제했다. Pump It Up: Exceed는 텍스처를 인라인으로 만든다(`XMemAlloc`). 슬롯을 다시 쓸 때마다 놓은 텍스처가 해제되지 않아, 화면 몇 개 만에 게스트 메모리가 바닥났다(**확인됨**).

## 2. 설계

* runtime: `RunLiftedOriginal()`. 지금 이 스레드에서 도는 generated override의 lift 본문을, export가 받은 게스트 상태 그대로 실행한다. 원래 본문이 호출자에게 돌아가므로 export는 그 뒤 `Return`하지 않는다. 브리지가 실행 중인 슬롯을 스레드별로 기억한다(`t_running_slot`). 본문 안에서는 슬롯을 비운다.
* `DestroyResourceHostSide(resource)`:
  * HLE가 만든 리소스와 장치의 표면은 HLE 경로(`DestroyResource`)로 처리하고 false를 돌려준다.
  * 타이틀이 만든 리소스는 호스트 상태(호스트 텍스처, 렌더 타깃 메모리, 기록)만 지우고 true를 돌려준다.
* `D3D_DestroyResource` export: `DestroyResourceHostSide`가 true면 `RunLiftedOriginal()`을 실행한다.
* `Release`와 바인딩 해제에서 참조가 0이 된 타이틀 리소스: 타이틀 프로파일이 XDK 내부 주소 `D3D_DestroyResource`를 바인딩했으면, EDI에 리소스를 넣고 그 주소를 `CallGuest`로 부른다. dispatch를 거쳐 override 슬롯로 들어가므로 위의 export가 실행된다. 바인딩이 없으면 전과 같다.

## 3. 검증

* d3d8·kernel probe와 CTest(바인딩이 없으면 동작이 같다).
* reEXCEED에서 곡 선택 → 본게임 → GAME OVER를 지나도 메모리가 바닥나지 않는다(#68과 함께).

---

# Design: D3D_DestroyResource frees through the library body (issue #67)

Status: **implemented**. Work log: [20261106-i067](../work-logs/20261106-i067-destroy-resource.md). Consumer: reexec/reEXCEED#9.

**Requirement.** The `D3D_DestroyResource_0__LTCG_edi1` HLE (#56) freed nothing. The library's own body, after `D3D_BlockOnResource` and its allocation bookkeeping, frees the data and the header through the title's allocator (`XMemFree(data, 0xA1800000)`, `XMemFree(header, 0x24800000)`). The `D3DResource_Release` HLE likewise freed only HLE-made memory when the count reached 0. Pump It Up: Exceed creates textures inline (`XMemAlloc`), so every texture dropped when a slot was reused stayed allocated, and guest memory ran out after a few screens (**confirmed**).

**Design.**
- Runtime: `RunLiftedOriginal()` runs the lifted body of the generated override whose export is running on this thread, on the guest state as the export found it. The original returns to the caller, so the export must not `Return` afterwards. The bridge records the running slot per thread (`t_running_slot`) and clears it while the body runs.
- `DestroyResourceHostSide(resource)`:
  - an HLE-made resource, or a device surface, takes the HLE path (`DestroyResource`) and returns false;
  - a title-made resource only loses its host state (host texture, render-target memory, the record) and returns true.
- The `D3D_DestroyResource` export runs `RunLiftedOriginal()` when `DestroyResourceHostSide` returns true.
- A title-made resource whose last reference goes in `Release` or in a released binding: if the title profile binds the XDK internal `D3D_DestroyResource`, the resource goes into EDI and that address is called through `CallGuest`. The call reaches the override slot through the dispatch, so the export above runs. Without the binding nothing changes.

**Verification.** The d3d8 and kernel probes and CTest (behavior without the binding is unchanged). In reEXCEED, song select → gameplay → GAME OVER no longer runs guest memory out (with #68).
