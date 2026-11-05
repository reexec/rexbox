# 설계: 연속 메모리 창 주소로 해제·조회 (issue #68)

상태: **구현됨**. 작업 로그: [20261106-i068](../work-logs/20261106-i068-contiguous-window-free.md). 소비자: reexec/reEXCEED#9.

## 1. 요구사항

타이틀과 라이브러리는 연속 메모리를 커널의 물리 RAM 창(`0x80000000 | 물리 주소`)으로도 다룬다. XDK의 D3D_DestroyResource는 텍스처 데이터를 `XMemFree(data | 0x80000000, ...)`로 해제한다. rexbox의 `MmFreeContiguousMemory`는 받은 주소 그대로 블록을 찾아서, 이런 해제가 조용히 버려졌다. reEXCEED에서는 연속 메모리 해제 173번 가운데 168번이 그랬다(**확인됨**).

## 2. 설계

* 커널의 `HeapAddress(address)` = `GuestMemory::Fold`(`0x80000000` 창과 write-combined `0xF0000000` 창을 RAM 아래 주소로).
* 이것을 `MmFreeContiguousMemory`, `MmFreeSystemMemory`, `MmQueryAllocationSize`, `MmGetPhysicalAddress`(접은 주소를 돌려준다), `MmQueryAddressProtect`, `MmSetAddressProtect`에 쓴다. D3D8의 `D3D_FreeContiguousMemory`도 접는다.
* 창이 꺼진 메모리에서는 `Fold`가 주소를 그대로 두므로 전과 같다.

## 3. 검증

* kernel·d3d8 probe와 CTest. reEXCEED에서 해제가 블록을 찾는다(#67과 함께, 곡 진행 뒤에도 메모리가 남는다).

---

# Design: frees and queries through the contiguous-memory window (issue #68)

Status: **implemented**. Work log: [20261106-i068](../work-logs/20261106-i068-contiguous-window-free.md). Consumer: reexec/reEXCEED#9.

**Requirement.** Titles and the libraries also handle contiguous memory through the kernel's window onto physical RAM (`0x80000000 | physical`). The XDK's D3D_DestroyResource frees texture data as `XMemFree(data | 0x80000000, ...)`. rexbox's `MmFreeContiguousMemory` looked the block up by the address as given, so such frees were silently dropped. In reEXCEED that was 168 of 173 contiguous frees (**confirmed**).

**Design.** The kernel's `HeapAddress(address)` is `GuestMemory::Fold`: the `0x80000000` window and the write-combined `0xF0000000` window map to the address below RAM. It is applied in:
- `MmFreeContiguousMemory` and `MmFreeSystemMemory`
- `MmQueryAllocationSize`, `MmQueryAddressProtect` and `MmSetAddressProtect`
- `MmGetPhysicalAddress`, which returns the folded address
- D3D8's `D3D_FreeContiguousMemory`

With the window off, `Fold` leaves addresses as they are, so nothing changes.

**Verification.** The kernel and d3d8 probes and CTest. In reEXCEED the frees now find their blocks (with #67, memory is left after a song).
