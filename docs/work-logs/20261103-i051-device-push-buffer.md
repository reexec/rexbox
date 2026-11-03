# 작업 로그: 타이틀이 직접 쓰는 디바이스 푸시 버퍼 (issue #51)

설계: [20261103-i051](../design/20261103-i051-device-push-buffer.md) · 작업 지시: [20261103-i051](../work-orders/20261103-i051-device-push-buffer.md) · 소비자: reexec/reEXCEED#4

## 한 일 (1단계)

* 디바이스 푸시 버퍼: 게스트 힙 512 KiB. 디바이스 `+0`/`+4`/`+0x24`/`+0x28`에 `pPut`, 한계(끝 − 16 KiB), 시작, 끝을 둔다. flush는 `[flushed, pPut)`를 i007의 `Execute`로 해석한다.
* `D3D_MakeRequestedSpace_4__LTCG_eax2`: flush 뒤 `pPut`을 시작으로 되돌리고 그 주소를 돌려준다(`ret 4`). `CDevice_KickOff_0__LTCG_edx1`: flush.
* flush 지점: `Clear`, `Swap`, `DrawVertices`, `DrawIndexedVertices`, `DrawVerticesUP`, `DrawIndexedVerticesUP`, `BeginPush`.
* issue #50과 함께: `d3d8_ltcg.cpp`의 표 기반 어댑터. 변형이 받은 레지스터·스택 인자를 공개 함수의 stdcall 프레임(`[반환 주소, P1..Pn]`, 호출자의 반환 뒤 esp에서 끝남)으로 다시 쌓고 공개 export를 부른다. 12개.

## 검증

* Win32 x86 Debug: 오류 0, 경고 0. CTest 7/7.
* reEXCEED: `MakeRequestedSpace`를 지나고, LTCG `Swap`으로 **첫 프레임이 present**된다(`[d3d8] first frame presented`). 해석하지 않은 메서드: `030C`, `1D78`(첫 값 0).

---

# Work log: the device push buffer the title writes itself (issue #51)

Design: [20261103-i051](../design/20261103-i051-device-push-buffer.md) · Work order: [20261103-i051](../work-orders/20261103-i051-device-push-buffer.md) · Consumer: reexec/reEXCEED#4

## Done (stage 1)

The device push buffer is 512 KiB of guest heap with `pPut`, the threshold (16 KiB before the end), the base and the end at device `+0`, `+4`, `+0x24` and `+0x28`; a flush interprets `[flushed, pPut)` with i007's `Execute`. `D3D_MakeRequestedSpace_4__LTCG_eax2` flushes, rewinds `pPut` to the base and returns it (`ret 4`); `CDevice_KickOff_0__LTCG_edx1` flushes. `Clear`, `Swap`, the four draw exports and `BeginPush` flush first. With issue #50, `d3d8_ltcg.cpp` re-frames twelve variants from a table: the registers and stack arguments the variant received become the public function's stdcall frame (`[return address, P1..Pn]`, ending at the caller's esp after the return) and the public export runs.

## Verification

Win32 x86 Debug: 0 errors, 0 warnings; CTest 7/7. In reEXCEED the run passes `MakeRequestedSpace` and the LTCG `Swap` **presents the first frame** (`[d3d8] first frame presented`); uninterpreted methods: `030C` and `1D78` (first value 0).
