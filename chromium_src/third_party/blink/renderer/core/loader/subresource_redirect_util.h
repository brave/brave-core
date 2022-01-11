/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_SUBRESOURCE_REDIRECT_UTIL_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_SUBRESOURCE_REDIRECT_UTIL_H_

#include "third_party/blink/renderer/core/loader/subresource_redirect_util.h"

// Overriding this method so as to bypass CSP for $redirect-url filter option
// in adblock rules. This method was chosen for overriding because it has
// similar functionality (if a certain kind of subresource redirect then ignore
// CSP), but there's no relation between Lite Pages and redirect urls
#define ShouldDisableCSPCheckForLitePageSubresourceRedirectOrigin         \
  ShouldDisableCSPCheckForLitePageSubresourceRedirectOrigin(              \
      scoped_refptr<SecurityOrigin> litepage_subresource_redirect_origin, \
      mojom::blink::RequestContextType request_context,                   \
      ResourceRequest::RedirectStatus redirect_status, const KURL& url);  \
  bool ShouldDisableCSPCheckForLitePageSubresourceRedirectOrigin_ChromiumImpl

#include "src/third_party/blink/renderer/core/loader/subresource_redirect_util.h"
#undef ShouldDisableCSPCheckForLitePageSubresourceRedirectOrigin

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_SUBRESOURCE_REDIRECT_UTIL_H_
