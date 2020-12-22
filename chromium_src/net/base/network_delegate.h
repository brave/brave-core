/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_NET_BASE_NETWORK_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_NET_BASE_NETWORK_DELEGATE_H_

#define BRAVE_NETWORK_DELEGATE_H                                \
  virtual bool OnCanGetCookiesWithoutEphemeralCookies(          \
      const URLRequest& request, bool allowed_from_caller);     \
  virtual bool OnCanSetCookieWithoutEphemeralCookies(           \
      const URLRequest& request, const CanonicalCookie& cookie, \
      CookieOptions* options, bool allowed_from_caller);

#include "../../../../net/base/network_delegate.h"

#undef BRAVE_NETWORK_DELEGATE_H

#endif  // BRAVE_CHROMIUM_SRC_NET_BASE_NETWORK_DELEGATE_H_
