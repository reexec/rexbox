# 설계: 호스트에는 Xbox 네트워크가 없다 — XNET 오프라인 (issue #53)

상태: **구현됨**. 작업 지시: [20261103-i053](../work-orders/20261103-i053-xnet-offline.md) · 작업 로그: [20261103-i053](../work-logs/20261103-i053-xnet-offline.md). 소비자: reexec/reEXCEED#4.

## 1. 요구사항

Exceed는 부팅 중 `XNetStartup`을 부르고, lift된 XNET은 `KeInitializeInterrupt`(벡터 52) 뒤 NIC를 MMIO(`0xFEF00000`)로 프로그래밍하다 게스트 RAM 밖에 써서 fault한다. 타이틀에는 오프라인 경로가 이미 있다: 네트워크 초기화(`0x00080E70`)가 `XNetStartup` 실패 시 `E_FAIL`을 돌려주고, 호출자는 XOnline 시작을 건너뛴다(**확인됨**, lift 코드).

## 2. 설계

| 방안 | 판단 |
|---|---|
| **API 경계: `XNetStartup` 실패, 링크 없음** | **채택**. 타이틀이 가진 오프라인 경로를 탄다. 하드웨어를 흉내 내지 않는다(AGENTS 핵심 목표 2) |
| NIC LLE(레지스터, 인터럽트, PHY) | 하지 않는다. 범위가 크고, 호스트 네트워크 연결은 목표가 아니다 |

* xapi HLE 모듈에 `XNetStartup`(stdcall 1인자) → `WSAENETDOWN`(10050), `XNetGetEthernetLinkStatus`(인자 0) → 0(케이블 없음)을 둔다.
* 실기에서 케이블이 없어도 `XNetStartup`은 성공한다(NIC가 있으므로). 여기서는 실패를 돌려주므로 원본과 다르다. 네트워크 기능(Xbox Live, 시스템 링크)을 쓸 수 없다는 뜻이며 이를 문서에 남긴다.
* 타이틀은 `hle_libraries.json`에서 두 함수를 고른다(XbSymbolDatabase의 라이브러리 이름 `XONLINES`).

## 3. 검증

reEXCEED: `[xnet] XNetStartup: no network on this host` 한 줄이 나오고, NIC MMIO fault 없이 다음 정지점으로 간다.

---

# Design: the host has no Xbox network — XNET offline (issue #53)

Status: **implemented**. Work order: [20261103-i053](../work-orders/20261103-i053-xnet-offline.md) · Work log: [20261103-i053](../work-logs/20261103-i053-xnet-offline.md). Consumer: reexec/reEXCEED#4.

Exceed calls `XNetStartup` at boot, and the lifted XNET programs the NIC through its MMIO window (`0xFEF00000`) after `KeInitializeInterrupt` (vector 52), faulting outside guest RAM. The title already has an offline path: its network init (`0x00080E70`) returns `E_FAIL` when `XNetStartup` fails, and its caller skips XOnline's start-up (**confirmed** in the lifted code). Of the options in the table, the API boundary is taken (no hardware imitation, AGENTS goal 2); NIC LLE (registers, interrupts, the PHY) is not done. The xapi HLE module exports `XNetStartup` (stdcall, one argument) returning `WSAENETDOWN` (10050) and `XNetGetEthernetLinkStatus` (no arguments) returning 0 (no cable). On a console without a cable `XNetStartup` succeeds (the NIC is there), so this differs from the original: network features (Xbox Live, system link) are unavailable, as documented. A title selects both in its `hle_libraries.json` (library `XONLINES`, XbSymbolDatabase's name). Verification: reEXCEED prints one `[xnet] XNetStartup: no network on this host` line and reaches its next stop without the NIC fault.
