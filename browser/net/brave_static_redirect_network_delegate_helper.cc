/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_static_redirect_network_delegate_helper.h"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "brave/components/static_redirect_helper/static_redirect_helper.h"
#include "net/base/net_errors.h"

namespace brave {

int OnBeforeURLRequest_StaticRedirectWorkForGURL(
    const GURL& request_url,
    GURL* new_url) {
  brave::StaticRedirectHelper(request_url, new_url);
  return net::OK;
}

}  // namespace brave
