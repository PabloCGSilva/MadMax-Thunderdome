"""Byte signatures for Thunderdome.asi, generated from the GOG AVAMain.exe (timestamp 0x566520B2).

Each function signature starts at the function entry; RIP-relative displacements, call/jmp targets and address-like
immediates are wildcards; the pattern grows instruction by instruction until it matches once in .text.
For the global s_SpawnSystemContext the signature starts at an instruction that loads it (mov reg, [rip+disp32]);
the plugin reads the displacement from the match.

usage: python gen_sigs.py   (needs pefile + capstone)
"""
import re
import struct

import capstone
import pefile
from capstone import x86

EXE = r'C:\Games\Mad Max\AVAMain.exe'
pe = pefile.PE(EXE, fast_load=True)
BASE = pe.OPTIONAL_HEADER.ImageBase
TEXT_SEC = [s for s in pe.sections if s.Name.startswith(b'.text')][0]
TEXT = TEXT_SEC.get_data()
TEXT_VA = BASE + TEXT_SEC.VirtualAddress
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
md.detail = True

FUNCS = [
    ('CMissionManager::Load', 0x14047A9F0),
    ('CMissionManager::SetRestartPointEnable', 0x1404687E0),
    ('CArenaSpawner::UpdatePostSim', 0x14055BB40),
    ('CSpawnedEntityHandler::GetResourceSetSpawnTypeId', 0x1404F6EF0),
    ('CSpawnedEntityHandler::LoadResourceSets', 0x140548B20),
]
GLOBAL = ('s_SpawnSystemContext', 0x14171B678)


def count(b, m):
    rx = b''.join(re.escape(bytes([x])) if mm == 'x' else b'.' for x, mm in zip(b, m))
    return len(re.findall(rx, TEXT, re.S)) if len(rx) else 0


def masked(va, min_len=20, max_len=128, keep_first_disp=False):
    off = va - TEXT_VA
    out_b, out_m = bytearray(), []
    first = True
    for insn in md.disasm(TEXT[off:off + max_len], va):
        b = bytearray(insn.bytes)
        m = ['x'] * len(b)
        rip = insn.disp_offset and any(op.type == x86.X86_OP_MEM and op.mem.base == x86.X86_REG_RIP for op in insn.operands)
        if rip and not (first and keep_first_disp):
            for i in range(insn.disp_offset, insn.disp_offset + insn.disp_size):
                m[i] = '?'
        if (insn.mnemonic in ('call', 'jmp') or insn.mnemonic.startswith('j')) and insn.imm_offset:
            for i in range(insn.imm_offset, insn.imm_offset + insn.imm_size):
                m[i] = '?'
        for op in insn.operands:
            if op.type == x86.X86_OP_IMM and insn.imm_size >= 4 and 0x140000000 <= op.imm < 0x143000000:
                for i in range(insn.imm_offset, insn.imm_offset + insn.imm_size):
                    m[i] = '?'
        out_b += b
        out_m += m
        first = False
        if len(out_b) >= min_len and count(out_b, out_m) == 1:
            break
    return bytes(out_b), out_m


def fmt(b, m):
    return ' '.join('%02X' % x if mm == 'x' else '??' for x, mm in zip(b, m))


for name, va in FUNCS:
    b, m = masked(va)
    print('    { "%s", "%s" },  // 0x%X, %d bytes, %d match(es)' % (name, fmt(b, m), va, len(b), count(b, m)))

# instructions that load the global: 48/4C 8B ?? [rip+disp32] whose target is the global
gname, gva = GLOBAL
best = None
for mo in re.finditer(rb'[\x48\x4C]\x8B[\x05\x0D\x15\x1D\x25\x2D\x35\x3D]', TEXT):
    i = mo.start()
    disp = struct.unpack_from('<i', TEXT, i + 3)[0]
    if TEXT_VA + i + 7 + disp != gva:
        continue
    # pattern = the load itself (displacement wildcarded) + the following instructions
    b, m = masked(TEXT_VA + i, min_len=16, max_len=64)
    m = list(m)
    for k in range(3, 7):
        m[k] = '?'
    c = count(b, m)
    if c == 1 and (best is None or len(b) < len(best[1])):
        best = (TEXT_VA + i, b, m)
if best:
    print('    { "%s (mov reg,[rip+disp32] at +0)", "%s" },  // insn at 0x%X, %d bytes' % (gname, fmt(best[1], best[2]), best[0], len(best[1])))
else:
    print('no unique load of', gname)
