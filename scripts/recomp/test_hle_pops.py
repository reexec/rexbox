"""hle_pops on synthetic HLE sources and lifted ret operands (no game data)."""

import pathlib
import sys
import unittest

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from hle_pops import compare_pops, hle_pop_counts, pop_counts_in  # noqa: E402

SOURCE = """\
void CreateDevice()
{
    HleCall call;
    call.Return(0, 7);
}

template <uint32_t N>
void AcceptSetting()
{
}

void Helper()
{
}

void Register(HleRegistry& registry)
{
    registry.AddExport("D3D8", "Direct3D_CreateDevice", &CreateDevice);
    registry.AddExport("DSOUND", "IDirectSoundBuffer_SetVolume", &AcceptSetting<2>);
    registry.AddExport("D3D8", "D3D_Helper", &Helper);
}
"""


class PopCounts(unittest.TestCase):
    def test_parse(self):
        self.assertEqual(pop_counts_in(SOURCE), {
            "Direct3D_CreateDevice": {7},
            "IDirectSoundBuffer_SetVolume": {2},
            "D3D_Helper": set(),
        })

    def test_compare(self):
        hle = pop_counts_in(SOURCE)
        names = {0x100: "Direct3D_CreateDevice", 0x200: "IDirectSoundBuffer_SetVolume", 0x300: "D3D_Helper",
                 0x400: "NotHle"}
        lines, mismatches = compare_pops(names, {0x100: [28], 0x200: [4], 0x300: [8]}, hle)
        self.assertEqual(mismatches, 1)
        self.assertTrue(any("IDirectSoundBuffer_SetVolume" in line and "MISMATCH" in line for line in lines))
        self.assertTrue(any("D3D_Helper" in line and "not parsed" in line for line in lines))
        self.assertFalse(any("Direct3D_CreateDevice" in line for line in lines))  # matches hidden
        self.assertEqual(lines[-1], "1 mismatch(es)")
        lines, _ = compare_pops(names, {0x200: [8]}, hle, show_all=True)
        self.assertTrue(any("Direct3D_CreateDevice" in line and "no ret" in line for line in lines))
        self.assertTrue(any("IDirectSoundBuffer_SetVolume" in line and "MISMATCH" not in line for line in lines))

    def test_rexbox_sources_parse(self):
        # The real HLE sources: the D3D8 module registers exports with pops.
        hle = hle_pop_counts()
        self.assertGreater(len(hle), 50)
        self.assertTrue(any(pops for pops in hle.values()))


if __name__ == "__main__":
    unittest.main()
