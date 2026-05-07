// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_OBLIVIOUS_HTTP_UTILS_H_
#define BRAVE_COMPONENTS_OBLIVIOUS_HTTP_UTILS_H_

#include <string>

#include "net/http/http_request_headers.h"
#include "services/network/public/mojom/oblivious_http_request.mojom.h"

namespace oblivious_http {

// Applies relay_request_headers to |headers| if present. If brave_services_key
// is set, also computes a Digest (SHA-256 of |encrypted_body|) and an
// Authorization header signed with that key, and adds both to |headers|.
void ApplyBraveRelayHeaders(const network::mojom::ObliviousHttpRequest& request,
                            const std::string& encrypted_body,
                            net::HttpRequestHeaders& headers);

}  // namespace oblivious_http

#endif  // BRAVE_COMPONENTS_OBLIVIOUS_HTTP_UTILS_H_
