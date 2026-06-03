// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/oblivious_http/utils.h"

#include "base/containers/flat_map.h"
#include "brave/components/brave_service_keys/brave_service_key_utils.h"

namespace oblivious_http {

void ApplyBraveRelayHeaders(const network::mojom::ObliviousHttpRequest& request,
                            const std::string& encrypted_body,
                            net::HttpRequestHeaders& headers) {
  if (request.relay_request_headers) {
    headers.MergeFrom(*request.relay_request_headers);
  }
  if (request.brave_services_key) {
    base::flat_map<std::string, std::string> signing_headers;
    const auto digest = brave_service_keys::GetDigestHeader(encrypted_body);
    signing_headers.emplace(digest.first, digest.second);
    const auto auth = brave_service_keys::GetAuthorizationHeader(
        *request.brave_services_key, signing_headers, request.relay_url,
        net::HttpRequestHeaders::kPostMethod, {"digest"});
    headers.SetHeader(digest.first, digest.second);
    headers.SetHeader(auth.first, auth.second);
  }
}

}  // namespace oblivious_http
