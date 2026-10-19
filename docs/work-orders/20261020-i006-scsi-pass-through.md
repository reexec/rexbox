# 작업 지시: IOCTL_SCSI_PASS_THROUGH (issue #6)

설계: [20261020-i006](../design/20261020-i006-scsi-pass-through.md)

1. `src/hle/kernel/kernel_file.cpp`: `kIoctlScsiPassThrough`, `ScsiPassThrough()`, `ControlFile`의 분기.
2. `docs/kb/xbox-dvd-drive.md`와 kb 색인, `ARCHITECTURE.md`의 커널 파일 표.
3. 검증: Win32 x86 Debug 빌드와 `ctest`; reCT3에서 실행(frontier 이동 확인). 작업 브랜치 `i004-linux-preparation`에서 이어서(규칙: `main`이 아니면 새 브랜치를 만들지 않는다); PR #5가 #4와 함께 닫는다.

---

# Work order: IOCTL_SCSI_PASS_THROUGH (issue #6)

Design: [20261020-i006](../design/20261020-i006-scsi-pass-through.md)

(1) `kIoctlScsiPassThrough`, `ScsiPassThrough()` and the `ControlFile` branch in `kernel_file.cpp`. (2) `docs/kb/xbox-dvd-drive.md`, the kb index and the kernel file table in `ARCHITECTURE.md`. (3) Verify with the Win32 x86 Debug build and `ctest`, and by running reCT3 (the frontier moves). On the task branch `i004-linux-preparation` (the rules: no new branch while off `main`); pull request #5 closes it together with #4.
