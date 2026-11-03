# 작업 로그: APU 전역 샘플 카운터 (issue #55)

설계: [20261103-i055](../design/20261103-i055-apu-sample-count.md) · 소비자: reexec/reEXCEED#4

## 한 일

`include/rexbox/hle/dsound/dsound_hle.h`에 `extern "C" uint32_t rexbox_dsound_apu_sample_count(void)`, `dsound_hle.cpp`에 구현(마이크로초 × 48 / 1000)을 더했다. 작업 지시는 이 로그의 한 항목으로 갈음한다(함수 하나).

## 검증

Win32 x86 Debug(reEXCEED 트리): 오류 0, 경고 0. reEXCEED: XACT의 fault 없이 지나 다음 정지점은 DSOUND `XAudioCreatePcmFormat`.

---

# Work log: the APU's global sample count (issue #55)

Design: [20261103-i055](../design/20261103-i055-apu-sample-count.md) · Consumer: reexec/reEXCEED#4

`extern "C" uint32_t rexbox_dsound_apu_sample_count(void)` is declared in `include/rexbox/hle/dsound/dsound_hle.h` and implemented in `dsound_hle.cpp` (microseconds x 48 / 1000); the work order is this entry (one function). Verification: Win32 x86 Debug (in the reEXCEED tree) with 0 errors and 0 warnings; reEXCEED passes XACT without the fault, its next stop being DSOUND `XAudioCreatePcmFormat`.
