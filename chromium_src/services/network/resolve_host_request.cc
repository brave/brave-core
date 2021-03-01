/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_START                                   \
  if (internal_request_->GetTextResults()) {          \
    response_client->OnTextResults(                   \
        internal_request_->GetTextResults().value()); \
  }

#include "../../../../services/network/resolve_host_request.cc"
#undef BRAVE_START
