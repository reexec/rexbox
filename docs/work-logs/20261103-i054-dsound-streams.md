# 작업 로그: DirectSound 스트림 (issue #54)

설계: [20261103-i054](../design/20261103-i054-dsound-streams.md) · 작업 지시: [20261103-i054](../work-orders/20261103-i054-dsound-streams.md) · 소비자: reexec/reEXCEED#4

## 한 일 (1단계)

* `dsound_stream.cpp`: 스트림(패킷 큐, 형식, 콜백·컨텍스트, 볼륨·헤드룸·주파수, 일시정지), 게스트 객체와 vtable 8항목(합성 주소 `0xFD000000`~), `DirectSoundCreateStream`, `IDirectSound_CreateSoundStream`, `AddRef`/`Release`/`GetInfo`/`GetStatus`/`Process`/`Discontinuity`/`Flush`, `IDirectSoundStream_Pause`/`SetVolume`/`SetHeadroom`/`SetFrequency`/`FlushEx`, 받기만 하는 설정 10개.
* 믹서가 스트림을 섞고, `DirectSoundDoWork`(이전에는 아무것도 하지 않음)가 완료를 전달한다.
* vtable은 첫 스트림을 만들 때 게스트 힙에 만든다(등록 시점에는 힙이 없을 수 있다).

## 검증

* Win32 x86 Debug(reEXCEED 트리): 오류 0, 경고 0. CTest 7/7.
* reEXCEED(`REXBOX_DSOUND_TRACE=1`): `[dsound] stream 03FDA4C0: 2 ch, 44100 Hz, 16 bit, 43 packets, callback 0007E9C0, flags 00000000, mix bins 00000000`. 다음 정지점은 D3D8LTCG `D3DDevice_CreateStateBlock`. 패킷 완료와 소리는 아직 확인하지 않았다(타이틀이 패킷을 넣기 전에 멈춤).

---

# Work log: DirectSound streams (issue #54)

Design: [20261103-i054](../design/20261103-i054-dsound-streams.md) · Work order: [20261103-i054](../work-orders/20261103-i054-dsound-streams.md) · Consumer: reexec/reEXCEED#4

Done (stage 1): `dsound_stream.cpp` with the stream (packet queue, format, callback and context, volume, headroom, frequency, pause), the guest object and its eight-entry vtable (synthetic addresses from `0xFD000000`), `DirectSoundCreateStream`, `IDirectSound_CreateSoundStream`, `AddRef`/`Release`/`GetInfo`/`GetStatus`/`Process`/`Discontinuity`/`Flush`, `IDirectSoundStream_Pause`/`SetVolume`/`SetHeadroom`/`SetFrequency`/`FlushEx` and ten settings that are accepted only; the mixer mixes streams, and `DirectSoundDoWork` (which did nothing before) delivers completions; the vtable is made in the guest heap with the first stream (the heap may not exist at registration).

Verification: Win32 x86 Debug (in the reEXCEED tree) with 0 errors and 0 warnings, CTest 7/7. In reEXCEED (`REXBOX_DSOUND_TRACE=1`): `[dsound] stream 03FDA4C0: 2 ch, 44100 Hz, 16 bit, 43 packets, callback 0007E9C0, flags 00000000, mix bins 00000000`; the next stop is D3D8LTCG `D3DDevice_CreateStateBlock`. Packet completion and sound are not checked yet (the title stops before it queues packets).
