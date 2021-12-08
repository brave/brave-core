/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Nullify the Origin header for cross-origin CORS requests
// originating from a .onion address.
#define BRAVE_CORS_URL_LOADER_START_REQUEST                        \
  if (base::EndsWith(request_.request_initiator->host(), ".onion", \
                     base::CompareCase::INSENSITIVE_ASCII) &&      \
      !request_.request_initiator->IsSameOriginWith(               \
          url::Origin::Create(request_.url))) {                    \
    request_.headers.SetHeader(net::HttpRequestHeaders::kOrigin,   \
                               url::Origin().Serialize());         \
  } else /* NOLINT */

#include "src/services/network/cors/cors_url_loader.cc"
#undef BRAVE_CORS_URL_LOADER_START_REQUEST
