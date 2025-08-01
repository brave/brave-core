/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/security_interstitials/content/ssl_blocking_page_base.h"

#include "components/safe_browsing/core/common/safe_browsing_prefs.h"

#define IsSafeBrowsingPolicyManaged(PREF_SERVICE) \
  IsSafeBrowsingPolicyManaged(PREF_SERVICE);      \
  return false;
#include <components/security_interstitials/content/ssl_blocking_page_base.cc>
#undef IsSafeBrowsingPolicyManaged
