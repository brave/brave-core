/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CAPTIVE_PORTAL_CORE_CAPTIVE_PORTAL_DETECTOR_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CAPTIVE_PORTAL_CORE_CAPTIVE_PORTAL_DETECTOR_H_

// We use our own endpoint for detecting captive portals.
#define GetDefaultUrl \
  GetDefaultUrl();    \
  static const std::string_view GetDefaultUrl_ChromiumImpl

#include <components/captive_portal/core/captive_portal_detector.h>  // IWYU pragma: export
#undef GetDefaultUrl

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CAPTIVE_PORTAL_CORE_CAPTIVE_PORTAL_DETECTOR_H_
