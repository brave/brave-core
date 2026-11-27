// Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com) All rights reserved.
// SPDX-License-Identifier: Apache-2.0

#include <openssl/bio.h>

#include <errno.h>

#include "internal.h"


int bio_errno_should_retry(int return_value) {
  return return_value == -1 && bio_errno_is_retryable(errno);
}

int bio_errno_is_retryable(int error) {
  return
#ifdef EWOULDBLOCK
      error == EWOULDBLOCK ||
#endif
#ifdef ENOTCONN
      error == ENOTCONN ||
#endif
#ifdef EINTR
      error == EINTR ||
#endif
#ifdef EAGAIN
      error == EAGAIN ||
#endif
#ifdef EPROTO
      error == EPROTO ||
#endif
#ifdef EINPROGRESS
      error == EINPROGRESS ||
#endif
#ifdef EALREADY
      error == EALREADY ||
#endif
      0;
}
