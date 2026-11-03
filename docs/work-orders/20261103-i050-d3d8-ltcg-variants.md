# 작업 지시: D3D8LTCG의 레지스터 규약 변형 (issue #50)

설계: [20261103-i050](../design/20261103-i050-d3d8-ltcg-variants.md)

1. `HleCall`: `Eax()`, `Ebx()`, `Esi()`, `Edi()`.
2. `d3d8_device.cpp`: `CreateDevice` 핵심 함수, `Direct3D_CreateDevice`와 `Direct3D_CreateDevice_4__LTCG_eax1_ecx3` export.
3. 빌드, probe, reEXCEED 실행. 다음 변형은 소비자가 닿을 때 같은 방식으로 더한다.

---

# Work order: D3D8LTCG's register-convention variants (issue #50)

Design: [20261103-i050](../design/20261103-i050-d3d8-ltcg-variants.md)

1. `HleCall`: `Eax()`, `Ebx()`, `Esi()`, `Edi()`.
2. `d3d8_device.cpp`: the `CreateDevice` core and the `Direct3D_CreateDevice` and `Direct3D_CreateDevice_4__LTCG_eax1_ecx3` exports.
3. Build, probes, a reEXCEED run; further variants are added the same way as a consumer reaches them.
