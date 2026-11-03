# 작업 로그: HLE 스냅숏으로 하는 상태 블록 (issue #57)

설계: [20261103-i057](../design/20261103-i057-state-blocks.md) · 소비자: reexec/reEXCEED#4. 작업 지시는 설계 §2로 갈음한다.

## 한 일

* `d3d8_stateblock.cpp`(새 파일): `CreateStateBlock`, `ApplyStateBlock`, `CaptureStateBlock`, `DeleteStateBlock`.
* 같은 세션의 다른 변경(같은 브랜치, 각 issue의 문서에 기록): `Lock2DSurface_16__LTCG_esi4_eax5`(#56: 스택 pPixels·FaceType·Level·Flags, esi pLockedRect, eax pRect, `ret 16`, 큐브 면 오프셋은 보고만), HLE 메서드 합성 주소를 `0xFF000000`(`kHleMethodVaBase`)으로 옮기고 `IsKernelThunkVa`를 그 아래로 좁힘(#54: `0xFD000000`은 lifter의 간접 호출 검사에서 "outside code"로 건너뛰어졌다).

## 검증

* Win32 x86 Release(reEXCEED 트리): 오류 0, 경고 0. CTest 7/7.
* reEXCEED: 프레임 600 캡처가 `vs 00000152 … vp 0,0 640x480 … cw 01010101`. 스트림 vtable 호출이 HLE에 닿는다("outside code" 경고 3건이 사라짐). 화면은 아직 검다: 캡처의 그리기는 검은 페이드 사각형(정점 색 `FF000000`, 텍스처 없음)과 높이 0인 텍스처 사각형뿐이다(reEXCEED 작업 로그 i004에서 이어서 본다).

---

# Work log: state blocks as HLE snapshots (issue #57)

Design: [20261103-i057](../design/20261103-i057-state-blocks.md) · Consumer: reexec/reEXCEED#4. The design's section 2 stands in for a work order.

Done: `d3d8_stateblock.cpp` (new) with `CreateStateBlock`, `ApplyStateBlock`, `CaptureStateBlock` and `DeleteStateBlock`. Other changes of the same session (same branch, recorded in each issue's documents): `Lock2DSurface_16__LTCG_esi4_eax5` (#56: pPixels, FaceType, Level and Flags on the stack, pLockedRect in esi, pRect in eax, `ret 16`, the cube face offset only reported); the HLE method synthetic addresses moved to `0xFF000000` (`kHleMethodVaBase`) with `IsKernelThunkVa` stopping below it (#54: `0xFD000000` was skipped as "outside code" by the lifter's indirect-call check).

Verification: Win32 x86 Release (in the reEXCEED tree) with 0 errors and 0 warnings, CTest 7/7. In reEXCEED the frame-600 capture reads `vs 00000152 ... vp 0,0 640x480 ... cw 01010101`, and the stream vtable calls reach the HLE (the three "outside code" warnings are gone). The picture is still black: the captured draws are black fade quads (vertex color `FF000000`, no texture) and a zero-height textured quad (followed up in reEXCEED's work log i004).
