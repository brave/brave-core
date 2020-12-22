/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_NETWORK_SERVICE_NETWORK_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_NETWORK_SERVICE_NETWORK_DELEGATE_H_

#define BRAVE_NETWORK_SERVICE_NETWORK_DELEGATE_H                          \
 private:                                                                 \
  bool OnCanGetCookiesWithoutEphemeralCookies(                            \
      const net::URLRequest& request, bool allowed_from_caller) override; \
  bool OnCanSetCookieWithoutEphemeralCookies(                             \
      const net::URLRequest& request, const net::CanonicalCookie& cookie, \
      net::CookieOptions* options, bool allowed_from_caller) override;

#include "../../../../../services/network/network_service_network_delegate.h"

#undef BRAVE_NETWORK_SERVICE_NETWORK_DELEGATE_H

#endif  // BRAVE_CHROMIUM_SRC_SERVICES_NETWORK_NETWORK_SERVICE_NETWORK_DELEGATE_H_
