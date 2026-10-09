import struct, sys, json
sys.path.insert(0, sys.path[0])
from serialized import parse, R
def subtree_end(nodes, i):
    l = nodes[i][0]; j = i + 1
    while j < len(nodes) and nodes[j][0] > l: j += 1
    return j
def decode(r, nodes, i):
    lvl, arr, tname, name, size, meta = nodes[i]
    end = subtree_end(nodes, i)
    if arr:  # array node: children size, data
        n = r.i32()
        d = i + 2
        dl, da, dt, dn, dsz, dm = nodes[d]
        if dsz == 1 and subtree_end(nodes, d) == d + 1:
            v = bytes(r.b[r.p:r.p+n]); r.p += n
        else:
            v = [decode(r, nodes, d) for _ in range(n)]
        if meta & 0x4000: r.align(4)
        return v
    if end == i + 1:
        if size == 1: v = r.u8()
        elif size == 2: v = r.i16()
        elif size == 4: v = r.i32()
        elif size == 8: v = r.i64()
        else: raise Exception('size %d %s' % (size, name))
        if meta & 0x4000: r.align(4)
        return v
    # struct or string
    if len(nodes[i+1:end]) and nodes[i+1][1] == 1 and tname in ('string','common840') or (nodes[i+1][1]==1 and nodes[i+1][2]=='common49' and name!='Records' and end == i+4):
        v = decode(r, nodes, i + 1)
        if isinstance(v, (bytes, bytearray)):
            v = v.decode('utf8', 'replace')
        if meta & 0x4000: r.align(4)
        return v
    d = {}
    j = i + 1
    while j < end:
        d[nodes[j][3]] = decode(r, nodes, j)
        j = subtree_end(nodes, j)
    if meta & 0x4000: r.align(4)
    return d
def load(path):
    b, types, objs = parse(path)
    out = {}
    for pid, off, sz, ti in objs:
        cid, si, nodes = types[ti]
        if cid != 114: continue
        # find Records node
        recs = [k for k, n in enumerate(nodes) if n[3] == 'Records']
        if not recs: continue
        # skip header: m_GameObject(12) m_Enabled(1) m_Script(12) m_Name(string)
        r = R(b, off, '<')
        r.i32(); r.i64(); r.u8(); r.align(4); r.i32(); r.i64()
        L = r.i32(); r.p += L; r.align(4)
        k = recs[0]
        recs_v = decode(r, nodes, k)
        rec_t = nodes[k][2]
        if isinstance(recs_v, dict) and len(recs_v) == 1: recs_v = list(recs_v.values())[0]
        out[rec_t] = recs_v
    return out
if __name__ == '__main__':
    out = load(sys.argv[1])
    for k, v in out.items(): print(k, len(v))
    json.dump(out, open(sys.argv[2], 'w'))
