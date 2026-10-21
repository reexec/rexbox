# 작업 로그: 추적이 꺼져 있을 때 D3D8 추적 문장을 만들지 않는다 — 완료 (issue #22)

작업 지시: [20261021-i022-trace-text-off-path](../work-orders/20261021-i022-trace-text-off-path.md) · 소비자: reexec/reCT3#17

## 한 일

1. 원인 확인: 운전 화면의 표시 스레드 표본에서 ucrtbase가 13.2%였다. 내보내기 이름은 가까운 export일 뿐이라 해당 주소를 역어셈블했다: printf 서식 해석 상태 기계(`sub al,20h; cmp al,5Ah` 문자 분류표), 10진 자릿수 생성(`div; add dl,30h`), 그것이 부르는 `fegetround`의 `fnstenv`/`fldenv` 함수, `_aulldiv`. 합쳐 표본의 7.1%였고 할당·해제가 더 있었다.
2. 호출처: `D3DDevice_SetModelView`가 호출마다 실수 다섯 개를 `%g`로 쓰고 행렬을 곱했다(추적에만 쓰임). `D3DDevice_SetVertexShaderInput`도 호출마다 문자열 두 개를 만들었다. 텍스처 업로드 추적도 감싸지 않았다.
3. 세 곳을 `D3d8TraceEnabled()` 안으로 옮기고 `TraceOnce` 주석에 규칙을 적었다.

## 검증

| 검증 | 결과 |
|---|---|
| Win32 x86 Release 빌드 | 오류 0 |
| 표시 스레드 표본(소비자 운전, present 2500~3200) | ucrtbase 13.2% → 2.7%(printf 영역 7.1% → 0), 프레임당 표본 20.5 → 17.3 |
| 짝지은 실행(같은 입력, 바로 이어서) | `other` 15.6 → 12.1 ms, CPU 27.2 → 24.7 ms/프레임 |
| CTest | 소비자 작업 로그(reCT3 #17) 참조 |

이 기계(i5-7200U 노트북)는 같은 바이너리로도 실행마다 프레임 시간이 23.5~16.9 fps로 크게 흔들려(발열·다른 프로세스로 **추정**), fps 대신 바로 이은 짝 실행의 단계 시간과 표본 비율로 비교했다. 바쁜 시간이 vblank 둘 근처라 fps는 거의 그대로다(27.3 → 28.4).

## 남은 것

* 그리기 하나의 CPU 비용(프레임당 약 14 ms): issue #23 설계.
* 표시 스레드의 `NtWriteFile`(표본 2.6%, 프레임당 약 0.9 ms): 출처 미확정(소비자 작업 로그).

---

# Work log: no D3D8 trace text while tracing is off — done (issue #22)

Work order: [20261021-i022-trace-text-off-path](../work-orders/20261021-i022-trace-text-off-path.md) · Consumer: reexec/reCT3#17

Done:

1. The cause: ucrtbase was 13.2% of the presenting thread's samples while driving. Export names are only the nearest export, so the hot addresses were disassembled: printf's format-parsing state machine (the `sub al,20h; cmp al,5Ah` character-class table), decimal digit generation (`div; add dl,30h`), the `fnstenv`/`fldenv` function `fegetround` calls from there, and `_aulldiv`, together 7.1% of the samples, with allocations and frees on top.
2. The callers: `D3DDevice_SetModelView` wrote five floats with `%g` and multiplied matrices on every call, for the trace only; `D3DDevice_SetVertexShaderInput` built two strings on every call; the texture-upload trace was unwrapped too.
3. The three sites now sit inside `D3d8TraceEnabled()`, and `TraceOnce`'s comment states the rule.

Verification:

| Check | Result |
|---|---|
| Win32 x86 Release build | no errors |
| Presenting-thread samples (consumer driving, presents 2500–3200) | ucrtbase 13.2% → 2.7% (printf code 7.1% → 0), samples per frame 20.5 → 17.3 |
| Paired runs (same input, back to back) | `other` 15.6 → 12.1 ms, CPU 27.2 → 24.7 ms per frame |
| CTest | see the consumer's work log (reCT3 #17) |

This machine (an i5-7200U laptop) swings between 23.5 and 16.9 fps from run to run with the same binary (**inferred**: heat and other processes), so the comparison uses phase times and sample shares of back-to-back pairs instead of fps. The busy time sits near two vblanks, so fps barely moves (27.3 → 28.4).

Left over: the CPU cost of a draw (about 14 ms per frame) is designed in issue #23; an `NtWriteFile` on the presenting thread (2.6% of samples, about 0.9 ms per frame) has no confirmed source yet (see the consumer's work log).
