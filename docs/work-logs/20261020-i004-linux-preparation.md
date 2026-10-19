# 작업 로그: Linux x64 / x86 준비 (issue #4)

설계: [20261020-i004](../design/20261020-i004-linux-preparation.md) · 작업 지시: [20261020-i004](../work-orders/20261020-i004-linux-preparation.md)

## 한 일

1. `src/platform/linux/` 다섯 파일: mmap/mprotect와 soft-dirty 페이지(지원 여부 자가 검사), C++ 예외(`RaiseStop`)와 신호 핸들러 + `siglongjmp`(fault)의 보호 호출, pthread와 `CLOCK_THREAD_CPUTIME_ID`, `/proc/self/statm`, 샘플러 거부. `src/platform/CMakeLists.txt`에 소스 목록과 `Threads::Threads`.
2. `cmake/rexbox_host_flags.cmake`: 32비트 x86 GCC/Clang에 `-msse2 -mfpmath=sse`. 첫 조건이 `CMAKE_SYSTEM_PROCESSOR`를 `i686`으로만 봐서 `-m32` 빌드(호스트 `x86_64`)에 적용되지 않았고, x86 Debug의 probe가 다시 실패해 조건을 넓혔다.
3. `CMakeLists.txt`: XbSymbolDatabase가 `-Werror`로 빌드되는데 GCC의 fortified `memcpy`가 `-m32 -O2`에서 `stringop-overread`를 내므로(`XbSDB_GenerateLibraryFilter`, 8바이트 읽기 대 짧은 문자열 리터럴) 그 타깃에만 `-Wno-error=stringop-overread`. upstream 코드는 그대로 두었다(upstream 보고 후보).
4. `d3d8_resource.cpp`의 미사용 `Log2Floor` 삭제(`-Wall`이 찾음).
5. `CMakePresets.json`(Linux 프리셋 4개), `scripts/build_linux.sh`, `ci.yml`의 `linux` 잡, `release.yml`의 `verify-linux` 잡.
6. 문서: 설계·작업 지시·이 로그, `docs/guides/linux-setup-and-build.md`와 색인, `README.md`, `ARCHITECTURE.md`.

## 검증

| 검증 | 결과 |
|---|---|
| Linux x64 Debug / Release (WSL2 Ubuntu 24.04, GCC 13.3, Ninja) | 빌드 경고 0(우리 타깃), `ctest` 3/3 |
| Linux x86 Debug / Release (`-m32`) | 빌드 경고 0, `ctest` 3/3. SSE 플래그 없이는 D3D8 probe 2개 실패(x87 확장 정밀도), 플래그로 해결 |
| Win32 x86 Debug | 빌드 경고 0, `ctest` 3/3 (회귀 없음) |
| 워크플로 YAML | 파싱 통과. CI의 i386 패키지 조합은 PR에서 확인(대기) |
| soft-dirty | WSL2 커널은 미지원 → 자가 검사가 추적 없이 할당하고 `TakeWrittenPages`가 false. 지원 커널에서의 추적 경로는 미검증(**미확정**) |

## 남은 것

* Linux 샘플러, 스택 오버플로용 대체 신호 스택, soft-dirty 지원 커널에서의 검증.
* 타이틀 앱의 Linux 빌드(reCT3)는 그쪽 작업.

---

# Work log: Linux x64 / x86 preparation (issue #4)

Design: [20261020-i004](../design/20261020-i004-linux-preparation.md) · Work order: [20261020-i004](../work-orders/20261020-i004-linux-preparation.md)

## Done

The five `src/platform/linux/` files (mmap/mprotect with soft-dirty pages behind a self test, guarded calls through a C++ exception for `RaiseStop` and signal handlers with `siglongjmp` for faults, pthreads with `CLOCK_THREAD_CPUTIME_ID`, `/proc/self/statm`, the sampler declining) and their CMake list with `Threads::Threads`; `-msse2 -mfpmath=sse` for 32-bit x86 GCC/Clang in `rexbox_host_flags.cmake` (the first condition matched only `i686`, so a `-m32` build on an `x86_64` host missed it and the x86 Debug probe failed again; widened); `-Wno-error=stringop-overread` for the XbSymbolDatabase target only, since it builds with `-Werror` and GCC's fortified `memcpy` reports an 8-byte read of a shorter string literal at `-m32 -O2` (upstream's code untouched; a report candidate); the unused `Log2Floor` removed; the four Linux presets, `scripts/build_linux.sh`, the `linux` CI job and the `verify-linux` release job; the documents.

## Verification

Linux x64 and x86 Debug/Release on WSL2 Ubuntu 24.04 (GCC 13.3, Ninja): 0 warnings on our targets, `ctest` 3/3 each (without the SSE flags two D3D8 probe checks failed from x87 excess precision); Win32 x86 Debug: 0 warnings, `ctest` 3/3, no regression; the workflow YAML parses, and CI's i386 package set is confirmed at the pull request (pending); the WSL2 kernel has no soft-dirty tracking, so the self test allocates untracked and `TakeWrittenPages` reports failure; the tracking path on a supporting kernel is unverified (**unresolved**).

## Left open

The Linux sampler, an alternate signal stack for stack overflows, verification on a soft-dirty kernel; a title app's Linux build is reCT3's task.
