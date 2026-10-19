# 설계: 커널 HLE의 IOCTL_SCSI_PASS_THROUGH (issue #6)

## 1. 요구사항

Crazy Taxi 3(일본판, reexec/reCT3#3)의 첫 실행이 `NtDeviceIoControlFile(\Device\CdRom0, 0x4D014)` 직후 `HalReturnToFirmware`에서 멈춘다. `ControlFile`은 모르는 IOCTL을 0으로 채운 출력과 성공으로 답하는데, 타이틀은 그 답(DVD 보안 페이지)이 "인증된 Xbox 파티션"이라고 말해야 계속한다.

## 2. 설계

```mermaid
sequenceDiagram
    participant T as title (sub_000E412D)
    participant K as kernel HLE ControlFile
    participant S as ScsiPassThrough
    T->>K: NtDeviceIoControlFile(CdRom0, 0x4D014, SCSI_PASS_THROUGH{CDB 5A 00 3E .. 1C 00, DataIn, 0x1C, buffer})
    K->>S: request, length
    S->>S: trace the CDB once; ScsiStatus = GOOD; zero the data
    alt MODE SENSE(10) page 0x3E
        S-->>T: header + page 0x3E (partition 1, byte 3 = 1, authenticated 1, book D1)
    else other command
        S-->>T: zeroed data
    end
    K-->>T: STATUS_SUCCESS, Information = request length
```

* `kernel_file.cpp`: `kIoctlScsiPassThrough`와 `ScsiPassThrough()`. 구조체 배치와 페이지 0x3E의 답은 [docs/kb/xbox-dvd-drive.md](../kb/xbox-dvd-drive.md). 데이터 버퍼는 구조체의 `DataBufferOffset`(게스트 포인터)이며 IOCTL의 OutputBuffer는 0이다.
* 타이틀을 모르는 답이다: 어떤 타이틀이 물어도 "이 디스크는 인증된 Xbox 게임 디스크"라고 답하는 것이 호스트에서 디스크 폴더를 읽는 런타임의 사실과 맞는다.
* 다른 CDB는 한 번 기록하고 0 데이터로 성공한다. 챌린지·응답이 필요한 타이틀이 나오면 그때 넓힌다(**미확정**).

## 3. 검증

* reCT3가 이 변경으로 `HalReturnToFirmware`를 지나 다음 frontier로 간다(reCT3 작업 로그 i003).
* probe: 파일 객체 없이는 `NtDeviceIoControlFile`을 부를 수 없어 `rexbox_kernel_probe`에는 넣지 않는다. Win32 빌드와 기존 probe 통과.

---

# Design: IOCTL_SCSI_PASS_THROUGH in the kernel HLE (issue #6)

Crazy Taxi 3's first run (reexec/reCT3#3) stops at `HalReturnToFirmware` right after `NtDeviceIoControlFile(\Device\CdRom0, 0x4D014)`: `ControlFile` answered the unknown IOCTL with zeroed output and success, while the title continues only when that answer (the DVD security page) says the disc is an authenticated Xbox partition. `ScsiPassThrough()` in `kernel_file.cpp` parses the `SCSI_PASS_THROUGH` request (the data buffer is its `DataBufferOffset`, a guest pointer; the IOCTL's OutputBuffer is 0), sets `ScsiStatus` GOOD, zeroes the data, and answers MODE SENSE(10) page 0x3E with the header and page described in [docs/kb/xbox-dvd-drive.md](../kb/xbox-dvd-drive.md) (partition 1, byte 3 = 1, authenticated 1, book type D1); other CDBs are traced once and complete with zeroed data (challenge/response support when a title needs it, **unresolved**). The answer knows no title: a runtime reading the disc folder from the host is, by its nature, an authenticated game disc. Verified by reCT3 passing `HalReturnToFirmware` to its next frontier (reCT3 work log i003); the kernel probe cannot call the IOCTL without a file object, so the Win32 build and the existing probes are the library's own check.
