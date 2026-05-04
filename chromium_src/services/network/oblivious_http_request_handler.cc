// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/containers/flat_map.h"
#include "brave/components/brave_service_keys/brave_service_key_utils.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/mojom/oblivious_http_request.mojom.h"
#include "url/gurl.h"

namespace {

// Applies relay_request_headers to |headers| if present. If brave_services_key
// is set, also computes a Digest (SHA-256 of |encrypted_body|) and an
// Authorization header signed with that key, and adds both to |headers|.
void ApplyBraveRelayHeaders(
    const network::mojom::ObliviousHttpRequest& request,
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

}  // namespace

#define BRAVE_OBLIVIOUS_HTTP_CONTINUE_HANDLING_REQUEST        \
  ApplyBraveRelayHeaders(*state->request, *maybe_encrypted_blob, \
                         resource_request->headers);

#include <services/network/oblivious_http_request_handler.cc>

#undef BRAVE_OBLIVIOUS_HTTP_CONTINUE_HANDLING_REQUEST
