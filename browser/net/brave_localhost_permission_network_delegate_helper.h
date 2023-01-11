/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_LOCALHOST_PERMISSION_NETWORK_DELEGATE_HELPER_H_
#define BRAVE_BROWSER_NET_BRAVE_LOCALHOST_PERMISSION_NETWORK_DELEGATE_HELPER_H_

#include <memory>
#include <string>

#include "brave/browser/net/url_context.h"
#include "url/gurl.h"

class HostContentSettingsMap;
class PrefService;
class Profile;
struct BraveRequestInfo;

namespace net {
class HttpRequestHeaders;
class URLRequest;
}  // namespace net

namespace brave {

int OnBeforeURLRequest_LocalhostPermissionWork(
    const ResponseCallback& next_callback,
    std::scoped_refptr<BraveRequestInfo> ctx);

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_BRAVE_LOCALHOST_PERMISSION_NETWORK_DELEGATE_HELPER_H_
