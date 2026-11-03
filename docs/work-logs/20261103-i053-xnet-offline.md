# 작업 로그: XNET 오프라인 (issue #53)

설계: [20261103-i053](../design/20261103-i053-xnet-offline.md) · 작업 지시: [20261103-i053](../work-orders/20261103-i053-xnet-offline.md) · 소비자: reexec/reEXCEED#4

## 한 일

xapi HLE 모듈의 첫 export로 `XNetStartup`(`WSAENETDOWN`)과 `XNetGetEthernetLinkStatus`(0)를 더했다. reEXCEED가 `hle_libraries.json`에서 두 함수를 고른다(XONLINES, HLE override 268 → 270).

## 검증

* Win32 x86 Debug: 오류 0, 경고 0.
* reEXCEED: `[xnet] XNetStartup: no network on this host (WSAENETDOWN); offline`. NIC MMIO fault 없이 지나가고, 다음 정지점은 DSOUND `DirectSoundCreateStream`(`000EBD9E`).
* xapi 모듈에는 probe가 없다. 두 함수는 상수를 돌려줄 뿐이라 reEXCEED 실행으로만 확인했다.

---

# Work log: XNET offline (issue #53)

Design: [20261103-i053](../design/20261103-i053-xnet-offline.md) · Work order: [20261103-i053](../work-orders/20261103-i053-xnet-offline.md) · Consumer: reexec/reEXCEED#4

The xapi HLE module gains its first exports, `XNetStartup` (`WSAENETDOWN`) and `XNetGetEthernetLinkStatus` (0); reEXCEED selects both in its `hle_libraries.json` (XONLINES; HLE overrides 268 → 270). Verification: Win32 x86 Debug with 0 errors and 0 warnings; reEXCEED prints `[xnet] XNetStartup: no network on this host (WSAENETDOWN); offline`, passes without the NIC MMIO fault and next stops at DSOUND `DirectSoundCreateStream` (`000EBD9E`). The xapi module has no probe; the two functions return constants and were checked by the reEXCEED run only.
