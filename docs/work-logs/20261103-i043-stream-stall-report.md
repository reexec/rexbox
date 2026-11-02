# 작업 로그: 다시 채워지지 않는 스트림 루프 버퍼 보고 (issue #43)

작업 지시: [20261103-i043-stream-stall-report](../work-orders/20261103-i043-stream-stall-report.md) · 설계: [20261103-i043-stream-stall-report](../design/20261103-i043-stream-stall-report.md) · 소비자: reexec/reCT3#29

## 한 일

설계 §2대로 구현했다. reCT3 작업 i029의 진단 빌드(바퀴 해시, `backtrace`)를 기본 동작에 맞게 다듬었다. 정적 루프 제외 조건(내용이 바뀐 적이 있는 버퍼만)과 무음 제외를 넣었고, 보고는 한 번만 분리한 스레드에서 한다. 플랫폼 계층으로 옮겼다.

## 검증

| 검증 | 결과 |
|---|---|
| reCT3 Linux x64 Release | 경고 0, CTest 7/7 |
| reCT3 데모 160초(숨김, 소리 없음) | 정적 루프(엔진음 등)와 곡 사이 무음에 보고 없음. 약 143초에 48 kHz 음악 버퍼(`03FDA400`) 보고 한 번, 게스트 스레드 줄과 7개 스레드의 호스트 콜스택(57줄) |

데모에서도 reCT3#29가 재현된다(**확인됨**). 이때의 콜스택에서 모든 게스트 스레드는 평소의 대기(vblank, 지연, 자기 정지, 이벤트)에 있었다. 루프에 갇힌 스레드는 없고 스트림 상태가 진행하지 않는다(reCT3 작업 로그 i029의 play2와 같음).

## 남은 것

Win32 콜스택. 원인(reCT3#29).

---

# Work log: reporting a streamed loop buffer no longer refilled (issue #43)

Work order: [20261103-i043-stream-stall-report](../work-orders/20261103-i043-stream-stall-report.md) · Design: [20261103-i043-stream-stall-report](../design/20261103-i043-stream-stall-report.md) · Consumer: reexec/reCT3#29

Done as design section 2, refining reCT3 task i029's diagnostic build (lap hashes, `backtrace`) for default use: the static-loop exclusion (only buffers whose content has changed), the silence exclusion, a single report from a detached thread, and the stacks moved into the platform layer.

Verified: reCT3 Linux x64 Release with 0 warnings and CTest 7/7; a 160 s reCT3 demo (hidden, no audio): no report for static loops (engine sounds) or silence between tracks, one report at about 143 s for the 48 kHz music buffer (`03FDA400`) with the guest thread lines and seven threads' host stacks (57 lines). The demo reproduces reCT3#29 too (**confirmed**); at that moment every guest thread sat in its usual wait (vblank, delay, self-suspend, events), none stuck in a loop: the stream state does not advance (as play2 in reCT3 work log i029).

Left open: Win32 stacks; the cause (reCT3#29).
