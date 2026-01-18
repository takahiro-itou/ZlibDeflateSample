
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
    showPos(st);

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
