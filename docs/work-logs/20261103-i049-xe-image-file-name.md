# 작업 로그: 커널 데이터 export `XeImageFileName` (issue #49)

설계: [20261103-i049](../design/20261103-i049-xe-image-file-name.md) · 작업 지시: [20261103-i049](../work-orders/20261103-i049-xe-image-file-name.md) · 소비자: reexec/reEXCEED#4

## 한 일

* `KernelConfig::image_file_name`(기본 `default.xbe`)과 공개 함수 `DataExportAddress(name)`을 더했다.
* 데이터 export 표에 `XeImageFileName`(8바이트 `ANSI_STRING`)을 더하고, `InitializeKernel`이 `SetImageFileName`으로 `\Device\CdRom0\<파일 이름>`과 NUL을 게스트 힙에 쓴 뒤 `Length`(NUL 제외), `MaximumLength`(NUL 포함), `Buffer`를 채운다.
* kernel probe에 `ProbeImageFileName`(검사 3개)을 더했다.

## 검증

* Win32 x86 Debug(reEXCEED 트리 안에서, Visual Studio 2026): 오류 0, 경고 0.
* `rexbox_kernel_probe`: 97 check(s), 0 failure(s)(94 → 97).
* reEXCEED(`image_file_name = DEFAULT.XBE`): XAPI 시작 코드가 fault 없이 이 지점을 지나 Z: 캐시 디렉터리를 만들고, 다음 정지점은 D3D8LTCG `Direct3D_CreateDevice_4__LTCG_eax1_ecx3`이다(reEXCEED 작업 로그 i004).

---

# Work log: the kernel data export `XeImageFileName` (issue #49)

Design: [20261103-i049](../design/20261103-i049-xe-image-file-name.md) · Work order: [20261103-i049](../work-orders/20261103-i049-xe-image-file-name.md) · Consumer: reexec/reEXCEED#4

## Done

`KernelConfig::image_file_name` (default `default.xbe`) and the public `DataExportAddress(name)` are in; `XeImageFileName` (an 8-byte `ANSI_STRING`) joins the data export table, and `InitializeKernel` has `SetImageFileName` write `\Device\CdRom0\<file name>` and a NUL to the guest heap and fill `Length` (without the NUL), `MaximumLength` (with it) and `Buffer`; the kernel probe gains `ProbeImageFileName` (three checks).

## Verification

Win32 x86 Debug (inside the reEXCEED tree, Visual Studio 2026): 0 errors, 0 warnings. `rexbox_kernel_probe`: 97 checks, 0 failures (from 94). In reEXCEED (`image_file_name = DEFAULT.XBE`) XAPI's start-up passes this point without a fault and creates the Z: cache directories; the next stop is D3D8LTCG `Direct3D_CreateDevice_4__LTCG_eax1_ecx3` (reEXCEED work log i004).
