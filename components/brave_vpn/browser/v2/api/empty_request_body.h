/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_EMPTY_REQUEST_BODY_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_EMPTY_REQUEST_BODY_H_

#include "base/values.h"

namespace brave_vpn::v2::endpoints {

// Request body with no fields, for bodyless Guardian VPN endpoints.
struct EmptyRequestBody {
  base::DictValue ToValue() const { return base::DictValue(); }
};

}  // namespace brave_vpn::v2::endpoints

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_V2_API_EMPTY_REQUEST_BODY_H_
