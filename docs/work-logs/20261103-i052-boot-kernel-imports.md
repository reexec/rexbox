# 작업 로그: Exceed의 부팅이 닿는 커널 import (issue #52)

설계: [20261103-i052](../design/20261103-i052-boot-kernel-imports.md) · 작업 지시: [20261103-i052](../work-orders/20261103-i052-boot-kernel-imports.md) · 소비자: reexec/reEXCEED#4

## 한 일 (1차)

멈춘 순서: `KeQueryPerformanceCounter`(126) → `XcHMAC`(340) → `MmQueryStatistics`(181) → `XcRC4Key`(338). 각각 설계 표대로 구현했고, 짝인 `KeQueryPerformanceFrequency`(127)와 `XcRC4Crypt`(339)도 더했다.

## 검증

* Win32 x86 Debug: 오류 0, 경고 0(`MmQueryStatistics`의 지역 상수가 전역을 가리던 C4459를 고쳤다).
* kernel probe: 103 check(s), 0 failure(s).
* reEXCEED: 네 import를 지나 `KeInitializeInterrupt vector 52` 뒤 NIC MMIO(`0xFEF00110`) 쓰기에서 fault(issue #53으로).

---

# Work log: the kernel imports Exceed's boot reaches (issue #52)

Design: [20261103-i052](../design/20261103-i052-boot-kernel-imports.md) · Work order: [20261103-i052](../work-orders/20261103-i052-boot-kernel-imports.md) · Consumer: reexec/reEXCEED#4

Done (first pass): the stops came in the order `KeQueryPerformanceCounter` (126) → `XcHMAC` (340) → `MmQueryStatistics` (181) → `XcRC4Key` (338); each is implemented as the design's table says, with `KeQueryPerformanceFrequency` (127) and `XcRC4Crypt` (339) as their pairs. Verification: Win32 x86 Debug with 0 errors and 0 warnings (the C4459 of a local constant hiding the global in `MmQueryStatistics` fixed); the kernel probe passes 103 checks; reEXCEED passes the four imports and faults writing the NIC's MMIO (`0xFEF00110`) after `KeInitializeInterrupt vector 52` (on to issue #53).
