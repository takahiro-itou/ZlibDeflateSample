
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
showArray(T (& ptr)[N]) {
    for ( size_t i = 0; i < N; ++ i ) {
        fprintf(stderr, "%d, ", ptr[i]);
    }
}

const uint32_t
readBits(status & st, int bits)
{
    uint32_t    val = 0;
    const uint32_t * ptr = pointer_cast<const uint32_t *>(st.buf + st.pos);
    val = *(ptr) >> (st.bit);
    fprintf(stderr, "# DBG: pos=%ld, *ptr=%x, val=%x\n",
            st.pos, (*ptr), val);
    st.bit  += bits;
    while ( st.bit >= 8 ) {
        st.bit  -= 8;
        st.pos  ++;
    }
    const  uint32_t msk = (1UL << bits) - 1;
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
        if ( sel == -1 ) {
            break;
        }
        work[i] = sel;
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

void inflate(const uint8_t * buf, const size_t fsz, const size_t osz)
{
    status  st = { buf, fsz, 0, 0 };
    std::vector<uint8_t>    wrt;
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

    return;
}

int main(int argc, char * argv[])
{
    FILE *  fp;

    if ( argc > 1 ) {
        fp  = fopen(argv[0], "rb");
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

    inflate(ptr, fsz - 0x18, orgSize);

    return ( 0 );
}
