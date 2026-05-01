// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Applies any caller-provided outer relay request headers to the
// network::ResourceRequest used for the POST to the OHTTP relay. These headers
// are sent to the relay in the clear (not encapsulated in bhttp).
#define BRAVE_OBLIVIOUS_HTTP_CONTINUE_HANDLING_REQUEST \
  if (state->request->relay_request_headers) {         \
    resource_request->headers.MergeFrom(               \
        *state->request->relay_request_headers);       \
  }

#include <services/network/oblivious_http_request_handler.cc>

#undef BRAVE_OBLIVIOUS_HTTP_CONTINUE_HANDLING_REQUEST
