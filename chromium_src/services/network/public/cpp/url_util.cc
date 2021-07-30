/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "services/network/public/cpp/url_util.h"

#include "base/feature_list.h"
#include "build/build_config.h"
#include "net/net_buildflags.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/is_potentially_trustworthy.h"
#include "url/gurl.h"
#include "url/url_constants.h"

#define IsURLHandledByNetworkService IsURLHandledByNetworkService_ChromiumImpl
#include "../../../../../../services/network/public/cpp/url_util.cc"
#undef IsURLHandledByNetworkService

namespace network {

bool IsURLHandledByNetworkService(const GURL& url) {
  if (url.SchemeIs("ipns") || url.SchemeIs("ipfs")) {
    return true;
  }
  return IsURLHandledByNetworkService_ChromiumImpl(url);
}

}  // namespace network
