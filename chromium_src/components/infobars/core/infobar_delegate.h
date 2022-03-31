/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_INFOBARS_CORE_INFOBAR_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_INFOBARS_CORE_INFOBAR_DELEGATE_H_

// Add corresponding value into
// brave/android/java/org/chromium/chrome/browser/infobar/BraveInfoBarIdentifier.java
// When there will be too many items, redo java_cpp_enum.py to generate it
// automatically

#define BRAVE_INFOBAR_DELEGATE_IDENTIFIERS  \
  BRAVE_CONFIRM_P3A_INFOBAR_DELEGATE = 500, \
  WAYBACK_MACHINE_INFOBAR_DELEGATE = 502,   \
  SYNC_V2_MIGRATE_INFOBAR_DELEGATE = 503,   \
  ANDROID_SYSTEM_SYNC_DISABLED_INFOBAR = 504,

#include "src/components/infobars/core/infobar_delegate.h"

#undef BRAVE_INFOBAR_DELEGATE_IDENTIFIERS

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_INFOBARS_CORE_INFOBAR_DELEGATE_H_
