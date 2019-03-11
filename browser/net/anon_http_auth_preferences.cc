/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/anon_http_auth_preferences.h"

namespace net {

AnonHttpAuthPreferences::AnonHttpAuthPreferences() = default;
AnonHttpAuthPreferences::~AnonHttpAuthPreferences() = default;

bool AnonHttpAuthPreferences::CanUseDefaultCredentials(
    const GURL& auth_origin) const {
  return false;
}

}  // namespace net
