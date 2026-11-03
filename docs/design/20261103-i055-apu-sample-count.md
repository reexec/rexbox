# 설계: 레지스터를 직접 읽는 타이틀을 위한 APU 전역 샘플 카운터 (issue #55)

상태: **구현됨**. 작업 로그: [20261103-i055](../work-logs/20261103-i055-apu-sample-count.md). 소비자: reexec/reEXCEED#4.

## 1. 요구사항

Pump It Up: Exceed의 XACT 엔진(XDK 5849, lift된 코드, 타이틀 함수 `0x0016F12A`)은 APU의 전역 샘플 카운터 `0xFE80200C`를 시간 기준으로 읽는다(48 kHz, **추정**: 결과를 `x 10000 / 48000 x 1000`으로 100 ns 단위로 바꾸는 코드에서). 그 주소는 게스트 메모리 밖이라 fault한다.

## 2. 설계

* rexbox는 MMIO를 매핑하지 않는다(32비트 호스트에서 `기준 + 0xFE80200C`가 4 GiB를 넘어 감기는 등, 주소 공간을 다룰 수 없다). 대신 카운터 값을 C 함수 `rexbox_dsound_apu_sample_count()`로 준다: DSOUND 모듈이 시작된 뒤의 48 kHz 샘플 수(steady clock 기준, 32비트에서 감김).
* 타이틀은 레지스터를 읽는 함수를 어댑터로 바꾸어 이 값을 읽는다(reEXCEED `decomp/xact_sample_clock.c`, 호스트 훅을 거침).
* 오디오 출력 장치와 무관한 벽시계다. 믹서의 진행과 정확히 같지는 않다(**미확정**: 어긋남이 문제가 되면 믹서의 렌더 프레임 수로 바꾼다).

---

# Design: the APU's global sample count for titles that read the register directly (issue #55)

Status: **implemented**. Work log: [20261103-i055](../work-logs/20261103-i055-apu-sample-count.md). Consumer: reexec/reEXCEED#4.

Pump It Up: Exceed's XACT engine (XDK 5849, lifted, title function `0x0016F12A`) reads the APU's global sample counter at `0xFE80200C` as its time base (48 kHz, **inferred** from the code turning the total into 100 ns units with `x 10000 / 48000 x 1000`), outside guest memory, and faults. rexbox maps no MMIO (on a 32-bit host `base + 0xFE80200C` wraps past 4 GiB, among other problems); it gives the counter's value as the C function `rexbox_dsound_apu_sample_count()`: 48 kHz samples since the DSOUND module started (a steady clock, wrapping at 32 bits). The title replaces the function that reads the register with an adapter that reads this (reEXCEED's `decomp/xact_sample_clock.c`, through a host hook). It is a wall clock independent of the audio device and not exactly the mixer's progress (**unresolved**: switch to the mixer's rendered frames if the drift matters).
