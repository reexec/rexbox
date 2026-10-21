# 작업 지시: D3D8 펜스 export (issue #19)

설계: [20261021-i019-fence-exports](../design/20261021-i019-fence-exports.md) · 소비자: reexec/reCT3#14

1. 작업 브랜치 `i019-fence-hle`(`main`에서). 커밋 제목 끝에 `(#19)`.
2. `src/hle/d3d8/d3d8_device.cpp`: `D3D_SetFence` 옆에 `D3DDevice_InsertFence`, `D3DDevice_BlockOnFence`, `D3DDevice_IsFencePending`을 두고 `RegisterDeviceExports`에 등록한다(설계 §2).
3. `docs/kb/xbox-d3d8-api.md` §5에 펜스 함수의 원본 동작과 HLE의 처리를 적는다.
4. 검증: Win32 x86 Release 빌드, CTest, 소비자 reCT3에서 `REXBOX_HLE_FALLBACK` 없이 Start → 게임플레이(설계 §3).
5. 작업 로그 `docs/work-logs/20261021-i019-fence-exports.md`.

---

# Work order: the D3D8 fence exports (issue #19)

Design: [20261021-i019-fence-exports](../design/20261021-i019-fence-exports.md) · Consumer: reexec/reCT3#14

(1) Task branch `i019-fence-hle` (from `main`); commit titles end with `(#19)`. (2) `src/hle/d3d8/d3d8_device.cpp`: `D3DDevice_InsertFence`, `D3DDevice_BlockOnFence` and `D3DDevice_IsFencePending` next to `D3D_SetFence`, registered in `RegisterDeviceExports` (design section 2). (3) `docs/kb/xbox-d3d8-api.md` section 5: what the fence functions do on the original and how the HLE handles them. (4) Verify: Win32 x86 Release build, CTest, and in the consumer reCT3, Start reaching gameplay without `REXBOX_HLE_FALLBACK` (design section 3). (5) Work log `docs/work-logs/20261021-i019-fence-exports.md`.
