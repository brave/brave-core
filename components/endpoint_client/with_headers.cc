/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/with_headers.h"

#include "base/check.h"
#include "base/strings/strcat.h"

namespace endpoint_client::internal {

void SetBearerToken(net::HttpRequestHeaders& headers,
                    std::string_view bearer_token) {
  CHECK(!bearer_token.empty());
  headers.SetHeader(net::HttpRequestHeaders::kAuthorization,
                    base::StrCat({"Bearer ", bearer_token}));
}

}  // namespace endpoint_client::internal
