# 설계: NtQueryFullAttributesFile (issue #69)

상태: **구현됨**. 작업 로그: [20261106-i069](../work-logs/20261106-i069-query-full-attributes.md). 소비자: reexec/reEXCEED#9.

## 1. 요구사항

커널 import 210 `NtQueryFullAttributesFile(ObjectAttributes, FileInformation)`에 HLE가 없었다. Pump It Up: Exceed는 곡이 끝난 뒤 저장할 때 세이브 폴더(`U:\33A446FC39CC\`)의 속성을 묻다가 여기서 멈췄다(**확인됨**).

## 2. 설계

* VFS로 이름을 풀고, `NtQueryInformationFile` class 34와 같은 `FILE_NETWORK_OPEN_INFORMATION`(시각, 할당 크기, 파일 끝, 속성, 0x38바이트)을 쓴다.
* 이름이 없으면 `STATUS_OBJECT_NAME_NOT_FOUND`, 디렉터리가 없으면 `STATUS_OBJECT_PATH_NOT_FOUND`, 장치 자체(raw device)도 path not found다.

## 3. 검증

* kernel probe와 CTest. reEXCEED에서 GAME OVER 뒤 저장을 지나 메뉴로 돌아온다.

---

# Design: NtQueryFullAttributesFile (issue #69)

Status: **implemented**. Work log: [20261106-i069](../work-logs/20261106-i069-query-full-attributes.md). Consumer: reexec/reEXCEED#9.

**Requirement.** Kernel import 210 `NtQueryFullAttributesFile(ObjectAttributes, FileInformation)` had no HLE. Pump It Up: Exceed stopped there when, saving after a song, it asked for the save folder's attributes (`U:\33A446FC39CC\`) (**confirmed**).

**Design.** The name is resolved through the VFS, and the same `FILE_NETWORK_OPEN_INFORMATION` as `NtQueryInformationFile` class 34 is written: the times, the allocation size, the end of file and the attributes, 0x38 bytes. The status codes are:
- a missing name gives `STATUS_OBJECT_NAME_NOT_FOUND`;
- a missing directory gives `STATUS_OBJECT_PATH_NOT_FOUND`;
- a raw device gives path not found too.

**Verification.** The kernel probe and CTest. In reEXCEED the save after GAME OVER goes through and the game returns to the menu.
