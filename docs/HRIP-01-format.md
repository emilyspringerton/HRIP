# HRIP-01 — Hearthstone bundle format (partial, verified on one real bundle)

Status: **partial, further along.** Container, SerializedFile type trees, object table, and CardDef values are
decoded and verified on one bundle. The card *stats* (dbf_id, name, cost, text) are in a different bundle,
`Data\Win\dbf.unity3d` (44,314,879 bytes), which is not yet decoded. See "Where card data actually lives". Source: one real bundle, `carddef_base_global-0516e06c-prefab-0.unity3d` (67,510 bytes),
from the founder's Windows install at `C:\Program Files (x86)\Hearthstone\Data\Win\`, pulled through
EDGE.GAME `exec` (certutil base64, reassembled and hash-checked). Decoder is a throwaway pure-Python
spike, not shipped code.

## Install layout (verified)

- `Hearthstone.exe`, `UnityPlayer.dll` (Unity), `Hearthstone_Data\`, `Strings\enUS\` (UI text only:
  `GLUE.txt`, `GLOBAL.txt`, and similar; no card names).
- `Data\Win\` holds roughly 960 bundles. Naming:
  - `carddef_base_global-<hash>-prefab-<N>.unity3d` — **card definitions** (the target).
  - `cardasset_*`, `cardtexture_*` — art and materials (deferred per NORTHSTAR).
  - `asset_manifest.unity3d` — not yet examined.

## Container: UnityFS (verified)

Header, big-endian throughout:

| Field | Value on sample |
|---|---|
| signature | `UnityFS\0` |
| format version | 8 |
| unity version / revision | `5.x.x` / `0.0.0` |
| total size | 67,510 (matches file size) |
| blocks-info compressed / uncompressed | 77 / 101 |
| flags | `0x243` |

Flag decoding that held on this sample:

- low 6 bits = codec. `3` = LZ4HC. Same block format as LZ4, so the monorepo's LZ4 default applies.
- `0x80` = blocks-info at end of file. **Not set** here.
- `0x200` = pad to 16 bytes before blocks-info. **Set** here.

Layout on the sample:

1. Header ends at offset 44. Pad to 48.
2. Blocks-info: LZ4 block, 77 compressed bytes, decodes to 101 bytes at offset 48.
3. Data region starts at the next 16-byte boundary after blocks-info, offset 128.
4. Blocks-info (101 bytes): 16-byte GUID, then 2 data blocks, then 1 node.
   - Block 1: 131,072 uncompressed, 40,697 compressed, flags `0x3` (LZ4).
   - Block 2: 85,176 uncompressed, 26,685 compressed, flags `0x3` (LZ4).
   - Node: `CAB-e8db3854066f260bafb889d6e1f75bdb`, offset 0, size 216,248, flags 4.
5. Both blocks decompress to exactly the stated sizes, total 216,248 (matches the node size).

## Inner payload: Unity SerializedFile (`CAB-…`), 216,248 bytes (decoded)

- Header (big-endian): metadata size at 20, file size i64 at 24 (= 216,248), version 22 at 8, data offset
  i64 at 32 (= 41,104), endian byte 0 (little-endian body).
- Metadata (little-endian), from offset 48: unity version cstring `0.0.0`, platform i32 = 19
  (StandaloneWindows64), has-type-tree u8 = 1, then type count i32 = 7 (read unaligned at offset 59).
- Each type: class ID i32, stripped u8, script index i16, [16-byte script hash if class 114 or <0],
  16-byte old-type hash, **type-tree blob**, i32 dependency count (0).
- Type-tree blob: node count u32, string-buffer length u32, then nodes of **32 bytes**: version u16,
  level u8, isArray u8, typeOffset u32, nameOffset u32, size i32, index i32, meta u32, ref-hash u64. Strings
  live in the buffer; an offset with the high bit set is a Unity common-string index. The table below
  was checked against data, and the names resolved from it match the field names found in the payload.
- Seven types in this bundle: MonoScript (115), two MonoBehaviour scripts (114), Transform (4), GameObject (1),
  AssetBundle (142), and one class 82.
- Object table: count i32, then 24-byte records (pathID i64, byteStart i64 from the data offset, byteSize u32,
  typeIdx i32). The class comes from the type. 446 objects in this bundle, ending at 41,060.
- MonoScripts resolve to class names **`SoundDef`** and **`CardDef`** (Assembly-CSharp).
- Decoded all 147 `CardDef` objects with the type tree. Each has presentation and effect fields (portrait,
  material, and effect-def paths, plus flags). Card-set-specific art names carry the card IDs (e.g.
  `REV_244`, `MAW_009`, `HS26-058`).

### Where card data actually lives

`CardDef` does **not** contain cost, attack, health, name, text, or `dbf_id`. Those belong to the card
database `Data\Win\dbf.unity3d` (44 MB), which is the real HRIP-02 target. Earlier notes suggesting
that the carddef bundles hold stats were wrong.

## Earlier notes (now superseded where they conflict)

The decompressed payload is a Unity `SerializedFile`. It contains readable type-tree field names
(`m_ExecutionOrder`, `m_PropertiesHash`, `m_ClassName`, `m_Namespace`, `m_PathID`, and many more).
That suggests the type trees are embedded, so object fields can be decoded without external type
info. **Not yet verified and not yet implemented.**

Card-relevant strings found in the payload:

- `CardDef` and `CardDefSpecialEvent` MonoBehaviour class names.
- **129 card-ID-shaped strings**, e.g. `REV_307`, `MAW_023e`, `REV_248t`. The `e` and `t` suffixes
  look like encoded and token variants. Card-ID format is `SET_NNN[variant]`.
- Asset references to textures, materials, and prefabs (art, out of scope for V0).

## What is NOT yet found

- **Card name, text, cost, attack, health, class, rarity, and set are not visible as plain strings** in
  this bundle. They are either in binary object fields, which need the type-tree decode, or in a
  different bundle. No localized card-text bundle was found by filename search.
- `dbf_id` (WOTAN's `hs_cards` key) has not been located. Card IDs are not dbf IDs, and the mapping
  is still unknown.

## Next steps

1. **Get `dbf.unity3d` (44 MB) off the Windows machine.** Blocked: the deployed client has no working
   `file_get`, and a 44 MB base64 stream through exec chunks is impractical. The `file_get` change is
   uncommitted in EDGE.GAME and needs a decision before anyone commits it. No real Python exists on the
   machine (the `python.exe` in WindowsApps is the Store stub).
2. Decode `dbf.unity3d` with the same pipeline (type trees, objects), find the card table, and map
   `dbf_id` → name/cost/class/rarity/set/text. Cross-check the card IDs against `CardDef`.
3. Repeat across any other card-table bundles, then diff against HearthstoneJSON as the verification
   reference only (NORTHSTAR §5, HRIP-05).
4. Productize: the decoder must become PARENA (HRIP's core-dep rule). The Python spike is only the spec.

## Tooling caveats

- Pulling bytes: the Windows client's `file_get` is not live (the deployed build predates it), and
  PowerShell exceeds the relay's response timeout (about 4 s). What works is `certutil -encode` to a
  temp file, then reading it back by line with `more +N | find /n /v ""`. Note that `more +N` starts at
  line N+1, not N. The exec output cap is 48 KB, so long files need several reads. This is slow.
- Temp files created on the Windows machine during the spike were removed.
