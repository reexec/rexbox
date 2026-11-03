# 작업 지시: DirectSound 스트림 (issue #54)

설계: [20261103-i054](../design/20261103-i054-dsound-streams.md)

1. `src/hle/dsound/dsound_stream.cpp`: 스트림 상태, 게스트 객체와 vtable(합성 주소 바인딩), `DirectSoundCreateStream`, `IDirectSound_CreateSoundStream`, XMediaObject 메서드, `IDirectSoundStream_*` 설정, `DirectSoundDoWork`.
2. `dsound_mixer.cpp`의 `MixLocked`가 `MixStreamsLocked`를 부른다. `dsound_internal.h`에 선언, `dsound_hle.cpp`에 등록, `src/CMakeLists.txt`에 소스.
3. 빌드, probe, reEXCEED 실행. 작업 로그를 쓰고 커밋한다(`(#54)`).

---

# Work order: DirectSound streams (issue #54)

Design: [20261103-i054](../design/20261103-i054-dsound-streams.md)

1. `src/hle/dsound/dsound_stream.cpp`: stream state, the guest object and its vtable (synthetic addresses bound), `DirectSoundCreateStream`, `IDirectSound_CreateSoundStream`, the XMediaObject methods, the `IDirectSoundStream_*` settings, `DirectSoundDoWork`.
2. `MixLocked` in `dsound_mixer.cpp` calls `MixStreamsLocked`; declarations in `dsound_internal.h`, registration in `dsound_hle.cpp`, the source in `src/CMakeLists.txt`.
3. Build, probes, a reEXCEED run; write the work log and commit (`(#54)`).
