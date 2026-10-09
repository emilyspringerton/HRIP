import struct, sys
def lz4_block(src, usize):
    out = bytearray(); i = 0; n = len(src)
    while i < n:
        t = src[i]; i += 1
        l = t >> 4
        if l == 15:
            while True:
                b = src[i]; i += 1; l += b
                if b != 255: break
        out += src[i:i+l]; i += l
        if i >= n: break
        off = src[i] | src[i+1] << 8; i += 2
        m = t & 15
        if m == 15:
            while True:
                b = src[i]; i += 1; m += b
                if b != 255: break
        m += 4
        s = len(out) - off
        if off >= m: out += out[s:s+m]
        else:
            for k in range(m): out.append(out[s+k])
    assert len(out) == usize, (len(out), usize)
    return bytes(out)
def dec(data, codec, usize):
    if codec == 0: return data
    if codec in (2, 3): return lz4_block(data, usize)
    raise Exception("codec %d" % codec)
def read_unityfs(path):
    d = open(path, 'rb').read()
    p = d.index(b'\0') + 1
    ver, = struct.unpack('>I', d[p:p+4]); p += 4
    p = d.index(b'\0', p) + 1; p = d.index(b'\0', p) + 1
    size, cs, us, flags = struct.unpack('>qIII', d[p:p+20]); p += 20
    print('ver', ver, 'size', size, 'blk', cs, us, 'flags', hex(flags))
    if ver >= 7: p = (p + 15) & ~15
    if flags & 0x80: bi = d[-cs:]
    else: bi = d[p:p+cs]; p += cs
    if flags & 0x200: p = (p + 15) & ~15
    bi = dec(bi, flags & 63, us)
    q = 16
    nb, = struct.unpack('>I', bi[q:q+4]); q += 4
    blocks = []
    for _ in range(nb):
        u, c, f = struct.unpack('>IIH', bi[q:q+10]); q += 10; blocks.append((u, c, f))
    nn, = struct.unpack('>I', bi[q:q+4]); q += 4
    nodes = []
    for _ in range(nn):
        o, s, f = struct.unpack('>qqI', bi[q:q+20]); q += 20
        e = bi.index(b'\0', q); nodes.append((o, s, f, bi[q:e].decode())); q = e + 1
    out = bytearray()
    for u, c, f in blocks:
        out += dec(d[p:p+c], f & 63, u); p += c
    print('blocks', len(blocks), 'total', len(out), 'nodes', nodes)
    return bytes(out), nodes
if __name__ == '__main__':
    data, nodes = read_unityfs(sys.argv[1])
    for o, s, f, n in nodes:
        open(sys.argv[2] + '/' + n.replace('/', '_'), 'wb').write(data[o:o+s])
