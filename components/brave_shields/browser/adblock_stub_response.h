/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_ADBLOCK_STUB_RESPONSE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_ADBLOCK_STUB_RESPONSE_H_

#include <string>
#include "services/network/public/mojom/url_response_head.mojom-forward.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace network {
struct ResourceRequest;
}  // namespace network

namespace brave_shields {

// Intercepts certain requests and blocks them by silently returning 200 OK
// and not allowing them to hit the network.
void MakeStubResponse(const absl::optional<std::string>& data_url,
                      const network::ResourceRequest& request,
                      network::mojom::URLResponseHeadPtr* response,
                      std::string* data);

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_ADBLOCK_STUB_RESPONSE_H_
