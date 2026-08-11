// Copyright (c) 2026 The BNES Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BNES_BNS_CONSTANTS_H_
#define BRAVE_BNES_BNS_CONSTANTS_H_

#include <string_view>

namespace bnes {

// Default path-style IPFS gateway host for BnesBrowser. Callers must still pass
// the host through IsAllowedGatewayUrl after every redirect hop.
inline constexpr std::string_view kDefaultIpfsGatewayHost =
    "ipfs.bearnetwork.net";

// Path prefix used with a validated CID. Never concatenate untrusted input
// before IsValidCid has accepted the CID text.
inline constexpr std::string_view kIpfsPathPrefix = "/ipfs/";

// User-facing name suffix. Navigation hosts must end with this label and must
// contain at least one non-empty label before it.
inline constexpr std::string_view kBnesHostSuffix = ".bnes";

// Scheme registered for native routing (Phase 4). Secure-context registration
// remains a separate Chromium touch point and is not claimed complete here.
inline constexpr std::string_view kBnesScheme = "bnes";

// Helper to test whether a URL uses the BNES scheme.
inline bool IsBnesScheme(const GURL& url) {
  return url.is_valid() && url.SchemeIs(kBnesScheme);
}

}  // namespace bnes

#endif  // BRAVE_BNES_BNS_CONSTANTS_H_
