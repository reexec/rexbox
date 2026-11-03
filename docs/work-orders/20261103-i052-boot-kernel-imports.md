# 작업 지시: Exceed의 부팅이 닿는 커널 import (issue #52)

설계: [20261103-i052](../design/20261103-i052-boot-kernel-imports.md)

1. reEXCEED를 실행해 멈춘 커널 import를 확인한다.
2. 설계 표에 더하고 `src/hle/kernel/`에 구현한다. 게임 없이 확인할 수 있는 것은 kernel probe에 넣는다.
3. 빌드, probe, reEXCEED 실행. 작업 로그를 갱신하고 커밋한다(`(#52)`).

---

# Work order: the kernel imports Exceed's boot reaches (issue #52)

Design: [20261103-i052](../design/20261103-i052-boot-kernel-imports.md)

1. Run reEXCEED and note the kernel import it stops at.
2. Add it to the design's table and implement it under `src/hle/kernel/`, with what can be checked without a game in the kernel probe.
3. Build, probes, a reEXCEED run; update the work log and commit (`(#52)`).
