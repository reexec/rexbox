# AGENTS.md

이 문서의 작업 규칙과 코딩 규칙은 [nworkers/reJSRF](https://github.com/nworkers/reJSRF)의 규칙을 그대로 따르며(그 규칙은 [reexec/rePIU](https://github.com/reexec/rePIU)에서 왔다), 이 프로젝트의 성격(여러 타이틀이 공유하는 Xbox 런타임 라이브러리)과 라이선스(MIT)에 맞게 고쳤다. 소비자 프로젝트(reJSRF, reCT3)와 같은 규칙으로 일한다.

The working and coding rules here are those of [nworkers/reJSRF](https://github.com/nworkers/reJSRF) (themselves based on [reexec/rePIU](https://github.com/reexec/rePIU)), adapted to this project's nature (an Xbox runtime library shared by several titles) and license (MIT). The consumer projects (reJSRF, reCT3) work under the same rules.

## 프로젝트 개요

rexbox는 원본 Xbox 타이틀을 정적 재컴파일로 네이티브 실행하는 프로젝트들이 공유하는 **타이틀을 모르는 Xbox 런타임**이다. XBE 파서, 게스트 메모리와 힙, HLE 레지스트리, 재컴파일 코드(xboxrecomp lifter) ABI 어댑터, Xbox 커널·XDK 라이브러리(D3D8, DSOUND, XAPI, XInput)의 HLE 모듈, 호스트 플랫폼 계층(가상 메모리, 스레드, SDL3 표시·입력·오디오)을 담는다.

타이틀 프로젝트(예: reJSRF의 Jet Set Radio Future, reCT3의 Crazy Taxi 3)는 이 저장소를 submodule로 가져가 타이틀 계층(프로파일, 디컴파일 코드, 재컴파일 설정)과 앱만 더한다. 원본 x86 코드를 C로 옮기는 재컴파일과 함수를 하나씩 읽을 수 있는 코드로 바꾸는 디컴파일은 타이틀 프로젝트의 일이며, rexbox는 그 코드가 부르는 Xbox를 제공한다.

## Project Overview

rexbox is the **title-agnostic Xbox runtime** shared by projects that run original Xbox titles natively through static recompilation. It holds the XBE parser, guest memory and heap, the HLE registry, the ABI adapter for recompiled code (the xboxrecomp lifter), the HLE modules for the Xbox kernel and the XDK libraries (D3D8, DSOUND, XAPI, XInput), and the host platform layer (virtual memory, threads, display, input and audio on SDL3).

A title project (Jet Set Radio Future in reJSRF, Crazy Taxi 3 in reCT3) takes this repository as a submodule and adds only its title layer (profile, decompiled code, recompilation configuration) and its application. Recompiling the original x86 to C and decompiling functions one by one into readable code is the title project's work; rexbox provides the Xbox that code calls.

---

## 핵심 설계 목표

우선순위:

1. rexbox는 타이틀을 알지 못한다. 타이틀 이름, 타이틀 주소, 타이틀 헤더가 이 저장소에 나타나면 규칙 위반이다. 타이틀이 알려줘야 하는 것(앱 이름, XDK 변수 주소, 표시 기본값)은 공개 계약(`include/rexbox/`)으로 받는다.
2. 원본 게임 로직을 보존한다. 재컴파일된 코드가 기준 구현이며 HLE는 그 코드가 부르는 API의 동작을 원본과 같게 한다.
3. HLE는 API 경계(커널 ordinal, XDK 라이브러리 함수)에서 한다. 하드웨어 레지스터 수준 에뮬레이션(NV2A, MCPX APU LLE)은 API 경계로 해결되지 않을 때만 쓴다.
4. Win32 x86을 먼저 완성하고, 이후 Win64, Linux x86/x64, Web, Android로 넓힌다. 처음부터 멀티플랫폼 공용 구조로 설계한다.
5. 에뮬레이터 소스(xemu, Cxbx-Reloaded)를 통합하지 않는다.
6. 게임 데이터(XBE, 자산, ISO)와 그것에서 기계적으로 생성한 코드는 저장소에 넣지 않는다. 이 저장소에는 어떤 타이틀의 데이터도 없다.

## Primary Design Goals

Priority order:

1. rexbox knows no title. A title's name, addresses or headers appearing in this repository is a rule violation. What a title must tell it (the application name, XDK variable addresses, display defaults) comes through the public contracts in `include/rexbox/`.
2. Preserve the original game logic. The recompiled code is the reference implementation, and HLE makes the APIs that code calls behave as the originals do.
3. Do HLE at API boundaries (kernel ordinals, XDK library functions). Use hardware-register emulation (NV2A, MCPX APU LLE) only where an API boundary cannot work.
4. Finish Win32 x86 first, then widen to Win64, Linux x86/x64, the web and Android. Design shared multiplatform structures from the start.
5. Do not integrate emulator source code (xemu, Cxbx-Reloaded).
6. Never put game data (XBE, assets, ISO) or code mechanically generated from it in the repository. This repository holds no title's data at all.

---

## 참조 문서

아키텍처 변경 전에는 관련 문서를 먼저 읽고 갱신한다.

* `docs/PROJECT_CHARTER.md`
* `ARCHITECTURE.md`
* `docs/CODING_STYLE.md`
* `THIRD_PARTY_NOTICES.md`
* `docs/design/`
* `docs/work-orders/`
* `docs/work-logs/`
* `docs/kb/`
* `docs/guides/`

타이틀에서 확인한 바이너리 분석(`docs/analysis/`, `docs/XBE_DESIGN.*`)은 타이틀 프로젝트에 둔다. rexbox의 동작을 바꾼 근거가 어느 타이틀의 분석이면 그 문서를 링크한다.

## Reference Documents

Read and update the relevant documents before making architectural changes: `docs/PROJECT_CHARTER.md`, `ARCHITECTURE.md`, `docs/CODING_STYLE.md`, `THIRD_PARTY_NOTICES.md`, `docs/design/`, `docs/work-orders/`, `docs/work-logs/`, `docs/kb/`, `docs/guides/`.

Binary analysis confirmed on a title (`docs/analysis/`, `docs/XBE_DESIGN.*`) lives in the title project. When a change to rexbox rests on such an analysis, link that document.

---

## 핵심 원칙

1. 사용자 요구사항을 받으면 바로 구현하지 말고 먼저 설계를 작성하거나 기존 설계를 갱신한다.
2. 설계가 정리되면 구현 계획서를 작성하거나 갱신한 뒤 구현을 시작한다.
3. 코드 변경이 있는 모든 작업은 작업 지시 문서와 작업 로그 문서를 남긴다.
4. 문서는 기본적으로 한국어를 먼저 쓰고 바로 아래에 영어 번역을 추가한다. 이 규칙은 `docs/` 아래 문서와 `README.md`, `ARCHITECTURE.md` 같은 저장소 문서에만 적용되며, 소스 코드 주석에는 적용하지 않는다.
5. 기존 코드, 자산, 문서의 의도를 확인한 뒤 수정하며, 큰 구조 변경은 근거를 문서에 남긴다.
6. 멀티플랫폼 공용 구조를 우선 설계하고, 플랫폼별 세부 사항은 분리한다.
7. 공개 계약(`include/rexbox/`)을 바꾸면 소비자 프로젝트가 따라와야 한다. 바꾸는 이유와 소비자가 할 일을 설계 문서에 적는다.

## Core Principles

1. When a user requirement is received, do not implement immediately. First write a design or update the existing design.
2. After the design is clear, write or update the implementation work order before starting implementation.
3. Every task that changes code must leave a work-order document and a work-log document.
4. Documents should be written in Korean first, followed immediately by an English translation. This applies to documents under `docs/` and repository documents such as `README.md` and `ARCHITECTURE.md`; it does not apply to source-code comments.
5. Confirm the intent of existing code, assets, and documents before modifying them. Document the rationale for large structural changes.
6. Design shared multiplatform structures first, and keep platform-specific details separated.
7. A change to a public contract (`include/rexbox/`) must be followed by the consumer projects. Record why it changes and what a consumer has to do in the design document.

---

## 응답 태도 규칙

* 모든 질문에 대한 답변은 존대말을 사용하고 예의 있는 태도를 유지한다.

## Response Tone Rules

* Use polite Korean speech and maintain a respectful attitude for all answers.

---

## 요구사항 처리 절차

1. 요구사항 접수
2. 관련 코드, 문서, 자산 맥락 확인
3. 설계 문서 작성 또는 갱신
4. 구현 계획서 작성 또는 갱신
5. 사용자 승인 또는 현재 진행 맥락 확인
6. 구현
7. 빌드 또는 범위에 맞는 검증
8. 작업 로그 작성

단, 요구사항이 단순 질문이거나 확인 요청이라면 해당 내용에 바로 응답하고 문서화 및 코드 구현은 하지 않는다.

## Requirement Handling Procedure

1. Receive the requirement.
2. Inspect the relevant code, documentation, and asset context.
3. Write or update the design document.
4. Write or update the implementation work order.
5. Confirm user approval or the current active context.
6. Implement.
7. Run a build or verification appropriate to the scope.
8. Write the work log.

If the requirement is a simple question or confirmation request, answer it directly without documentation or code implementation.

---

## 문서 작성 규칙

* 설계 문서는 `docs/design/` 아래에 둔다.
* 계획 문서는 `docs/work-orders/` 아래에 둔다.
* 작업 결과와 회고는 `docs/work-logs/` 아래에 둔다.
* 사용자가 직접 수행하는 검증·측정·운영 절차는 `docs/guides/` 아래에 둔다. 반복 수행 가능한 절차만 두고, 근거가 되는 작업 로그와 설계를 링크한다.
* 일반 기술 정의와 배경 지식(XBE 형식, 커널 구조, XDK API)은 `docs/kb/` 아래에 주제별로 두고, 외부 자료에서 왔으면 원문 링크를 단다.
* 프로젝트의 큰 방향성은 `docs/PROJECT_CHARTER.md`에 반영한다.
* 현재 구현되는 코드의 설계와 구조는 `ARCHITECTURE.md`에 지속적으로 반영한다.
* 규칙이 바뀌면 `AGENTS.md`와 관련 문서를 함께 갱신한다.
* 작업 단위 문서 파일명은 `YYYYMMDD-i###-slug.md` 형식을 사용한다. `###`은 task의 GitHub issue 번호를 세 자리로 맞춘 값이다(예: issue #6 → `20261020-i006-slug.md`).
* Markdown 문서에서 구조, 관계, 흐름, 호출 순서, 상태 전이, 주소 변환을 도식화할 수 있으면 Mermaid를 적극적으로 사용한다.
* 세 개 이상의 구성요소·단계·분기·계층이 있으면 Mermaid 적용 가능성을 검토하고, 내용에 맞는 형식(flowchart, sequenceDiagram, stateDiagram, classDiagram 등)을 고른다.
* 단순 사실 하나나 한 단계 설명에는 도식을 강제하지 않는다.
* Mermaid 도식은 본문과 같은 사실 및 확인 상태를 표현해야 하며, 본문 설명을 완전히 대체하지 않는다.

## Documentation Rules

* Put design documents under `docs/design/`.
* Put work-order documents under `docs/work-orders/`.
* Put work results and retrospectives under `docs/work-logs/`.
* Put procedures the user runs themselves (verification, measurement, operations) under `docs/guides/`. Keep only repeatable procedures there, and link the design and work log they rest on.
* Put generally applicable technical definitions and background (the XBE format, kernel structures, XDK APIs) under `docs/kb/`, organized by topic and linked to external sources when derived from them.
* Reflect the project's broad direction in `docs/PROJECT_CHARTER.md`.
* Continuously reflect the design and structure of currently implemented code in `ARCHITECTURE.md`.
* When rules change, update `AGENTS.md` and the related documents together.
* Use the `YYYYMMDD-i###-slug.md` filename format for task documents, where `###` is the task's GitHub issue number padded to three digits (for example issue #6 → `20261020-i006-slug.md`).
* Use Mermaid whenever structure, relationships, flows, call sequences, state transitions, or address translation can be visualized.
* Evaluate Mermaid whenever content has three or more components, steps, branches, or layers, and choose a fitting form (flowchart, sequenceDiagram, stateDiagram, classDiagram, ...).
* Do not force diagrams into single-fact or one-step explanations.
* Mermaid diagrams must express the same facts and confirmation status as the prose and must not replace the written explanation.

## 지식 기반 유지 규칙

* 작업 중 프로젝트 이해에 중요한 새로운 용어, 표준, 실행 형식, CPU 동작, XDK API가 등장하면 관련 `docs/kb/` 문서를 작성하거나 갱신한다.
* 외부 자료에서 얻은 기술 지식에는 가능한 한 권위 있는 출처 링크를 가까운 위치에 남긴다.
* `docs/kb/`는 **확인됨**, **추정**, **미확정**을 구분한다. 어느 타이틀의 바이너리에서 확인한 사실이면 그 타이틀과 문서를 적는다.
* kb 파일을 추가하거나 이름을 바꾸면 `docs/kb/README.md` 색인을 같은 작업에서 갱신한다.
* 다른 프로젝트(xboxrecomp, jsrf-recomp 등)에서 얻은 주장은 우리 쪽에서 다시 확인하기 전까지 **추정**으로 적고 출처를 단다.

## Knowledge Base Maintenance Rules

* When important new terminology, standards, executable formats, CPU behavior or XDK APIs arise, create or update the relevant `docs/kb/` topic.
* Place authoritative external links near derived technical knowledge.
* Distinguish **confirmed**, **inferred**, and **unresolved** statements in `docs/kb/`. A fact confirmed on a title's binary names that title and its document.
* When adding or renaming a kb file, update the `docs/kb/README.md` index in the same task.
* Record claims taken from other projects (xboxrecomp, jsrf-recomp, ...) as **inferred**, with their source, until they are re-checked on our side.

---

## 레이어와 의존 규칙

* 레이어는 아래에서 위로 `platform` → `xbe`, `runtime` → `recomp`, `hle/<module>` 순서이고, 그 위에 소비자의 `game/<title>` → `app`이 온다. 각 레이어는 아래 레이어만 링크한다. CMake 타깃 그래프가 이 규칙을 강제한다.
* rexbox(`include/rexbox/`, `src/`)는 타이틀 이름, 타이틀 주소, 타이틀 헤더를 쓰지 않는다. 어떤 타이틀이든 그대로 가져다 쓸 수 있어야 한다.
* HLE 모듈(`src/hle/<module>/`)은 런타임만 링크하고 다른 HLE 모듈을 링크하지 않는다. 모듈 사이에 공유가 필요하면 런타임 계약으로 올린다.
* 타이틀 코드(재컴파일·디컴파일 코드)는 HLE 모듈을 직접 부르지 않는다. HLE는 `HleRegistry`의 바인딩(게스트 주소 → HLE export)과 생성된 override(`rexbox_hle_enter`)를 통해서만 연결된다.
* 플랫폼 계층은 런타임과 타이틀의 타입을 알지 못한다. 기본 값(주소, 크기, 버퍼, 콜백)으로만 호출된다.
* `rexbox_recomp`만 lifter의 ABI 헤더(`recomp_types.h`)를 안다. lifter 버전이 바뀌면 이 타깃만 바뀐다.

## Layer and Dependency Rules

* Layers run bottom to top: `platform` → `xbe`, `runtime` → `recomp`, `hle/<module>`, with the consumer's `game/<title>` → `app` above. Each layer links only the layers below it; the CMake target graph enforces this.
* rexbox (`include/rexbox/`, `src/`) never names a title, a title's addresses or a title's headers. Any title must be able to take it as is.
* An HLE module (`src/hle/<module>/`) links the runtime only, never another HLE module. Anything modules must share moves up into a runtime contract.
* Title code (recompiled and decompiled) never calls an HLE module directly. HLE is reached only through `HleRegistry` bindings (guest address → HLE export) and the generated overrides (`rexbox_hle_enter`).
* The platform layer knows none of the runtime's or the title's types. It is called with plain values (addresses, sizes, buffers, callbacks).
* Only `rexbox_recomp` knows the lifter's ABI header (`recomp_types.h`); a lifter upgrade changes that target alone.

---

## 구현 규칙

* 플랫폼(OS) 종속 코드는 `src/platform/win32/`, `src/platform/linux/`, `src/platform/web/`, `src/platform/android/` 아래에 둔다. 여러 OS가 공유하는 호스트 라이브러리 백엔드(예: SDL3)는 `src/platform/<라이브러리>/`(예: `src/platform/sdl/`)에 두고, 각 OS가 `src/platform/CMakeLists.txt`의 소스 목록에서 선택한다. 런타임·HLE·도구 디렉터리에는 플랫폼·아키텍처 하위 디렉터리를 두지 않는다.
* 한 플랫폼 안에서 CPU 아키텍처에 따라 갈리는 코드는 그 플랫폼 디렉터리의 아키텍처 하위 디렉터리(예: `src/platform/linux/x86/`, `src/platform/linux/x64/`)에 둔다. 공개 헤더도 같은 규칙으로 `include/rexbox/platform/<플랫폼>/`에 두고, 모든 호스트가 쓰는 플랫폼 중립 계약만 `include/rexbox/platform/`에 둔다.
* 게스트 주소는 항상 32비트 `GuestAddress`이며 호스트 포인터로 바꿀 때는 `GuestMemory::Translate`를 거친다(based 주소 모델). 32비트 호스트에서도 게스트 주소를 호스트 포인터로 직접 쓰지 않는다.
* 새 기능을 추가할 때는 테스트 전략 또는 최소 검증 절차를 문서에 함께 남긴다. 게임 없이 검증할 수 있는 것은 `tools/`의 probe에 넣어 CI에서 돌린다.
* 코드 수정이 있는 작업은 영향 범위에 맞는 빌드 검증을 수행하고, 불가능하면 이유를 작업 로그에 남긴다. 소비자 프로젝트에서의 실행 검증이 필요한 변경은 작업 로그에 어느 타이틀로 확인했는지 적는다.
* 코딩 스타일 세부 규칙은 `docs/CODING_STYLE.md`에서 관리한다.
* 소스 코드 주석은 영어로만 작성한다. 세부 규칙은 `docs/CODING_STYLE.md`의 주석 언어 항목을 따른다.
* 큰 기능은 하나의 거대 파일에 누적하지 않는다. 독립적으로 이름 붙일 수 있는 하위 시스템은 전용 header/source로 분리하고, 통합 지점에는 orchestration과 adapter 코드만 남긴다.

## Implementation Rules

* Put platform (OS) specific code under `src/platform/win32/`, `src/platform/linux/`, `src/platform/web/`, or `src/platform/android/`. A backend on a host library several OSes share (for example SDL3) goes in `src/platform/<library>/` (for example `src/platform/sdl/`), and each OS selects it in the source list of `src/platform/CMakeLists.txt`. The runtime, HLE and tool directories have no platform or architecture subdirectories.
* Within one platform, put code that differs by CPU architecture in an architecture subdirectory of that platform (for example `src/platform/linux/x86/` and `src/platform/linux/x64/`). Public headers follow the same rule under `include/rexbox/platform/<platform>/`; only platform-neutral contracts every host uses stay in `include/rexbox/platform/`.
* A guest address is always a 32-bit `GuestAddress`, turned into a host pointer only through `GuestMemory::Translate` (the based address model). Even on a 32-bit host, guest addresses are not used directly as host pointers.
* When adding a feature, document the test strategy or minimum verification procedure. What can be verified without a game goes into a probe under `tools/` and runs in CI.
* For tasks that modify code, run build verification appropriate to the impact. If verification is impossible, record the reason in the work log. A change that needs a run in a consumer project records in the work log which title verified it.
* Maintain detailed coding style rules in `docs/CODING_STYLE.md`.
* Write source-code comments in English only, following the comment-language section of `docs/CODING_STYLE.md`.
* Do not accumulate major features in one monolithic file. Give independently named subsystems their own header/source, leaving only orchestration or adapter code at the integration point.

---

## 게임 데이터와 생성 코드 규칙

* 이 저장소에는 XBE, ISO, 게임 자산, 그리고 그것에서 기계적으로 생성한 코드를 절대 커밋하지 않는다. 타이틀 프로젝트의 `roms/`, `gen/`도 여기로 가져오지 않는다.
* 커밋 전에 `git status`로 게임 데이터가 staging되지 않았는지 확인한다.

## Game Data and Generated Code Rules

* Never commit an XBE, ISO, game assets or code mechanically generated from them to this repository. A title project's `roms/` and `gen/` never come here.
* Before committing, check with `git status` that no game data is staged.

---

## 라이선스 규칙

* 프로젝트 라이선스는 `MIT License`다.
* 서드파티 오픈소스는 MIT, BSD, zlib, ISC, Apache-2.0, Boost 같은 허용적 라이선스만 빌드에 포함한다.
* GPL, LGPL, AGPL 같은 전염성 라이선스의 코드는 컴파일·링크하지 않고 저장소에 복사하지도 않는다. `cmake/rexbox_license.cmake`의 `REXBOX_COPYLEFT_PATHS` 가드가 xboxrecomp의 LGPL 파일을 막는다(소비자가 지정한 xboxrecomp 체크아웃에도 적용된다).
* 전염성 라이선스 프로젝트(xemu, Cxbx-Reloaded 등)는 동작을 이해하는 지식 출처로만 참고하고, 코드를 옮기지 않는다.
* 라이선스가 명시되지 않았거나 LLM 기여를 금지한 프로젝트의 코드는 가져오지 않는다. 공개 문서의 사실은 출처를 달아 참고할 수 있다.
* 서드파티 코드를 가져오면 같은 작업에서 `THIRD_PARTY_NOTICES.md`를 갱신한다. 예외가 필요하면 먼저 문서화하고 사용자 확인을 받는다.
* 가져온 서드파티 코드에서 문제(버그, 이식성)를 찾으면 우리 사본만 고치지 않고 업스트림에 기여한다. 특히 같은 호스트 계층을 쓰는 [reexec/re2DJ](https://github.com/reexec/re2DJ)(BSD-3-Clause)는 필요할 때 참고하고, 얻어온 코드의 수정은 re2DJ에 되돌려 보낸다. xboxrecomp의 문제는 [sp00nznet/xboxrecomp](https://github.com/sp00nznet/xboxrecomp)에 PR로 낸다.

## License Rules

* The project license is the `MIT License`.
* Only permissively licensed third-party code (MIT, BSD, zlib, ISC, Apache-2.0, Boost, ...) is built into the project.
* Copyleft code (GPL, LGPL, AGPL) is neither compiled, linked, nor copied into the repository. The `REXBOX_COPYLEFT_PATHS` guard in `cmake/rexbox_license.cmake` blocks xboxrecomp's LGPL files (in a consumer's own xboxrecomp checkout too).
* Copyleft projects (xemu, Cxbx-Reloaded, ...) are consulted only as sources of knowledge about behavior; their code is not carried over.
* Do not take code from projects with no stated license or that forbid LLM contributions. Facts from their public documents may be used with attribution.
* When bringing in third-party code, update `THIRD_PARTY_NOTICES.md` in the same task. If an exception is needed, document it and confirm with the user first.
* When a problem (a bug, a portability issue) is found in third-party code we took, contribute the fix upstream instead of fixing only our copy. In particular, consult [reexec/re2DJ](https://github.com/reexec/re2DJ) (BSD-3-Clause), which shares our host layer, when useful, and send fixes to code taken from it back to re2DJ. Problems in xboxrecomp go to [sp00nznet/xboxrecomp](https://github.com/sp00nznet/xboxrecomp) as pull requests.

---

## 작업 단위 규칙

* task는 GitHub issue로 만든다. task 번호는 GitHub issue 번호를 쓴다.
* 기존 task의 후속 작업(남은 검증, 후속 수정)은 새 issue를 만들지 않고 그 task의 issue에 붙인다.
* 커밋 제목 끝에 관련 issue 번호를 `(#6)`처럼 붙인다.
* 의미 있는 작업마다 하나의 작업 지시 문서를 만든다.
* 작업이 끝나면 대응되는 작업 로그를 남긴다.
* 설계 없이 바로 코드만 추가하지 않는다.
* 초기 구조 작업이라도 디렉터리 목적과 향후 확장 방향을 문서로 남긴다.
* 소비자 프로젝트의 요구로 rexbox를 바꿀 때는 rexbox의 issue를 만들고, 소비자 쪽 issue를 본문에 링크한다.

## Task Unit Rules

* A task is created as a GitHub issue, and the task number is the GitHub issue number.
* Follow-up work on an existing task (verification left open, a follow-up fix) goes to that task's issue rather than a new one.
* End commit titles with the related issue number, as in `(#6)`.
* Create one work-order document for each meaningful task.
* When the task is complete, leave the corresponding work log.
* Do not add code directly without a design.
* Even for initial structure work, document the directory purpose and future extension direction.
* When a consumer project's need changes rexbox, open a rexbox issue and link the consumer's issue in its body.

---

## Git 작업 규칙

* 사용자가 작업을 요청하면 먼저 현재 Git 브랜치명을 확인한다.
* 현재 브랜치가 `main`이면 사용자가 요청한 작업 내용을 바탕으로 작업용 브랜치를 새로 만든 뒤 작업한다.
* 현재 브랜치가 `main`이 아니면 새 브랜치를 만들지 않고 그 브랜치에서 이어서 작업한다. 다른 issue의 작업이어도 같다. 커밋마다 제목 끝에 해당 issue 번호를 붙여 구분한다.
* 작업 단위가 하나 끝날 때마다 관련 변경을 Git 커밋으로 남긴다.
* 프로젝트 버전은 저장소 루트의 `VERSION` 파일에서 `major.minor.patch` 형식으로 관리한다.
* 사용자가 머지를 요청하면 `main`에 머지하기 전에 patch 버전을 1 증가시킨다.
* 사용자가 minor 버전 증가를 요청하면 minor 버전을 1 증가시키고 patch 버전은 0으로 리셋한다.
* 사용자가 major 버전 증가를 요청하면 major 버전을 1 증가시키고 minor와 patch 버전은 0으로 리셋한다.
* 사용자가 머지를 요청하면 작업 브랜치를 원격에 push하고 GitHub pull request를 만든 뒤(머지 요청이 이 push와 PR 생성의 승인이다), 그 PR을 squash merge해 `main`에 넣는다. 머지한 뒤 `git pull`로 로컬 `main`을 원격과 같게 갱신한다.
* PR 본문에는 해결하는 issue를 `Closes #6`처럼 적는다. 여러 issue를 해결하면 issue마다 적는다.
* squash merge의 커밋 제목은 작업 브랜치 안의 커밋 제목들을 확인하고, 전체 변경 내용을 잘 표현하는 제목으로 만든다.
* `main`에 머지한 뒤에는 그 머지 커밋에 `VERSION`과 같은 값의 annotated tag를 `vmajor.minor.patch` 형식으로 붙인다.
* tag 메시지에는 해당 버전의 핵심 변경을 한 줄로 남긴다.
* tag는 로컬까지만 만들고 원격 push는 사용자가 직접 수행한다.
* 머지가 완료되면 현재 작업 브랜치를 삭제한다.
* 릴리스 노트(`docs/release-notes/v<version>.md`)에는 해결된 issue 번호와 그 커밋 ID를 함께 적고, 소비자 프로젝트가 submodule을 올릴 때 해야 할 일(재생성, 계약 변경)을 적는다.

## Git Workflow Rules

* When the user requests work, first check the current Git branch name.
* If the current branch is `main`, create a task branch based on the requested work before making changes.
* If the current branch is not `main`, keep working on it without creating a new branch, even for a different issue. Each commit title ends with its own issue number.
* Leave a Git commit for the related changes whenever one task unit is complete.
* Manage the project version in the repository-root `VERSION` file using `major.minor.patch`.
* When the user requests a merge, increment the patch version by 1 before merging into `main`.
* When the user requests a minor version bump, increment the minor version by 1 and reset the patch version to 0.
* When the user requests a major version bump, increment the major version by 1 and reset the minor and patch versions to 0.
* When the user requests a merge, push the task branch, open a GitHub pull request from it (the merge request approves this push and the pull request), and squash-merge it into `main`. Afterwards update local `main` with `git pull`.
* Name the issues the pull request resolves in its description, as in `Closes #6`, one per issue.
* For the squash merge's commit title, inspect the branch's commit titles and write one that describes the complete change.
* After merging into `main`, tag that merge commit with an annotated tag matching `VERSION`, in the form `vmajor.minor.patch`.
* Put a one-line summary of the version's key change in the tag message.
* Create tags locally only; the user pushes them to the remote.
* Delete the task branch after the merge is complete.
* Release notes (`docs/release-notes/v<version>.md`) record the resolved issue numbers together with their commit IDs, and what a consumer project must do when it moves its submodule (regenerate, follow a contract change).

---

## 아키텍처 규칙

* 운영체제, XDK 라이브러리, 하드웨어 인터페이스만 HLE로 대체한다. 타이틀의 게임플레이 로직은 rexbox가 다루지 않는다.
* 모든 하위 시스템은 독립적으로 교체 가능해야 한다.
* 한 타이틀만을 위한 동작을 rexbox에 넣지 않는다. 타이틀마다 다른 것은 계약의 매개변수(프로파일, 기본값, 훅)로 만든다.

## Architecture Rules

* Replace only the operating system, XDK library and hardware interfaces with HLE. A title's gameplay logic is not rexbox's business.
* Every subsystem should be replaceable independently.
* Do not put behavior for one title alone into rexbox. What differs between titles becomes a parameter of a contract (a profile, a default, a hook).

---

## 개발 철학

여러 구현이 가능할 때는 원본 실행 파일과의 동작 일치를 보존하면서 리버스 엔지니어링 부담을 최소화하는 방법을 선택한다. 런타임 동작은 생성된 코드를 읽어서 추론하지 말고 실행해서 측정한다.

최적화보다 정확성을 우선한다.

## Development Philosophy

When multiple implementations are possible, choose the one that preserves behavioral parity with the original executable while minimizing reverse engineering effort. Measure runtime behavior by running it rather than inferring it from reading generated code.

Accuracy is preferred over optimization.
