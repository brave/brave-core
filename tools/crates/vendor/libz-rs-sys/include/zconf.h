/* zconf.h -- configuration of the zlib compression library
 * Copyright (C) 1995-2024 Jean-loup Gailly, Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#ifndef ZCONF_H
#define ZCONF_H

#ifdef Z_PREFIX     /* may be set to #if 1 by ./configure */
#  define Z_PREFIX_SET

/* all linked symbols and init macros */
#  define _dist_code            z__dist_code
#  define _length_code          z__length_code
#  define _tr_align             z__tr_align
#  define _tr_flush_bits        z__tr_flush_bits
#  define _tr_flush_block       z__tr_flush_block
#  define _tr_init              z__tr_init
#  define _tr_stored_block      z__tr_stored_block
#  define _tr_tally             z__tr_tally
#  define adler32               z_adler32
#  define adler32_combine       z_adler32_combine
#  define adler32_combine64     z_adler32_combine64
#  define adler32_z             z_adler32_z
#  ifndef Z_SOLO
#    define compress              z_compress
#    define compress2             z_compress2
#    define compressBound         z_compressBound
#  endif
#  define crc32                 z_crc32
#  define crc32_combine         z_crc32_combine
#  define crc32_combine64       z_crc32_combine64
#  define crc32_combine_gen     z_crc32_combine_gen
#  define crc32_combine_gen64   z_crc32_combine_gen64
#  define crc32_combine_op      z_crc32_combine_op
#  define crc32_z               z_crc32_z
#  define deflate               z_deflate
#  define deflateBound          z_deflateBound
#  define deflateCopy           z_deflateCopy
#  define deflateEnd            z_deflateEnd
#  define deflateGetDictionary  z_deflateGetDictionary
#  define deflateInit           z_deflateInit
#  define deflateInit2          z_deflateInit2
#  define deflateInit2_         z_deflateInit2_
#  define deflateInit_          z_deflateInit_
#  define deflateParams         z_deflateParams
#  define deflatePending        z_deflatePending
#  define deflatePrime          z_deflatePrime
#  define deflateReset          z_deflateReset
#  define deflateResetKeep      z_deflateResetKeep
#  define deflateSetDictionary  z_deflateSetDictionary
#  define deflateSetHeader      z_deflateSetHeader
#  define deflateTune           z_deflateTune
#  define deflateUsed           z_deflateUsed
#  define deflate_copyright     z_deflate_copyright
#  define get_crc_table         z_get_crc_table
#  ifndef Z_SOLO
#    define gz_error              z_gz_error
#    define gz_intmax             z_gz_intmax
#    define gz_strwinerror        z_gz_strwinerror
#    define gzbuffer              z_gzbuffer
#    define gzclearerr            z_gzclearerr
#    define gzclose               z_gzclose
#    define gzclose_r             z_gzclose_r
#    define gzclose_w             z_gzclose_w
#    define gzdirect              z_gzdirect
#    define gzdopen               z_gzdopen
#    define gzeof                 z_gzeof
#    define gzerror               z_gzerror
#    define gzflush               z_gzflush
#    define gzfread               z_gzfread
#    define gzfwrite              z_gzfwrite
#    define gzgetc                z_gzgetc
#    define gzgetc_               z_gzgetc_
#    define gzgets                z_gzgets
#    define gzoffset              z_gzoffset
#    define gzoffset64            z_gzoffset64
#    define gzopen                z_gzopen
#    define gzopen64              z_gzopen64
#    ifdef _WIN32
#      define gzopen_w              z_gzopen_w
#    endif
#    define gzprintf              z_gzprintf
#    define gzputc                z_gzputc
#    define gzputs                z_gzputs
#    define gzread                z_gzread
#    define gzrewind              z_gzrewind
#    define gzseek                z_gzseek
#    define gzseek64              z_gzseek64
#    define gzsetparams           z_gzsetparams
#    define gztell                z_gztell
#    define gztell64              z_gztell64
#    define gzungetc              z_gzungetc
#    define gzvprintf             z_gzvprintf
#    define gzwrite               z_gzwrite
#  endif
#  define inflate               z_inflate
#  define inflateBack           z_inflateBack
#  define inflateBackEnd        z_inflateBackEnd
#  define inflateBackInit       z_inflateBackInit
#  define inflateBackInit_      z_inflateBackInit_
#  define inflateCodesUsed      z_inflateCodesUsed
#  define inflateCopy           z_inflateCopy
#  define inflateEnd            z_inflateEnd
#  define inflateGetDictionary  z_inflateGetDictionary
#  define inflateGetHeader      z_inflateGetHeader
#  define inflateInit           z_inflateInit
#  define inflateInit2          z_inflateInit2
#  define inflateInit2_         z_inflateInit2_
#  define inflateInit_          z_inflateInit_
#  define inflateMark           z_inflateMark
#  define inflatePrime          z_inflatePrime
#  define inflateReset          z_inflateReset
#  define inflateReset2         z_inflateReset2
#  define inflateResetKeep      z_inflateResetKeep
#  define inflateSetDictionary  z_inflateSetDictionary
#  define inflateSync           z_inflateSync
#  define inflateSyncPoint      z_inflateSyncPoint
#  define inflateUndermine      z_inflateUndermine
#  define inflateValidate       z_inflateValidate
#  define inflate_copyright     z_inflate_copyright
#  define inflate_fast          z_inflate_fast
#  define inflate_table         z_inflate_table
#  ifndef Z_SOLO
#    define uncompress            z_uncompress
#    define uncompress2           z_uncompress2
#  endif
#  define zError                z_zError
#  ifndef Z_SOLO
#    define zcalloc               z_zcalloc
#    define zcfree                z_zcfree
#  endif
#  define zlibCompileFlags      z_zlibCompileFlags
#  define zlibVersion           z_zlibVersion

/* all zlib typedefs in zlib.h and zconf.h */
#  define Byte                  z_Byte
#  define Bytef                 z_Bytef
#  define alloc_func            z_alloc_func
#  define charf                 z_charf
#  define free_func             z_free_func
#  ifndef Z_SOLO
#    define gzFile                z_gzFile
#  endif
#  define gz_header             z_gz_header
#  define gz_headerp            z_gz_headerp
#  define in_func               z_in_func
#  define intf                  z_intf
#  define out_func              z_out_func
#  define uInt                  z_uInt
#  define uIntf                 z_uIntf
#  define uLong                 z_uLong
#  define uLongf                z_uLongf
#  define voidp                 z_voidp
#  define voidpc                z_voidpc
#  define voidpf                z_voidpf

/* all zlib structs in zlib.h and zconf.h */
#  define gz_header_s           z_gz_header_s
#  define internal_state        z_internal_state

#endif

#if !defined(_WIN32) && defined(__WIN32__)
#  define _WIN32
#endif

/* Clang macro for detecting declspec support
 * https://clang.llvm.org/docs/LanguageExtensions.html#has-declspec-attribute
 */
#ifndef __has_declspec_attribute
#  define __has_declspec_attribute(x) 0
#endif

#if defined(ZLIB_CONST) && !defined(z_const)
#  define z_const const
#else
#  define z_const
#endif

/* Maximum value for memLevel in deflateInit2 */
#ifndef MAX_MEM_LEVEL
#  define MAX_MEM_LEVEL 9
#endif

/* Maximum value for windowBits in deflateInit2 and inflateInit2.
 * WARNING: reducing MAX_WBITS makes minigzip unable to extract .gz files
 * created by gzip. (Files created by minigzip can still be extracted by
 * gzip.)
 */
#ifndef MIN_WBITS
#  define MIN_WBITS   8  /* 256 LZ77 window */
#endif
#ifndef MAX_WBITS
#  define MAX_WBITS   15 /* 32K LZ77 window */
#endif

/* The memory requirements for deflate are (in bytes):
            (1 << (windowBits+2)) +  (1 << (memLevel+9))
 that is: 128K for windowBits=15  +  128K for memLevel = 8  (default values)
 plus a few kilobytes for small objects. For example, if you want to reduce
 the default memory requirements from 256K to 128K, compile with
     make CFLAGS="-O -DMAX_WBITS=14 -DMAX_MEM_LEVEL=7"
 Of course this will generally degrade compression (there's no free lunch).

   The memory requirements for inflate are (in bytes) 1 << windowBits
 that is, 32K for windowBits=15 (default value) plus about 7 kilobytes
 for small objects.
*/

/* Type declarations */


#ifndef OF /* function prototypes */
#  define OF(args)  args
#endif

#ifdef ZLIB_INTERNAL
#  define Z_INTERNAL ZLIB_INTERNAL
#endif

/* If building or using zlib as a DLL, define ZLIB_DLL.
 * This is not mandatory, but it offers a little performance increase.
 */
#if defined(ZLIB_DLL) && (defined(_WIN32) || (__has_declspec_attribute(dllexport) && __has_declspec_attribute(dllimport)))
#  ifdef Z_INTERNAL
#    define Z_EXTERN extern __declspec(dllexport)
#  else
#    define Z_EXTERN extern __declspec(dllimport)
#  endif
#endif

/* If building or using zlib with the WINAPI/WINAPIV calling convention,
 * define ZLIB_WINAPI.
 * Caution: the standard ZLIB1.DLL is NOT compiled using ZLIB_WINAPI.
 */
#if defined(ZLIB_WINAPI) && defined(_WIN32)
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
   /* No need for _export, use ZLIB.DEF instead. */
   /* For complete Windows compatibility, use WINAPI, not __stdcall. */
#  define Z_EXPORT WINAPI
#  define Z_EXPORTVA WINAPIV
#endif

#ifndef Z_EXTERN
#  define Z_EXTERN extern
#endif
#ifndef Z_EXPORT
#  define Z_EXPORT
#endif
#ifndef Z_EXPORTVA
#  define Z_EXPORTVA
#endif

/* Conditional exports */
#define ZNG_CONDEXPORT Z_INTERNAL

/* For backwards compatibility */

#ifndef ZEXTERN
#  define ZEXTERN Z_EXTERN
#endif
#ifndef ZEXPORT
#  define ZEXPORT Z_EXPORT
#endif
#ifndef ZEXPORTVA
#  define ZEXPORTVA Z_EXPORTVA
#endif
#ifndef FAR
#  define FAR
#endif

/* Legacy zlib typedefs for backwards compatibility. Don't assume stdint.h is defined. */
typedef unsigned char Byte;
typedef Byte Bytef;

typedef unsigned int   uInt;  /* 16 bits or more */
typedef unsigned long  uLong; /* 32 bits or more */

typedef char  charf;
typedef int   intf;
typedef uInt  uIntf;
typedef uLong uLongf;

typedef void const *voidpc;
typedef void       *voidpf;
typedef void       *voidp;

typedef unsigned int z_crc_t;

#if !defined(_WIN32)
#  define Z_HAVE_UNISTD_H
#endif

#include <sys/types.h>      /* for off_t */

#include <stddef.h>         /* for wchar_t and NULL */

/* a little trick to accommodate both "#define _LARGEFILE64_SOURCE" and
 * "#define _LARGEFILE64_SOURCE 1" as requesting 64-bit operations, (even
 * though the former does not conform to the LFS document), but considering
 * both "#undef _LARGEFILE64_SOURCE" and "#define _LARGEFILE64_SOURCE 0" as
 * equivalently requesting no 64-bit operations
 */
#if defined(_LARGEFILE64_SOURCE) && -_LARGEFILE64_SOURCE - -1 == 1
#  undef _LARGEFILE64_SOURCE
#endif

#if defined(Z_HAVE_UNISTD_H) || defined(_LARGEFILE64_SOURCE)
#  include <unistd.h>         /* for SEEK_*, off_t, and _LFS64_LARGEFILE */
#  ifndef z_off_t
#    define z_off_t off_t
#  endif
#endif

#if defined(_LFS64_LARGEFILE) && _LFS64_LARGEFILE-0
#  define Z_LFS64
#endif

#if defined(_LARGEFILE64_SOURCE) && defined(Z_LFS64)
#  define Z_LARGE64
#endif

#if defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS-0 == 64 && defined(Z_LFS64)
#  define Z_WANT64
#endif

#if !defined(SEEK_SET)
#  define SEEK_SET        0       /* Seek from beginning of file.  */
#  define SEEK_CUR        1       /* Seek from current position.  */
#  define SEEK_END        2       /* Set file pointer to EOF plus "offset" */
#endif

#ifndef z_off_t
#  define z_off_t long
#endif

#if !defined(_WIN32) && defined(Z_LARGE64)
#  define z_off64_t off64_t
#else
#  if defined(__MSYS__)
#    define z_off64_t _off64_t
#  elif defined(_WIN32) && !defined(__GNUC__)
#    define z_off64_t __int64
#  else
#    define z_off64_t z_off_t
#  endif
#endif

typedef size_t z_size_t;

#endif /* ZCONF_H */
