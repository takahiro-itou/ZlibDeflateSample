
#if 0
g++ -o testzlib.exe zlib.cpp
exit 0
#endif

#include    <stddef.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <stdint.h>
#include    <vector>

template <typename T>
T   pointer_cast(void * ptr)
{
    return ( static_cast<T>(ptr) );
}

size_t
checkHeader(uint8_t * &buf)
{
    const uint32_t * ptr = pointer_cast<const uint32_t *>(buf);
    if ( ptr[0] != 0x354E5452 ) {
        return ( 0 );
    }
    buf += 0x18;
    return ( ptr[2] );  //  展開後のサイズ。
}

void inflate(const uint8_t * buf, const size_t sz)
{
    std::vector<uint8_t>    wrt;
    wrt.clear();
    wrt.resize(sz, 0);

    //  ヘッダー    //
    if ( buf[0] != 0x78 || buf[1] != 0x9C ) {
        fprintf(stderr, "Unsupported Format.");
        exit( 2 );
    }
    fprintf(stderr, "Header OK.\n");

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

    inflate(ptr, orgSize);

    return ( 0 );
}
