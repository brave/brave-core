/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_RESOLVE_HOST_REQUEST_START                                  \
  if (internal_request_->GetTextResults() &&                              \
      !internal_request_->GetTextResults()->empty()) {                    \
    response_client->OnTextResults(*internal_request_->GetTextResults()); \
  }

#include "src/services/network/resolve_host_request.cc"
#undef BRAVE_RESOLVE_HOST_REQUEST_START
