"""Compare the native codec with Python's independent Base64 implementation."""
from pathlib import Path
import base64, ctypes, random, subprocess, tempfile
root=Path(__file__).resolve().parent
with tempfile.TemporaryDirectory() as tmp:
    source=Path(tmp)/'codec.c';binary=Path(tmp)/'codec.so'
    source.write_text('#include "base64_codec.h"\nsize_t enc(const unsigned char*p,size_t n,char*q){return b64_encode(p,n,q);}\nint dec(const char*p,size_t n,unsigned char*q,size_t*s){return b64_decode(p,n,q,s);}\n')
    subprocess.run(['gcc','-Wall','-Wextra','-Werror','-shared','-fPIC','-O2','-I',str(root),str(source),'-o',str(binary)],check=True)
    lib=ctypes.CDLL(str(binary));lib.enc.argtypes=[ctypes.c_char_p,ctypes.c_size_t,ctypes.c_void_p];lib.enc.restype=ctypes.c_size_t
    lib.dec.argtypes=[ctypes.c_char_p,ctypes.c_size_t,ctypes.c_void_p,ctypes.POINTER(ctypes.c_size_t)]
    rng=random.Random(42);count=0
    for n in list(range(1,1025))+[16000,65536,1000000]:
        data=rng.randbytes(n);expected=base64.b64encode(data);out=ctypes.create_string_buffer(len(expected)+1)
        assert lib.enc(data,n,out)==len(expected) and out.value==expected
        for encoded in [expected,expected.rstrip(b'='),b' \r\n'+b'\n'.join(expected[i:i+71] for i in range(0,len(expected),71))+b'\t']:
            decoded=ctypes.create_string_buffer(len(encoded)+3);size=ctypes.c_size_t()
            assert lib.dec(encoded,len(encoded),decoded,ctypes.byref(size)) and decoded.raw[:size.value]==data
            count+=1
    for bad in [b'',b'A',b'====',b'A===',b'Zm=8',b'Zg=',b'Zg===',b'Zh==',b'Zm9=',b'Zg==A',b'Zg!!',b'Zg--',b'\xff']:
        out=ctypes.create_string_buffer(32);size=ctypes.c_size_t()
        assert not lib.dec(bad,len(bad),out,ctypes.byref(size)),bad
    print(f'Native Base64: {count} independent round-trip comparisons and 13 invalid-input checks passed.')
