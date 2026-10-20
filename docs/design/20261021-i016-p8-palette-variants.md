# 설계: P8 텍스처의 팔레트별 캐시 (issue #16)

## 1. 요구사항

소비자 reexec/reCT3#11: 도시 장면에서 프레임당 D3D8 `setup`이 최대 12 ms다. 표시 스레드를 샘플링하면 vblank 대기 다음으로 텍스처 디코딩(`SwizzleOffset`, `DecodePalettedLevel`)이 가장 많다. 업로드 원인을 세어 보면 타이틀이 **같은 P8 텍스처 헤더를 한 프레임 안에서 최대 7개의 팔레트로** 그리고(하나의 텍스처, 여러 색), 팔레트가 바뀔 때마다 다시 디코딩한다(프레임당 최대 42회). 재디코딩 수와 `setup` 시간이 함께 움직인다(42회 12.4 ms, 0.3~2.8회 2.6 ms).

## 2. 현재 구조

`ResolveTexture(header, palette)`는 `g_textures[header]` 하나에 GL 텍스처 하나를 둔다. P8이면 그 캐시에 업로드 당시의 팔레트(헤더 주소, `Data`, `Common`, 항목 해시)를 적고, 다른 팔레트가 오면 다시 올린다. `MarkTextureWritten(header)`(Lock/쓰기)는 `written`을 세우고, `ForgetTexture(header)`(Release)는 GL 텍스처를 지운다. 텍스처 데이터의 무잠금 쓰기는 프레임마다 한 번 표본 해시로 확인한다.

## 3. 설계

* `g_textures`의 값을 **헤더 항목**으로 바꾼다. 항목은 변형(`CachedTexture`)의 작은 목록을 가진다.
  * P8이 아니면 변형은 하나다(지금과 같다).
  * P8이면 변형은 팔레트의 `Data`(물리 주소)와 `Common`(항목 수)로 구별한다. 팔레트 리소스 헤더 주소가 달라도 같은 `Data`면 같은 변형이다. 변형마다 GL 텍스처, 팔레트 해시, 데이터 해시, 확인한 프레임을 가진다.
* 조회: 키가 같은 변형을 찾고, 없으면 새로 만든다. 헤더 필드(Format, Size, Data)가 바뀌면 모든 변형이 무효가 되어야 하므로, 변형마다 지금처럼 헤더 필드를 비교해 다시 올린다.
* 상한: 헤더당 변형 16개. 넘으면 가장 오래 쓰이지 않은 변형의 GL 텍스처를 지우고 그 자리를 쓴다.
* `MarkTextureWritten`: 모든 변형의 `written`을 세운다. `ForgetTexture`: 모든 변형의 GL 텍스처를 지운다. `TextureCacheBytes`: 모든 변형을 센다.
* 팔레트가 없는 P8(스테이지에 팔레트 없음)은 지금처럼 흰색으로 보고하며 변형 키 0을 쓴다.

비용: 변형마다 RGBA8 사본 하나(256×256이면 256 KiB). 소비자에서 P8 헤더 약 18개 × 최대 7개.

## 4. 검증

* 소비자 실행에서 같은 구간(present 1500~2100)의 `REXBOX_D3D8_PROFILE` `setup`과 fps를 비교한다. 재디코딩은 변형마다 첫 사용 때만 일어나야 한다(임시 계측으로 확인).
* 화면 비교: 같은 프레임 덤프에서 P8 텍스처(차량 색)가 바뀌지 않아야 한다(실행마다 시점이 달라 눈으로 확인).
* CTest(probe 전부).

---

# Design: caching P8 textures per palette (issue #16)

## 1. Requirement

Consumer reexec/reCT3#11: in the city scenes D3D8 `setup` takes up to 12 ms per frame. Sampling the presenting thread puts texture decoding (`SwizzleOffset`, `DecodePalettedLevel`) first after the vblank wait. Counting uploads by cause shows the title drawing **the same P8 texture header with up to 7 palettes in one frame** (one texture, several colourings), re-decoded at every palette switch (up to 42 per frame); the re-decodes and `setup` move together (12.4 ms at 42, 2.6 ms at 0.3–2.8).

## 2. Current structure

`ResolveTexture(header, palette)` keeps one GL texture in `g_textures[header]`. For P8 it records the palette of the upload (its header address, `Data`, `Common` and a hash of its entries) and uploads again when another palette comes. `MarkTextureWritten(header)` (locks and writes) sets `written`, and `ForgetTexture(header)` (release) deletes the GL texture. Lock-free writes to texture data are caught by a sampled hash once per frame.

## 3. Design

* `g_textures` maps to a **header entry** holding a small list of variants (`CachedTexture`). A non-P8 texture has one variant (as now). A P8 texture's variants are told apart by the palette's `Data` (physical address) and `Common` (entry count); palette resource headers at different addresses with the same `Data` share a variant. Each variant has its own GL texture, palette hash, data hash and checked frame.
* Lookup finds the variant with the same key, or makes one. A change in the header's fields (Format, Size, Data) must invalidate every variant, so each variant compares them as now and uploads again.
* Bound: 16 variants per header; past that the least recently used variant's GL texture is deleted and its slot reused.
* `MarkTextureWritten` sets `written` on every variant, `ForgetTexture` deletes every variant's GL texture, and `TextureCacheBytes` counts every variant.
* A P8 texture with no palette on its stage is reported and sampled as white as now, with variant key 0.

Cost: one RGBA8 copy per variant (256 KiB at 256×256); in the consumer about 18 P8 headers × up to 7.

## 4. Verification

* In the consumer, compare `REXBOX_D3D8_PROFILE`'s `setup` and fps over the same span (presents 1500–2100); re-decodes must happen only at each variant's first use (checked with temporary counters).
* Pictures: in frame dumps the P8 textures (vehicle colours) must not change (the demo's timing differs between runs, so by eye).
* CTest (every probe).
