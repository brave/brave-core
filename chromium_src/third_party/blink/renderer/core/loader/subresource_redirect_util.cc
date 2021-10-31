/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/loader/subresource_redirect_util.h"
#include "net/base/features.h"

#define ShouldDisableCSPCheckForLitePageSubresourceRedirectOrigin \
  ShouldDisableCSPCheckForLitePageSubresourceRedirectOrigin_ChromiumImpl

#include "../../../../../../third_party/blink/renderer/core/loader/subresource_redirect_util.cc"
#undef ShouldDisableCSPCheckForLitePageSubresourceRedirectOrigin

namespace blink {

bool ShouldDisableCSPCheckForLitePageSubresourceRedirectOrigin(
    scoped_refptr<SecurityOrigin> litepage_subresource_redirect_origin,
    mojom::blink::RequestContextType request_context,
    ResourceRequest::RedirectStatus redirect_status,
    const KURL& url) {
  String host = url.Host();
  bool redirect_url_feature_enabled =
      base::FeatureList::IsEnabled(net::features::kAdblockRedirectUrl);
  bool is_script = request_context == mojom::blink::RequestContextType::SCRIPT;
  // Note that these strings are duplicated in
  // vendor/bat-native-ledger/src/bat/ledger/internal/endpoint/private_cdn/private_cdn_util.cc
  // and brave_ad_block_tp_network_delegate_helper.cc
  bool is_private_cdn_domain = host == "pcdn.brave.com" ||
                               host == "pcdn.bravesoftware.com" ||
                               host == "pcdn.brave.software";
  bool is_redirect =
      redirect_status == ResourceRequestHead::RedirectStatus::kFollowedRedirect;
  if (redirect_url_feature_enabled && is_script && is_private_cdn_domain &&
      is_redirect) {
    return true;
  }
  return ShouldDisableCSPCheckForLitePageSubresourceRedirectOrigin_ChromiumImpl(
      litepage_subresource_redirect_origin, request_context, redirect_status,
      url);
}

}  // namespace blink
