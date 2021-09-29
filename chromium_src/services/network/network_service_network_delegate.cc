/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "services/network/network_service_network_delegate.h"

#include "services/network/cookie_settings.h"

#define IsCookieAccessible IsEphemeralCookieAccessible
#define IsPrivacyModeEnabled IsEphemeralPrivacyModeEnabled
#define AnnotateAndMoveUserBlockedCookies \
  AnnotateAndMoveUserBlockedEphemeralCookies

#include "../../../../services/network/network_service_network_delegate.cc"

#undef AnnotateAndMoveUserBlockedCookies
#undef IsPrivacyModeEnabled
#undef IsCookieAccessible
