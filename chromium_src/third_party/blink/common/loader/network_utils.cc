/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/common/loader/network_utils.h"

#include "url/gurl.h"

#define IsURLHandledByNetworkService IsURLHandledByNetworkService_ChromiumImpl
#include "../../../../../../third_party/blink/common/loader/network_utils.cc"
#undef IsURLHandledByNetworkService

namespace blink {
namespace network_utils {

bool IsURLHandledByNetworkService(const GURL& url) {
  if (url.SchemeIs("ipns") || url.SchemeIs("ipfs")) {
    return true;
  }
  return IsURLHandledByNetworkService_ChromiumImpl(url);
}

}  // namespace network_utils
}  // namespace blink
