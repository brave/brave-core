// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

// Back side: draining AWS-LC's error queue.

#include <openssl/err.h>

#include <stdio.h>

#include "internal/backend.h"

void awslc_prov_error_clear(void) {
  ERR_clear_error();
}

int awslc_prov_error_shift(AWSLC_PROV_ERROR *out) {
  const char *file = NULL;
  const char *data = NULL;
  int line = 0;
  int flags = 0;
  uint32_t packed = 0;
  uint32_t library = 0;
  uint32_t reason = 0;

  if (out == NULL) {
    return 0;
  }
  awslc_prov_cleanse(out, sizeof(*out));

  packed = ERR_get_error_line_data(&file, &line, &data, &flags);
  if (packed == 0) {
    return 0;
  }
  library = (uint32_t)ERR_GET_LIB(packed);
  reason = (uint32_t)ERR_GET_REASON(packed);

  if (reason == 0 || library > AWSLC_PROV_ERROR_MAX_LIB) {
    // Nothing to name, or a library id too wide to tag. |detail| still reports
    // what AWS-LC said.
    out->reason = AWSLC_PROV_R_BACKEND_ERROR;
  } else if (reason < AWSLC_PROV_ERROR_FIRST_OWN_REASON) {
    // AWS-LC's cross-library reasons retain their reason code.
    out->reason = reason;
  } else {
    // Remap the library id and its reason code to the provider's reason codes.
    out->reason = AWSLC_PROV_ERROR_REASON(library, reason);
  }
  out->file = file;
  out->line = line;

  // Composed here because ERR_reason_error_string resolves against AWS-LC's
  // tables, which only this side can see. |data| belongs to the queue and dies on
  // the next call that touches it, so copy it now.
  if ((flags & ERR_FLAG_STRING) != 0 && data != NULL && data[0] != '\0') {
    snprintf(out->detail, sizeof(out->detail), "AWS-LC %s: %s: %s",
             ERR_lib_error_string(packed), ERR_reason_error_string(packed),
             data);
  } else {
    snprintf(out->detail, sizeof(out->detail), "AWS-LC %s: %s",
             ERR_lib_error_string(packed), ERR_reason_error_string(packed));
  }
  return 1;
}
