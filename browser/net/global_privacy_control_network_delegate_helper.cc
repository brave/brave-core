/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/global_privacy_control_network_delegate_helper.h"

#include <memory>

#include "base/check.h"
#include "base/feature_list.h"
#include "brave/browser/net/url_context.h"
#include "brave/components/constants/network_constants.h"
#include "brave/components/global_privacy_control/global_privacy_control_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"

namespace brave {

template <template <typename> class T>
int OnBeforeStartTransaction_GlobalPrivacyControlWork(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    T<BraveRequestInfo> ctx) {
  CHECK(ctx);
  Profile* profile = Profile::FromBrowserContext(ctx->browser_context());
  if (profile && global_privacy_control::IsGlobalPrivacyControlEnabled(
                     profile->GetPrefs())) {
    headers->SetHeader(kSecGpcHeader, "1");
  }
  return net::OK;
}

template int OnBeforeStartTransaction_GlobalPrivacyControlWork<std::shared_ptr>(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx);

template int OnBeforeStartTransaction_GlobalPrivacyControlWork<base::WeakPtr>(
    net::HttpRequestHeaders* headers,
    const ResponseCallback& next_callback,
    base::WeakPtr<BraveRequestInfo> ctx);

}  // namespace brave
