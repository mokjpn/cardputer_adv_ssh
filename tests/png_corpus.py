"""Deterministic PNGs plus independent RGB565 hashes; no Pillow dependency."""
import pathlib, random, struct, sys, zlib

out = pathlib.Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
rng = random.Random(20261017)
period = bytes(rng.randrange(256) for _ in range(30000))
manifest = []
def chunk(kind, data):
    return struct.pack('>I', len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data))
def paeth(a,b,c):
    p=a+b-c; x=abs(p-a); y=abs(p-b); z=abs(p-c)
    return a if x<=y and x<=z else b if y<=z else c
def emit(name, kind=2, filter_mode=0, level=6, wbits=15, split=2048, strategy=zlib.Z_DEFAULT_STRATEGY):
    channels={0:1,2:3,3:1,4:2,6:4}[kind]; rows=[]; rgb=[]
    palette=bytes((i*73+19)%256 for i in range(768))
    for y in range(240):
        row=bytearray(); colors=[]
        for x in range(320):
            pos=(y*961+x*3)%len(period)
            r,g,b=(period[(pos+i)%len(period)] for i in range(3))
            if kind==2: row.extend((r,g,b))
            elif kind==6:
                # Keep this full-width RGBA case compressible and opaque.
                r,g,b=(x*17%256, y*7%256, (x+y)*3%256); row.extend((r,g,b,255))
            elif kind==0: g=b=r; row.append(r)
            elif kind==4: g=b=r; row.extend((r,255))
            else:
                index=(x+y*13)%256; row.append(index); r,g,b=palette[index*3:index*3+3]
            colors.append((r,g,b))
        rows.append(bytes(row)); rgb.append(colors)
    raw=bytearray(); previous=bytes(len(rows[0]))
    for y,row in enumerate(rows):
        f=y%5 if filter_mode==-1 else filter_mode; raw.append(f)
        for i,value in enumerate(row):
            a=row[i-channels] if i>=channels else 0; b=previous[i]; c=previous[i-channels] if i>=channels else 0
            pred=[0,a,b,(a+b)//2,paeth(a,b,c)][f]; raw.append((value-pred)&255)
        previous=row
    compressor=zlib.compressobj(level,zlib.DEFLATED,wbits,8,strategy)
    compressed=compressor.compress(raw)+compressor.flush()
    if name=='rgb-filter-0.png':
        (out/'inflate-reference.raw').write_bytes(raw)
        (out/'inflate-reference.z').write_bytes(compressed)
        bad_streams=[b'bad zlib stream',compressed[:len(compressed)//2]]
        for index in [0,1,7,31,129,513,2049,len(compressed)//3,len(compressed)-2]:
            changed=bytearray(compressed); changed[index]^=0x40
            try: zlib.decompress(changed)
            except zlib.error: bad_streams.append(bytes(changed))
        bad_names=[]
        for index,bad in enumerate(bad_streams):
            bad_name=f'bad-deflate-{index}.png'
            data=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',320,240,8,2,0,0,0))+chunk(b'IDAT',bad)+chunk(b'IEND',b'')
            (out/bad_name).write_bytes(data); bad_names.append(bad_name+'\n')
        (out/'negative.txt').write_text(''.join(bad_names))
    png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',320,240,8,kind,0,0,0))
    if kind==3: png+=chunk(b'PLTE',palette)
    for i in range(0,len(compressed),split): png+=chunk(b'IDAT',compressed[i:i+split])
    png+=chunk(b'IEND',b''); assert len(png)<262144,(name,len(png))
    (out/name).write_bytes(png)
    h=2166136261
    for row in rgb:
        for r,g,b in row:
            pixel=((r>>3)<<11)|((g>>2)<<5)|(b>>3)
            for byte in (pixel&255,pixel>>8): h=((h^byte)*16777619)&0xffffffff
    manifest.append(f'{name}\t{h:08x}\n')
for f in range(5): emit(f'rgb-filter-{f}.png',filter_mode=f)
emit('rgb-long-distance-split.png',split=257)
emit('rgb-stored.png',level=0)
emit('rgb-fixed.png',strategy=zlib.Z_FIXED)
emit('rgb-small-window.png',wbits=9)
emit('rgba-full-width.png',kind=6,filter_mode=-1)
emit('gray.png',kind=0,filter_mode=-1)
emit('gray-alpha.png',kind=4,filter_mode=-1)
emit('palette.png',kind=3,filter_mode=-1)
(out/'manifest.tsv').write_text(''.join(manifest))
print(f'Generated {len(manifest)} full-size PNG cases; RGB source period=30000 bytes')
