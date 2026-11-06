# 설계: 타이틀의 라이브러리가 만든 픽셀 셰이더 (issue #73)

상태: **구현됨**. 작업 로그: [20261107-i073](../work-logs/20261107-i073-guest-pixel-shader.md). 소비자: reexec/reEXCEED#9.

## 1. 요구사항

그리기 경로는 HLE의 `D3DDevice_CreatePixelShader`가 준 핸들(작은 정수)만 알아서, 그 밖의 핸들로 그리는 호출을 건너뛰었다("… with unknown pixel shader %08X is not drawn"). Pump It Up: Exceed(XDK 5849)는 라이브러리의 `CreatePixelShader`를 lift된 코드로 실행한다(식별되지 않음). 그래서 핸들이 게스트 메모리의 라이브러리 객체(`0x04200360`)다. 곡 선택의 잠긴 곡은 이 셰이더로 회색조로 그리는데, 칸 그림과 큰 미리보기가 통째로 빠져 검게 보였다(**확인됨**, 프레임 캡처 `#16`·`#29`).

## 2. 설계

* lift된 `D3DDevice_SetPixelShader`(`0x00172480`)가 객체를 읽는 방식: `[handle + 8]`이 `D3DPIXELSHADERDEF`를 가리키고, 거기서 `+0x20`, `+0x24`, `+0xEC`를 읽는다(**확인됨**).
* `FindPixelShader`: 표에 없는 핸들이 유효한 게스트 주소이고 `[handle + 8]`이 정의(0xF0바이트)를 가리키면, 그 60 DWORD를 읽어 쓴다. 정의 메모리는 타이틀의 것이므로 그릴 때마다 다시 읽는다.
* 둘 다 아니면 전처럼 그리지 않는다.

## 3. 검증

* d3d8 probe와 CTest(HLE 핸들은 그대로).
* reEXCEED: 잠긴 곡의 칸 그림과 미리보기가 xemu처럼 회색조로 그려진다(사용자 확인).

---

# Design: a pixel shader the title's library created (issue #73)

Status: **implemented**. Work log: [20261107-i073](../work-logs/20261107-i073-guest-pixel-shader.md). Consumer: reexec/reEXCEED#9.

**Requirement.** The draw path knew only the handles the HLE's `D3DDevice_CreatePixelShader` gave out (small integers) and skipped any other draw ("… with unknown pixel shader %08X is not drawn"). Pump It Up: Exceed (XDK 5849) runs the library's `CreatePixelShader` as lifted code (it is not identified), so its handle is the library's object in guest memory (`0x04200360`). The song select draws locked songs grey through that shader, so their tile pictures and the big preview were dropped and came out black (**confirmed**, frame capture `#16` and `#29`).

**Design.** The lifted `D3DDevice_SetPixelShader` (`0x00172480`) shows how the object is read: `[handle + 8]` points to the `D3DPIXELSHADERDEF`, and it reads `+0x20`, `+0x24` and `+0xEC` from there (**confirmed**). `FindPixelShader` therefore handles a handle missing from its table this way: if it is a valid guest address and `[handle + 8]` points to a definition (0xF0 bytes), it reads those 60 DWORDs, again at every draw, since the title owns that memory. Anything else is still not drawn.

**Verification.** The d3d8 probe and CTest (HLE handles are unchanged). In reEXCEED the locked songs' tile pictures and preview draw grey as in xemu (the user's check).
