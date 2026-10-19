# 작업 지시: Linux x64 / x86 준비 (issue #4)

설계: [20261020-i004](../design/20261020-i004-linux-preparation.md)

1. 작업 브랜치 `i004-linux-preparation`.
2. `src/platform/linux/`: `virtual_memory_linux.cpp`, `guarded_call_linux.cpp`, `host_thread_linux.cpp`, `system_info_linux.cpp`, `sampler_linux.cpp`(미구현 선언). `src/platform/CMakeLists.txt`에 `REXBOX_PLATFORM_SOURCES_linux`와 `Threads::Threads`.
3. `cmake/rexbox_host_flags.cmake`: 32비트 x86 GCC/Clang에 `-msse2 -mfpmath=sse`.
4. `CMakePresets.json`(Linux 프리셋 4개), `scripts/build_linux.sh`.
5. `ci.yml`의 `linux` 잡, `release.yml`의 `verify-linux` 잡.
6. 문서: `ARCHITECTURE.md`(플랫폼 표, §5), `README.md`(플랫폼 상태), `docs/guides/linux-setup-and-build.md`와 색인.
7. 검증: 설계 §5.

---

# Work order: Linux x64 / x86 preparation (issue #4)

Design: [20261020-i004](../design/20261020-i004-linux-preparation.md)

(1) Task branch `i004-linux-preparation`. (2) `src/platform/linux/` with the five implementations (the sampler declining), `REXBOX_PLATFORM_SOURCES_linux` and `Threads::Threads` in the platform CMake. (3) `-msse2 -mfpmath=sse` for 32-bit x86 GCC/Clang in `rexbox_host_flags.cmake`. (4) `CMakePresets.json` (four Linux presets) and `scripts/build_linux.sh`. (5) The `linux` job in `ci.yml` and `verify-linux` in `release.yml`. (6) Documents: `ARCHITECTURE.md` (platform table, §5), `README.md` (platform status), `docs/guides/linux-setup-and-build.md` and its index. (7) Verify as in design §5.
