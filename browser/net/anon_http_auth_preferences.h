/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_ANON_HTTP_AUTH_PREFERENCES_H_
#define BRAVE_BROWSER_NET_ANON_HTTP_AUTH_PREFERENCES_H_

#include "net/http/http_auth_preferences.h"
#include "net/base/net_export.h"

class GURL;

namespace net {

class NET_EXPORT AnonHttpAuthPreferences : public HttpAuthPreferences {
 public:
  AnonHttpAuthPreferences();
  ~AnonHttpAuthPreferences() override;

  bool CanUseDefaultCredentials(const GURL& auth_origin) const override;
};

}  // namespace net

#endif  // BRAVE_BROWSER_NET_ANON_HTTP_AUTH_PREFERENCES_H_
