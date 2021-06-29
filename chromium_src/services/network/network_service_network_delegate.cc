/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "services/network/network_service_network_delegate.h"

#define OnCanGetCookies OnCanGetCookies_ChromiumImpl
#define OnCanSetCookie OnCanSetCookie_ChromiumImpl

#include "../../../../services/network/network_service_network_delegate.cc"

#undef OnCanSetCookie
#undef OnCanGetCookies

namespace network {

bool NetworkServiceNetworkDelegate::OnCanGetCookies(
    const net::URLRequest& request,
    bool allowed_from_caller) {
  // Enable ephemeral storage support for the call.
  auto scoped_ephemeral_storage_awareness =
      network_context_->cookie_manager()
          ->cookie_settings()
          .CreateScopedEphemeralStorageAwareness();
  return OnCanGetCookies_ChromiumImpl(request, allowed_from_caller);
}

bool NetworkServiceNetworkDelegate::OnCanSetCookie(
    const net::URLRequest& request,
    const net::CanonicalCookie& cookie,
    net::CookieOptions* options,
    bool allowed_from_caller) {
  // Enable ephemeral storage support for the call.
  auto scoped_ephemeral_storage_awareness =
      network_context_->cookie_manager()
          ->cookie_settings()
          .CreateScopedEphemeralStorageAwareness();
  return OnCanSetCookie_ChromiumImpl(request, cookie, options,
                                     allowed_from_caller);
}

}  // namespace network
