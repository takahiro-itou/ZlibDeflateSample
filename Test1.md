
# Test Data 1

- データ準備

```
import zlib

text = b'\x00' * 32
enc = zlib.compress(text)

with open('Test1.z', 'wb') as fw:
    fw.write(enc)
```

- 元データ (32 bytes)

```
00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00
```

- 圧縮データ (11 bytes)

```
78 9C 63 60 C0 0F 00 00 20 00 01
```

