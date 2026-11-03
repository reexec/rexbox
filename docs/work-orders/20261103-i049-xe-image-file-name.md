# 작업 지시: 커널 데이터 export `XeImageFileName` (issue #49)

설계: [20261103-i049](../design/20261103-i049-xe-image-file-name.md)

1. `include/rexbox/hle/kernel/kernel_hle.h`: `KernelConfig::image_file_name`(기본 `default.xbe`), `DataExportAddress(name)`.
2. `src/hle/kernel/kernel_data.cpp`: 데이터 export 표에 `XeImageFileName`(8), `SetImageFileName`, `DataExportAddress`.
3. `src/hle/kernel/kernel_hle.cpp`: `InitializeKernel`이 `SetImageFileName`을 부른다.
4. `tools/kernel_probe`: `ProbeImageFileName`.
5. Win32 Debug 빌드, kernel probe, reEXCEED에서 실행 확인. 작업 로그를 쓰고 커밋한다(`(#49)`).

---

# Work order: the kernel data export `XeImageFileName` (issue #49)

Design: [20261103-i049](../design/20261103-i049-xe-image-file-name.md)

1. `include/rexbox/hle/kernel/kernel_hle.h`: `KernelConfig::image_file_name` (default `default.xbe`) and `DataExportAddress(name)`.
2. `src/hle/kernel/kernel_data.cpp`: `XeImageFileName` (8) in the data export table, `SetImageFileName`, `DataExportAddress`.
3. `src/hle/kernel/kernel_hle.cpp`: `InitializeKernel` calls `SetImageFileName`.
4. `tools/kernel_probe`: `ProbeImageFileName`.
5. Win32 Debug build, kernel probe, a run in reEXCEED; write the work log and commit (`(#49)`).
