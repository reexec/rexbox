# 작업 로그: NtQueryFullAttributesFile (issue #69)

설계: [20261106-i069](../design/20261106-i069-query-full-attributes.md) · 소비자: reexec/reEXCEED#9. 작업 지시는 설계 §2로 갈음한다.

## 한 일

* `src/hle/kernel/kernel_file.cpp`: `NtQueryFullAttributesFile`과 export 등록. 정지 덤프에서 물은 이름은 `U:\33A446FC39CC\`(세이브 폴더)였다.

## 검증

* Win32 x86 Debug·Release(reEXCEED 트리): 오류 0, 경고 0. CTest 7/7. 커널 import 112/146 바인딩.
* reEXCEED: GAME OVER 뒤 저장을 지나 SELECT MODE로 돌아온다.

---

# Work log: NtQueryFullAttributesFile (issue #69)

Design: [20261106-i069](../design/20261106-i069-query-full-attributes.md) · Consumer: reexec/reEXCEED#9. The design's section 2 stands in for a work order.

Done: `src/hle/kernel/kernel_file.cpp` gains `NtQueryFullAttributesFile` and its export; the name asked in the stop dump was `U:\33A446FC39CC\` (the save folder).

Verification: Win32 x86 Debug and Release (in the reEXCEED tree) with 0 errors and 0 warnings, CTest 7/7, 112/146 kernel imports bound. In reEXCEED the save after GAME OVER goes through and the game returns to SELECT MODE.
