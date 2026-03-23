#pragma once

typedef int (*ZlibUncompressFunc)(unsigned char* dest, unsigned long* destLen,
                                  const unsigned char* source, unsigned long sourceLen);

typedef int (*ZlibCompressFunc)(unsigned char* dest, unsigned long* destLen,
                                const unsigned char* source, unsigned long sourceLen);

typedef int (*ZlibCompress2Func)(unsigned char* dest, unsigned long* destLen,
                                 const unsigned char* source, unsigned long sourceLen, int level);

typedef int (*ZlibDeflateInitFunc)(void* strm, int level);
typedef int (*ZlibDeflateFunc)(void* strm, int flush);
typedef int (*ZlibDeflateEndFunc)(void* strm);
typedef int (*ZlibInflateInitFunc)(void* strm);
typedef int (*ZlibInflateFunc)(void* strm, int flush);
typedef int (*ZlibInflateEndFunc)(void* strm);

ZlibUncompressFunc GetZlibUncompress();
ZlibCompressFunc GetZlibCompress();
ZlibCompress2Func GetZlibCompress2();
ZlibInflateInitFunc GetZlibInflateInit();
ZlibInflateFunc GetZlibInflate();
ZlibInflateEndFunc GetZlibInflateEnd();
ZlibDeflateInitFunc GetZlibDeflateInit();
ZlibDeflateFunc GetZlibDeflate();
ZlibDeflateEndFunc GetZlibDeflateEnd();
