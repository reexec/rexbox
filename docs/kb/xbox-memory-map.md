# Xbox 가상 메모리 지도: 물리 메모리 창

출처: [xboxdevwiki: Memory](https://xboxdevwiki.net/Memory), upstream xboxrecomp `src/kernel/xbox_memory_layout.c`(MIT; 주석의 사실만), Crazy Taxi 3 lift 코드(**확인됨**, reexec/reCT3#3).

| 가상 주소 | 내용 | 비고 |
|---|---|---|
| `0x00010000`… | XBE 이미지와 가상 메모리(`NtAllocateVirtualMemory`) | rexbox의 게스트 메모리 평면 영역 |
| `0x80000000` + P | 물리 RAM 64 MiB의 두 번째 매핑(캐시됨). `MmAllocateContiguousMemory`가 돌려주는 주소, "contiguous window" | rexbox `GuestMemory::kPhysicalWindow` |
| `0xF0000000` + P | 같은 RAM의 write-combined 매핑 | rexbox `GuestMemory::kWriteCombinedWindow` |
| `0xFD000000`… | NV2A, MCPX 등 장치 레지스터 | rexbox는 매핑하지 않는다(API 경계 HLE) |

* **확인됨**(CT3): 타이틀 코드는 D3D 리소스의 `Data`(물리 주소)에 `0x80000000`을 OR해 CPU 포인터를 만들고(30곳), `0xF0000000`도 쓴다(11곳). 예: `sub_000ABAA0`이 텍스처 데이터 주소로 파일을 읽는다.
* **확인됨**(reJSRF): JSRF는 이 창을 쓰지 않는다. 그래서 reJSRF의 게스트 메모리는 평면 128 MiB였다.
* rexbox: `GuestMemory::Options::physical_window`를 켜면 RAM을 두 창에 다시 매핑한다([설계 i008](../design/20261020-i008-physical-window.md)).

---

# Xbox virtual memory map: the physical memory windows

Sources: [xboxdevwiki: Memory](https://xboxdevwiki.net/Memory), upstream xboxrecomp's `src/kernel/xbox_memory_layout.c` (MIT; facts from its comments only), Crazy Taxi 3's lifted code (**confirmed**, reexec/reCT3#3). The table above: the XBE image and virtual memory from `0x00010000` (rexbox's flat region), physical RAM mapped again at `0x80000000` + P (cached, the contiguous window `MmAllocateContiguousMemory` returns addresses in; rexbox `kPhysicalWindow`) and at `0xF0000000` + P (write-combined; `kWriteCombinedWindow`), and the device registers from `0xFD000000` (not mapped by rexbox, which replaces the APIs). **Confirmed** (CT3): the title turns a D3D resource's `Data` into a CPU pointer with `| 0x80000000` (30 places) and uses `0xF0000000` too (11 places), e.g. `sub_000ABAA0` reads a file straight into a texture. **Confirmed** (reJSRF): JSRF never uses the window, which is why reJSRF's guest memory was a flat 128 MiB. rexbox maps the RAM again at both windows when `GuestMemory::Options::physical_window` is on ([design i008](../design/20261020-i008-physical-window.md)).
