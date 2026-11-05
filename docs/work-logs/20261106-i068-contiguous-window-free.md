# 작업 로그: 연속 메모리 창 주소로 해제·조회 (issue #68)

설계: [20261106-i068](../design/20261106-i068-contiguous-window-free.md) · 소비자: reexec/reEXCEED#9. 작업 지시는 설계 §2로 갈음한다.

## 한 일

* 추적: #67을 고친 뒤 해제는 173번으로 늘었는데 남은 메모리는 그대로였다. 168번이 `0x8xxxxxxx` 주소로 들어와 블록을 찾지 못했다.
* `src/hle/kernel/kernel_memory.cpp`: `HeapAddress`. `src/hle/d3d8/d3d8_create.cpp`: `D3D_FreeContiguousMemory`가 주소를 접는다.

## 검증

* Win32 x86 Debug·Release(reEXCEED 트리): 오류 0, 경고 0. CTest 7/7.
* reEXCEED: `PsCreateSystemThreadEx guest memory exhausted`가 더 나오지 않는다. 본게임의 BGA와 GAME OVER 영상이 그려진다.

---

# Work log: frees and queries through the contiguous-memory window (issue #68)

Design: [20261106-i068](../design/20261106-i068-contiguous-window-free.md) · Consumer: reexec/reEXCEED#9. The design's section 2 stands in for a work order.

Done: tracing — after #67 the frees rose to 173 while the memory left stayed the same; 168 of them came in as `0x8xxxxxxx` and found no block. `src/hle/kernel/kernel_memory.cpp`: `HeapAddress`; `src/hle/d3d8/d3d8_create.cpp`: `D3D_FreeContiguousMemory` folds the address.

Verification: Win32 x86 Debug and Release (in the reEXCEED tree) with 0 errors and 0 warnings, CTest 7/7. In reEXCEED `PsCreateSystemThreadEx guest memory exhausted` no longer appears, and the in-game BGA and the GAME OVER video draw.
