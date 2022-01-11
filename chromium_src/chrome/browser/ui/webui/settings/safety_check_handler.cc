/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/generated_resources.h"

#undef IDS_SETTINGS_SAFETY_CHECK_SAFE_BROWSING_ENABLED_STANDARD_AVAILABLE_ENHANCED  // NOLINT
// NOLINTNEXTLINE
#define IDS_SETTINGS_SAFETY_CHECK_SAFE_BROWSING_ENABLED_STANDARD_AVAILABLE_ENHANCED \
  IDS_SETTINGS_BRAVE_SAFETY_CHECK_SAFE_BROWSING_ENABLED_STANDARD_AVAILABLE_ENHANCED  // NOLINT

#include "src/chrome/browser/ui/webui/settings/safety_check_handler.cc"
