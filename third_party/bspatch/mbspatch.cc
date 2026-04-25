/*-
 * Copyright 2003,2004 Colin Percival
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Changelog:
 * 2005-04-26 - Define the header as a C structure, add a CRC32 checksum to
 *              the header, and make all the types 32-bit.
 *                --Benjamin Smedberg <benjamin@smedbergs.us>
 * 2007-11-14 - Added CalculateCrc() and ApplyBinaryPatch() methods.
 *                --Rahul Kuchhal
 * 2016-07-27 - Improve validation of diffs.
 *                --Ricky Zhou
 */

#include "mbspatch.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>

#ifdef _WIN32
# include <io.h>
# include <winsock2.h>
#else
# include <unistd.h>
# include <arpa/inet.h>
#endif

extern "C" {
#include <7zCrc.h>
}

#ifndef SSIZE_MAX
# define SSIZE_MAX LONG_MAX
#endif

int
MBS_ReadHeader(int fd, MBSPatchHeader *header)
{
  int s = read(fd, header, sizeof(MBSPatchHeader));
  if (s != sizeof(MBSPatchHeader))
    return READ_ERROR;

  header->slen      = ntohl(header->slen);
  header->scrc32    = ntohl(header->scrc32);
  header->dlen      = ntohl(header->dlen);
  header->cblen     = ntohl(header->cblen);
  header->difflen   = ntohl(header->difflen);
  header->extralen  = ntohl(header->extralen);

  struct stat hs;
  s = fstat(fd, &hs);
  if (s != 0)
    return READ_ERROR;

  if (memcmp(header->tag, "MBDIFF10", 8) != 0)
    return UNEXPECTED_ERROR;

  if (hs.st_size > INT_MAX)
    return UNEXPECTED_ERROR;

  size_t size = static_cast<size_t>(hs.st_size);
  if (size < sizeof(MBSPatchHeader))
    return UNEXPECTED_ERROR;
  size -= sizeof(MBSPatchHeader);

  if (size < header->cblen)
    return UNEXPECTED_ERROR;
  size -= header->cblen;

  if (size < header->difflen)
    return UNEXPECTED_ERROR;
  size -= header->difflen;

  if (size < header->extralen)
    return UNEXPECTED_ERROR;
  size -= header->extralen;

  if (size != 0)
    return UNEXPECTED_ERROR;

  return OK;
}

int
MBS_ApplyPatch(const MBSPatchHeader *header, int patchfd,
               unsigned char *fbuffer, int filefd)
{
  unsigned char *fbufstart = fbuffer;
  unsigned char *fbufend = fbuffer + header->slen;

  unsigned char *buf = (unsigned char*) malloc(header->cblen +
                                               header->difflen +
                                               header->extralen);
  if (!buf)
    return MEM_ERROR;

  int rv = OK;

  int r = header->cblen + header->difflen + header->extralen;
  unsigned char *wb = buf;
  while (r) {
    int c = read(patchfd, wb, (r > SSIZE_MAX) ? SSIZE_MAX : r);
    if (c < 0) {
      rv = READ_ERROR;
      goto end;
    }

    r -= c;
    wb += c;

    if (c == 0 && r) {
      rv = UNEXPECTED_ERROR;
      goto end;
    }
  }

  {
    MBSPatchTriple *ctrlsrc = (MBSPatchTriple*) buf;
    if (header->cblen % sizeof(MBSPatchTriple) != 0) {
      rv = UNEXPECTED_ERROR;
      goto end;
    }

    unsigned char *diffsrc = buf + header->cblen;
    unsigned char *extrasrc = diffsrc + header->difflen;

    MBSPatchTriple *ctrlend = (MBSPatchTriple*) diffsrc;
    unsigned char *diffend = extrasrc;
    unsigned char *extraend = extrasrc + header->extralen;

    while (ctrlsrc < ctrlend) {
      ctrlsrc->x = ntohl(ctrlsrc->x);
      ctrlsrc->y = ntohl(ctrlsrc->y);
      ctrlsrc->z = ntohl(ctrlsrc->z);

#ifdef DEBUG_bsmedberg
      printf("Applying block:\n"
             " x: %u\n"
             " y: %u\n"
             " z: %i\n",
             ctrlsrc->x,
             ctrlsrc->y,
             ctrlsrc->z);
#endif

      /* Add x bytes from oldfile to x bytes from the diff block */

      if (ctrlsrc->x > static_cast<size_t>(fbufend - fbuffer) ||
          ctrlsrc->x > static_cast<size_t>(diffend - diffsrc)) {
        rv = UNEXPECTED_ERROR;
        goto end;
      }
      for (unsigned int i = 0; i < ctrlsrc->x; ++i) {
        diffsrc[i] += fbuffer[i];
      }
      if ((int) write(filefd, diffsrc, ctrlsrc->x) != ctrlsrc->x) {
        rv = WRITE_ERROR;
        goto end;
      }
      fbuffer += ctrlsrc->x;
      diffsrc += ctrlsrc->x;

      /* Copy y bytes from the extra block */

      if (ctrlsrc->y > static_cast<size_t>(extraend - extrasrc)) {
        rv = UNEXPECTED_ERROR;
        goto end;
      }
      if ((int) write(filefd, extrasrc, ctrlsrc->y) != ctrlsrc->y) {
        rv = WRITE_ERROR;
        goto end;
      }
      extrasrc += ctrlsrc->y;

      /* "seek" forwards in oldfile by z bytes */

      if (ctrlsrc->z < fbufstart - fbuffer ||
          ctrlsrc->z > fbufend - fbuffer) {
        rv = UNEXPECTED_ERROR;
        goto end;
      }
      fbuffer += ctrlsrc->z;

      /* and on to the next control block */

      ++ctrlsrc;
    }
  }

end:
  free(buf);
  return rv;
}

int CalculateCrc(const unsigned char *buf, int size) {
  CrcGenerateTable();
  unsigned int crc = 0xffffffffL;
  crc = ~CrcCalc(buf, size);
  return crc;
}

/* _O_BINARY is a MSWindows open() mode flag.  When absent, MSWindows
 * open() translates CR+LF to LF; when present, it passes bytes
 * through faithfully.  Under *nix, we are always in binary mode, so
 * the following #define turns this flag into a no-op w.r.t.  bitwise
 * OR.  Note that this would be DANGEROUS AND UNSOUND if we used
 * _O_BINARY other than as a bitwise OR mask (e.g., as a bitwise AND
 * mask to check for binary mode), but it seems OK in the limited
 * context of the following small function. */
#ifndef _O_BINARY
# define _O_BINARY 0
#endif

int ApplyBinaryPatch(const wchar_t *old_file, const wchar_t *patch_file,
                     const wchar_t *new_file) {
  int ret = OK;
  int ofd = -1;
  int nfd = -1;
  unsigned char *buf = NULL;

  int pfd = _wopen(patch_file, O_RDONLY | _O_BINARY);
  if (pfd < 0) return READ_ERROR;

  do {
    MBSPatchHeader header;
    if ((ret = MBS_ReadHeader(pfd, &header)))
      break;

    ofd = _wopen(old_file, O_RDONLY | _O_BINARY);
    if (ofd < 0) {
      ret = READ_ERROR;
      break;
    }

    struct stat os;
    if ((ret = fstat(ofd, &os))) {
      ret = READ_ERROR;
      break;
    }

    if (os.st_size != header.slen) {
      ret = UNEXPECTED_ERROR;
      break;
    }

    buf = (unsigned char*) malloc(header.slen);
    if (!buf) {
      ret = MEM_ERROR;
      break;
    }

    if (read(ofd, buf, header.slen) != header.slen) {
      ret = READ_ERROR;
      break;
    }

    if (CalculateCrc(buf, header.slen) != header.scrc32) {
      ret = CRC_ERROR;
      break;
    }

    nfd = _wopen(new_file, O_WRONLY | O_TRUNC | O_CREAT | _O_BINARY);
    if (nfd < 0) {
      ret = READ_ERROR;
      break;
    }

    ret = MBS_ApplyPatch(&header, pfd, buf, nfd);
  } while (0);

  free(buf);
  close(pfd);
  if (ofd >= 0) close(ofd);
  if (nfd >= 0) close(nfd);
  return ret;
}
