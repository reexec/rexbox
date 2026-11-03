# 설계: Pump It Up: Exceed의 부팅이 닿는 커널 import (issue #52)

상태: **진행 중**. 작업 지시: [20261103-i052](../work-orders/20261103-i052-boot-kernel-imports.md) · 작업 로그: [20261103-i052](../work-logs/20261103-i052-boot-kernel-imports.md). 소비자: reexec/reEXCEED#4.

## 1. 요구사항

Exceed(XDK 5849)의 커널 import 146개 중 42개가 v0.0.21의 HLE에 없다. 부팅이 실제로 닿는 것만, 닿는 순서대로 구현한다.

## 2. 설계

| import | 구현 | 근거 |
|---|---|---|
| 126 `KeQueryPerformanceCounter` | `InterruptTime100ns() × 27 / 80`(3.375 MHz 틱) | ACPI 타이머 3,375,000 Hz(**추정**, Cxbx-Reloaded의 상수를 지식으로만 참고) |
| 127 `KeQueryPerformanceFrequency` | 3,375,000 | 같음 |
| 340 `XcHMAC` | HMAC-SHA1(RFC 2104). 키가 64바이트보다 길면 먼저 SHA-1. 입력은 pbData 다음 pbData2. 호스트 SHA-1(`HostSha1`)은 기존 `Transform`을 쓴다 | XDK 서명(7인자 stdcall) |
| 181 `MmQueryStatistics` | `Length`가 0x24가 아니면 `STATUS_INVALID_PARAMETER`. 총 16384쪽(64 MiB), 사용 가능 = 힙의 빈 바이트(64 MiB 상한), 나머지는 committed | `MM_STATISTICS` 배치 |
| 338 `XcRC4Key`, 339 `XcRC4Crypt` | RC4. 키 구조체는 `S[256]`, `i`, `j`(258바이트). 타이틀은 그 안을 보지 않는다(**추정**) | RC4 정의 |

## 3. 검증

kernel probe: 성능 카운터의 주파수와 진행, `XcHMAC`(RFC 2202 case 2, 입력을 둘로 나눔), `MmQueryStatistics`(쪽 수, 잘못된 Length), RC4(RFC 6229 키 `0102030405`, 두 번에 나눈 호출). 94 → 103 검사.

---

# Design: the kernel imports Pump It Up: Exceed's boot reaches (issue #52)

Status: **in progress**. Work order: [20261103-i052](../work-orders/20261103-i052-boot-kernel-imports.md) · Work log: [20261103-i052](../work-logs/20261103-i052-boot-kernel-imports.md). Consumer: reexec/reEXCEED#4.

Exceed (XDK 5849) has 42 of its 146 kernel imports without an HLE export in v0.0.21; only those its boot reaches are implemented, in the order it reaches them. The table above lists them: `KeQueryPerformanceCounter` (126) as `InterruptTime100ns() x 27 / 80` and `KeQueryPerformanceFrequency` (127) as 3,375,000, the ACPI timer (**inferred**, Cxbx-Reloaded's constant used as knowledge only); `XcHMAC` (340) as HMAC-SHA1 (RFC 2104) over pbData then pbData2, hashing keys longer than 64 bytes first, on a host SHA-1 (`HostSha1`) built from the existing `Transform`; `MmQueryStatistics` (181) rejecting a `Length` other than 0x24 and reporting 16384 pages (64 MiB), the heap's free bytes (capped at 64 MiB) as available and the rest as committed; `XcRC4Key`/`XcRC4Crypt` (338/339) as RC4 with a key structure of `S[256]`, `i` and `j` (258 bytes) that titles do not look into (**inferred**). The kernel probe checks the counter's frequency and progress, `XcHMAC` (RFC 2202 case 2 split over both inputs), `MmQueryStatistics` (page counts, a wrong Length) and RC4 (RFC 6229's key `0102030405` over two calls): 94 → 103 checks.
