/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_INFOBARS_CORE_INFOBAR_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_INFOBARS_CORE_INFOBAR_DELEGATE_H_

// Add corresponding value into
// NOLINTNEXTLINE
// brave/android/java/org/chromium/chrome/browser/infobar/BraveInfoBarIdentifier.java
// When there will be too many items, redo java_cpp_enum.py to generate it
// automatically

#define BRAVE_INFOBAR_DELEGATE_IDENTIFIERS                                 \
  BRAVE_CONFIRM_P3A_INFOBAR_DELEGATE = 500, SYNC_CANNOT_RUN_INFOBAR = 505, \
  WEB_DISCOVERY_INFOBAR_DELEGATE = 506,                                    \
  BRAVE_SYNC_ACCOUNT_DELETED_INFOBAR = 507,                                \
  BRAVE_REQUEST_OTR_INFOBAR_DELEGATE = 508,                                \
  DEV_CHANNEL_DEPRECATION_INFOBAR_DELEGATE = 509,                          \
  SEARCH_RESULT_AD_CLICKED_INFOBAR_DELEGATE = 510,

// Deprecated:
// WAYBACK_MACHINE_INFOBAR_DELEGATE = 502
// SYNC_V2_MIGRATE_INFOBAR_DELEGATE = 503
// ANDROID_SYSTEM_SYNC_DISABLED_INFOBAR = 504

#include "src/components/infobars/core/infobar_delegate.h"  // IWYU pragma: export

#undef BRAVE_INFOBAR_DELEGATE_IDENTIFIERS

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_INFOBARS_CORE_INFOBAR_DELEGATE_H_
