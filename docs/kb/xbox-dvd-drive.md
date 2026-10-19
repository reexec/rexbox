# Xbox DVD 드라이브: SCSI pass-through와 보안 페이지 0x3E

타이틀은 `NtDeviceIoControlFile(\Device\CdRom0, IOCTL_SCSI_PASS_THROUGH = 0x4D014)`로 드라이브에 SCSI 명령을 직접 보낼 수 있다. 커널 HLE(`src/hle/kernel/kernel_file.cpp`의 `ScsiPassThrough`)가 다루는 범위를 적는다.

출처: [xboxdevwiki: DVD Drive](https://xboxdevwiki.net/DVD_Drive)(페이지 0x3E 배치), NT DDK의 `SCSI_PASS_THROUGH`, Crazy Taxi 3(일본판) lift 코드 `sub_000E412D`의 호출 순서(**확인됨**: reexec/reCT3#3).

## `SCSI_PASS_THROUGH` (0x2C 바이트)

| 오프셋 | 필드 | 비고 |
|---|---|---|
| 0 | `Length` u16 | 0x2C |
| 2 | `ScsiStatus` u8 | 커널이 쓴다(0 = GOOD) |
| 6 | `CdbLength` u8 | |
| 8 | `DataIn` u8 | 1 = 드라이브 → 메모리 |
| 12 | `DataTransferLength` u32 | |
| 20 | `DataBufferOffset` | Xbox에서는 게스트 포인터 |
| 28 | `Cdb[16]` | |

## MODE SENSE(10) 페이지 0x3E (Xbox DVD 보안 페이지)

CDB `5A 00 3E 00 00 00 00 00 1C 00`: 페이지 0x3E, 할당 길이 0x1C. 답은 8바이트 모드 매개변수 헤더(모드 데이터 길이 big-endian, 자기 두 바이트 제외) 뒤에 20바이트 페이지다.

| 페이지 오프셋 | 필드 | HLE의 답 |
|---|---|---|
| 0 | 페이지 코드 | 0x3E |
| 1 | 길이(앞 두 바이트 제외) | 18 |
| 2 | 파티션 선택: 0 비디오, 1 Xbox | 1 |
| 3 | 1이 아니면 커널이 XGD로 인정하지 않음 | 1 |
| 4 | 인증 여부 | 1 |
| 5 | 북 타입(상위 4비트, Xbox 게임은 0xD)·버전 | 0xD1 |
| 7 | 챌린지 id | 0 |
| 8 / 12 | 챌린지 값 / 응답 값 u32 | 0 |

* **확인됨**(CT3): 부팅 직후 이 명령을 보내고 답의 바이트 11(페이지 오프셋 3)이 1이며 바이트 10 또는 12(파티션 또는 인증)가 0이 아니어야 계속한다. 아니면 `0xC0000014`/`0xC0000001`을 돌려주고 `HalReturnToFirmware`로 대시보드에 돌아간다.
* **미확정**: 다른 타이틀이 보내는 다른 CDB(챌린지·응답 MODE SELECT 등). HLE는 한 번 기록하고 0으로 채운 데이터로 성공을 돌려준다.

---

# Xbox DVD drive: SCSI pass-through and security page 0x3E

A title can send SCSI commands straight to the drive with `NtDeviceIoControlFile(\Device\CdRom0, IOCTL_SCSI_PASS_THROUGH = 0x4D014)`; this records what the kernel HLE (`ScsiPassThrough` in `src/hle/kernel/kernel_file.cpp`) handles. Sources: [xboxdevwiki: DVD Drive](https://xboxdevwiki.net/DVD_Drive) (page 0x3E layout), the NT DDK's `SCSI_PASS_THROUGH`, and the call sequence in Crazy Taxi 3's lifted `sub_000E412D` (**confirmed**, reexec/reCT3#3). The tables above give the structure (0x2C bytes; `DataBufferOffset` is a guest pointer on the Xbox) and the MODE SENSE(10) page 0x3E answer: an 8-byte mode parameter header, then the 20-byte page with partition select 1, byte 3 = 1, authenticated 1, book type D version 1 and the challenge fields at zero. **Confirmed** (CT3): right after boot it sends this command and continues only when byte 11 of the answer (page offset 3) is 1 and byte 10 or 12 (partition or authenticated) is non-zero; otherwise it returns `0xC0000014`/`0xC0000001` and goes back to the dashboard through `HalReturnToFirmware`. **Unresolved**: other CDBs other titles send (challenge/response MODE SELECT and the like); the HLE traces them once and completes them with zeroed data.
