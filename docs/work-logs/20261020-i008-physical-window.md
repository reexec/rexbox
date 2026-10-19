# 작업 로그: 물리 메모리 창 (issue #8)

설계: [20261020-i008](../design/20261020-i008-physical-window.md) · 작업 지시: [20261020-i008](../work-orders/20261020-i008-physical-window.md)

## 한 일

1. 플랫폼 계약 `AllocateGuestSpace`/`FreeGuestSpace`(`include/rexbox/platform/virtual_memory.h`). Win32: pagefile 섹션의 뷰를 base, base + 0x80000000, base + 0xF0000000(32비트에서 wrap)에 두고, 후보 base를 `VirtualQuery`로 훑는다. Linux: `memfd_create`와 `MAP_FIXED_NOREPLACE` claim 뒤 `MAP_SHARED | MAP_FIXED` 뷰.
2. `GuestMemory`: `Options{physical_window}`, `Initialize(size, options)` 오버로드(GCC는 중첩 구조체의 기본 멤버 초기화를 기본 인자에 쓰지 못한다), `Fold`, `Translate`/`IsValid`, 창을 켠 메모리는 쓰기 추적 없음.
3. `ThreadReport::host_exception_address`(fault의 호스트 명령 주소).
4. `rexbox_kernel_probe`에 창 검사 7개(`ProbeGuestWindow`). probe는 `/LARGEADDRESSAWARE`로 링크한다(32비트에서 창이 2 GiB 위에 놓인다).
5. 문서: 설계, `docs/kb/xbox-memory-map.md`, kb 색인, `ARCHITECTURE.md`.

## 검증

| 검증 | 결과 |
|---|---|
| Win32 x86 Debug: `rexbox_kernel_probe` | 63 check, 0 failure (창 검사 7개 포함); `ctest` 3/3 |
| Linux x64 / x86 Debug / Release: `rexbox_kernel_probe` | 63 check, 0 failure 각각 |
| reCT3(소비자, Win32 Debug, 창 켬) | `806E6000` fault를 지나 텍스처 읽기·DSOUND 생성까지 진행. 다음 frontier는 DSOUND의 `XAudioDownloadEffectsImage`(#9) |

---

# Work log: the physical memory window (issue #8)

Design: [20261020-i008](../design/20261020-i008-physical-window.md) · Work order: [20261020-i008](../work-orders/20261020-i008-physical-window.md)

## Done

The platform contract `AllocateGuestSpace`/`FreeGuestSpace` (Win32: views of a pagefile section at base, base + 0x80000000 and base + 0xF0000000, wrapping on 32-bit, with candidate bases scanned through `VirtualQuery`; Linux: `memfd_create`, `MAP_FIXED_NOREPLACE` claims, then `MAP_SHARED | MAP_FIXED` views); `GuestMemory` with `Options{physical_window}`, an `Initialize(size, options)` overload (GCC cannot use a nested struct's default member initializer in a default argument), `Fold`, `Translate`/`IsValid`, and no write tracking with the window; `ThreadReport::host_exception_address`; seven window checks in `rexbox_kernel_probe`, which now links large-address-aware; the design, `docs/kb/xbox-memory-map.md`, the kb index and `ARCHITECTURE.md`.

## Verification

Win32 x86 Debug: `rexbox_kernel_probe` 63 checks, 0 failures (the seven window checks included), `ctest` 3/3; Linux x64 and x86, Debug and Release: 63 checks, 0 failures each; reCT3 (Win32 Debug, window on) passes the `806E6000` fault and reaches texture loading and DirectSound creation; its next frontier is DSOUND's `XAudioDownloadEffectsImage` (#9).
