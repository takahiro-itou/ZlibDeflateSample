
#if 0
g++ -o testzlib.exe zlib.cpp
exit 0
#endif

#include    <stddef.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <stdint.h>
#include    <vector>

struct  status
{
    const   uint8_t * buf;
    size_t      len;
    size_t      pos;
    int         bit;
};

struct huffman
{
    int len;
    int code;
};

template <typename T>
T   pointer_cast(void * ptr)
{
    return ( static_cast<T>(ptr) );
}

template <typename T>
T   pointer_cast(const void * ptr)
{
    return ( static_cast<T>(ptr) );
}

void
showPos(const status &st)
{
    fprintf(stderr,
            "# INFO : pos = %ld, bit = %d, len = %ld\n",
            st.pos, st.bit, st.len);
    return;
}

template <typename T, size_t N>
void
showArray(T (& ptr)[N], FILE * fp = stderr)
{
    for ( size_t i = 0; i < N; ++ i ) {
        fprintf(fp, "%d, ", ptr[i]);
    }
    fprintf(fp, "\n");
}

void
showBits(const int code, const int len, FILE * fp = stderr)
{
    for ( int j = 0; j < len; ++ j ) {
        int k = len - j - 1;
        fprintf(fp, "%d", (code >> k) & 1);
    }
}

void
showHuffman(const huffman huff[], const int N, FILE * fp = stderr)
{
    for ( int i = 0; i < N; ++ i ) {
        if ( huff[i].len == 0 ) { continue; }
        fprintf(fp, "Huff[%d] (%d) : ", i, huff[i].len);
        showBits(huff[i].code, huff[i].len, fp);
        fprintf(fp, "\n");
    }
}


const uint32_t
readBits(status & st, int bits)
{
    uint32_t    val = 0;
    const uint32_t * ptr = pointer_cast<const uint32_t *>(st.buf + st.pos);
    val = *(ptr) >> (st.bit);
    st.bit  += bits;
    while ( st.bit >= 8 ) {
        st.bit  -= 8;
        st.pos  ++;
    }
    const  uint32_t msk = (1UL << bits) - 1;

#if defined( _DEBUG )
    fprintf(stderr, "# DBG: pos=%lx, *ptr=%x, val=%x, ret=%x\n",
            st.pos, (*ptr), val, val & msk);
#endif

    return ( val & msk );
}

const   size_t
checkHeader(uint8_t * &buf)
{
    const uint32_t * ptr = pointer_cast<const uint32_t *>(buf);
    if ( ptr[0] != 0x354E5452 ) {
        return ( 0 );
    }
    buf += 0x18;
    return ( ptr[2] );  //  展開後のサイズ。
}

void
canonical(const int len[], const int num, huffman huff[])
{
    std::vector<int>    work(num);
    std::vector<int>    copy(num);


    huffman h = { 0, 0 };
    for ( int i = 0; i < num; ++ i ) {
        huff[i] = h;
        copy[i] = len[i];
    }

    for ( int i = 0; i < num; ++ i ) {
        int sel = -1;
        int min = 9999;
        for ( int j = 0; j < num; ++ j ) {
            if ( copy[j] == 0 ) { continue; }
            if ( copy[j] < min ) {
                sel = j;
                min = copy[j];
            }
        }
        work[i] = sel;
        if ( sel == -1 ) {
            break;
        }
        copy[sel] = 0;
    }

    for ( int i = 0; i < num; ++ i ) {
        int sel = work[i];
        if ( sel == -1 ) {
            //  残りは符号長が 0 、つまり出現しない
            break;
        }
        int trglen = len[sel];
        while ( h.len < trglen ) {
            ++ h.len;
            h.code  <<= 1;
        }
        huff[sel] = h;
#if 1
        fprintf(stderr, "# DBG : sel=%d, huff=%d, %x, ",
                sel, h.len, h.code);
        showBits(h.code, h.len);
        fprintf(stderr, "\n");
#endif
        ++ h.code;
    }
}

void
convertHuffman(
        const int huff[],
        const int num,
        int trees[])
{
}

uint32_t
readHuffman(
        const  huffman huff[],
        const  int  num,
        status      &st,
        int *       pbits = nullptr)
{
    int bit = 0;
    int max = -1;
    int cod = 0;
    char    dbg[32] = { 0 };

    while ( max == -1 || bit <= max ) {
        int n = readBits(st, 1);
        dbg[bit] = '0' + n;
        cod = (cod << 1) | n;
        ++ bit;
        for ( int i = 0; i < num; ++ i ) {
            int len = huff[i].len;
            if ( len > max ) { max = len; }
            if ( bit != len ) { continue; }
            if ( huff[i].code == cod ) {
                //  見つかった。
                if ( pbits != nullptr ) {
                    *pbits = bit;
                }
                return ( i );
            }
        }
    }
    fprintf(stderr, "Invalid Code at %lx code = %s\n",
            st.pos, dbg
    );
    exit( 3 );
}

void inflate(
        const uint8_t * buf, const size_t fsz,
        const size_t osz,
        std::vector<uint8_t> &wrt)
{
    status  st = { buf, fsz, 0, 0 };
    wrt.clear();
    wrt.resize(osz, 0);

    //  ヘッダー    //
    uint32_t  head  = readBits(st, 16);
    if ( head != 0x9C78 ) {
        fprintf(stderr, "Unsupported Format: %04x\n", head);
        exit( 2 );
    }
    fprintf(stderr, "Header OK.\n");

    int  bfinal = readBits(st, 1);
    int  btype  = readBits(st, 2);
    fprintf(stderr, "bfinal = %d, btype = %d\n", bfinal, btype);

    //  データの個数
    const int hlit  = readBits(st, 5) + 257;
    const int hdist = readBits(st, 5) +   1;
    const int hclen = readBits(st, 4) +   4;
    fprintf(stderr, "hlit = %d, hdist = %d, hclen = %d\n",
            hlit, hdist, hclen);

    //  符号長の符号長
    const int len_pos[19] = {
        16, 17, 18, 0,  8, 7,  9, 6, 10, 5,
        11,  4, 12, 3, 13, 2, 14, 1, 15
    };
    int lenlen[19] = { 0 };
    for ( int i = 0; i < hclen; ++ i ) {
        lenlen[len_pos[i]] = readBits(st, 3);
    }
    showArray(lenlen);

    huffman len_huf[19];
    canonical(lenlen, 19, len_huf);
    showHuffman(len_huf, 19);

    //  文字・一致長の符号長表を復元
    int lit_len[285] = { 0 };
    for ( int i = 0; i < hlit; ) {
        int code = readHuffman(len_huf, 19, st);
        int len = 0;
        if ( code == 16 ) {
            //  直前の数値を繰り返す。  //
            len = readBits(st, 2) + 3;
            for ( int j = 0; j < len; ++ j ) {
                lit_len[i] = lit_len[i - 1];
                ++ i;
            }
        } else if ( code == 17 ) {
            //  ゼロが 3..10    //
            len = readBits(st, 3) + 3;
            for ( int j = 0; j < len; ++ j ) {
                lit_len[i++]  = 0;
            }
        } else if ( code == 18 ) {
            //  ゼロが 11 .. 138 //
            len = readBits(st, 7) + 11;
            for ( int j = 0; j < len; ++ j ) {
                lit_len[i++]  = 0;
            }
        } else {
            //  そのまま
            lit_len[i++] = code;
        }
    }
    showArray(lit_len);

    //  文字・一致長の符号
    huffman lit_huf[285];
    canonical(lit_len, 285, lit_huf);
    showHuffman(lit_huf, 285);

    //  距離の符号長表を復元
    int dist_len[29] = { 0 };
    for ( int i = 0; i < hdist; ) {
        int code = readHuffman(len_huf, 19, st);
        int len = 0;
        if ( code == 16 ) {
            //  直前の数値を繰り返す。  //
            len = readBits(st, 2) + 3;
            for ( int j = 0; j < len; ++ j ) {
                dist_len[i] = dist_len[i - 1];
                ++ i;
            }
        } else if ( code == 17 ) {
            //  ゼロが 3..10    //
            len = readBits(st, 3) + 3;
            for ( int j = 0; j < len; ++ j ) {
                dist_len[i++]  = 0;
            }
        } else if ( code == 18 ) {
            //  ゼロが 11 .. 138 //
            len = readBits(st, 7) + 11;
            for ( int j = 0; j < len; ++ j ) {
                dist_len[i++]  = 0;
            }
        } else {
            //  そのまま
            dist_len[i++] = code;
        }
    }
    fprintf(stderr, "dist_len = ");
    showArray(dist_len);

    //  距離の符号
    huffman dst_huf[285];
    canonical(dist_len, 29, dst_huf);
    showHuffman(dst_huf, 29);

    //  データ本体を復元
    int len_base[286 - 256] = {
        -1,  3,   4,  5,  6,  7,  8,  9, 10,  11,  13,  15,  17,  19,  23,
        27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258,
    };
    int len_ext[286 - 256] = {
        -1,  0,  0,  0,  0,  0,  0,  0,  0,   1,   1,   1,   1,   2,   2,
        2,   2,  3,  3,  3,  3,  4,  4,  4,   4,   5,   5,   5,   5,   0,
    };
    int dst_base[30] = {
        1,       2,    3,    4,    5,    7,    9,    13,    17,    25,
        33,     49,   65,   97,  129,  193,  257,   385,   513,   769,
        1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577,
    };

    int pos = 0;
    int bits = 0;
    while ( pos < osz ) {
        //  ハフマン符号を読み出す。
        fprintf(stderr, "# DBG : pos = %08lx, %d : ", st.pos, st.bit);
        int code = readHuffman(lit_huf, 285, st, &bits);
        fprintf(stderr, "%x ", code);
        showBits(code, bits);
        fprintf(stderr, "\n");

        if ( code == 256 ) {
            break;      //  終端
        }
        if ( code >= 286 ) {
            fprintf(stderr, "Invalid Code : %d\n", code);
            exit( 4 );
        }
        if ( code < 256 ) {
            //  そのまま
            wrt[pos++]  = code;
            continue;
        }

    }

    return;
}

int main(int argc, char * argv[])
{
    FILE *  fp;

    if ( argc > 1 ) {
        fp  = fopen(argv[1], "rb");
    } else {
        fp  = fopen("Test6.z", "rb");
    }
    if ( fp == NULL ) {
        perror("file open error");
        exit( 1 );
    }

    fseek(fp, 0, SEEK_END);
    long    fsz = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    fprintf(stderr, "File Size = %ld\n", fsz);

    std::vector<uint8_t>    buf(fsz);
    uint8_t *   ptr = &(buf[0]);
    size_t  rsz = fread(ptr, 1, fsz, fp);
    fclose(fp);
    if ( rsz != fsz ) {
        perror("file read error");
    }
    fprintf(stderr, "Read File OK\n");

    size_t  orgSize =checkHeader(ptr);
    fprintf(stderr, "Original Size = %lx\n", orgSize);

    std::vector<uint8_t>    wrt;
    inflate(ptr, fsz - 0x18, orgSize, wrt);

    return ( 0 );
}
