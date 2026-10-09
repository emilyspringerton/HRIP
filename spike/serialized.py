import struct, sys
class R:
    def __init__(s, b, p=0, e='<'): s.b=b; s.p=p; s.e=e
    def u8(s): v=s.b[s.p]; s.p+=1; return v
    def i16(s): v,=struct.unpack_from(s.e+'h',s.b,s.p); s.p+=2; return v
    def u16(s): v,=struct.unpack_from(s.e+'H',s.b,s.p); s.p+=2; return v
    def i32(s): v,=struct.unpack_from(s.e+'i',s.b,s.p); s.p+=4; return v
    def u32(s): v,=struct.unpack_from(s.e+'I',s.b,s.p); s.p+=4; return v
    def i64(s): v,=struct.unpack_from(s.e+'q',s.b,s.p); s.p+=8; return v
    def cstr(s):
        e=s.b.index(b'\0',s.p); v=s.b[s.p:e].decode('utf8','replace'); s.p=e+1; return v
    def align(s,n=4): s.p=(s.p+n-1)//n*n
COMMON = None
def parse(path):
    b = open(path,'rb').read()
    r = R(b, 0, '>')
    meta_size=r.u32(); fsize=r.u32(); ver=r.u32(); data_off=r.u32()
    r.p = 8; ver = r.u32()
    r.p = 20; meta_size = r.u32(); fsize = r.i64(); data_off = r.i64(); 
    r.p = 48
    r.e = '<'
    uv = r.cstr(); plat = r.i32(); has_tt = r.u8(); ntypes = r.i32()
    print('unity', uv, 'platform', plat, 'tt', has_tt, 'types', ntypes, 'fsize', fsize, 'dataoff', data_off)
    types = []
    for _ in range(ntypes):
        cid = r.i32(); strip = r.u8(); si = r.i16()
        if cid == 114 or cid < 0: r.p += 16
        r.p += 16
        nn = r.u32(); sl = r.u32()
        nodes_raw = []
        for _ in range(nn):
            ver_, lvl, arr = r.u16(), r.u8(), r.u8()
            to, no = r.u32(), r.u32(); sz = r.i32(); idx = r.i32(); meta = r.u32(); r.p += 8
            nodes_raw.append((lvl, arr, to, no, sz, meta))
        sbuf = b[r.p:r.p+sl]; r.p += sl
        def st(o):
            if o & 0x80000000: return 'common%d' % (o & 0x7fffffff)
            e = sbuf.index(b'\0', o); return sbuf[o:e].decode()
        nodes = [(l, a, st(to), st(no), sz, m) for l, a, to, no, sz, m in nodes_raw]
        nd = r.i32(); r.p += 4*nd
        types.append((cid, si, nodes))
    nobj = r.i32()
    objs = []
    for _ in range(nobj):
        r.align(4)
        pid = r.i64(); bs = r.i64(); bsz = r.u32(); ti = r.i32()
        objs.append((pid, data_off + bs, bsz, ti))
    return b, types, objs
if __name__ == '__main__':
    b, types, objs = parse(sys.argv[1])
    from collections import Counter
    c = Counter(o[3] for o in objs)
    print('objects', len(objs))
    for i, (cid, si, nodes) in enumerate(types):
        print('type', i, 'class', cid, 'script', si, 'count', c[i], 'nodes', len(nodes))
        for n in nodes[:40]: print('   ', '  '*n[0], n[2], n[3], n[4], n[1])
