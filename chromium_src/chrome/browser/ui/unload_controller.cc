/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/unload_controller.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"

#define TabStripEmpty                                                         \
  TabStripEmpty() {                                                           \
    if (browser_->profile()->GetPrefs()->GetBoolean(kEnableClosingLastTab)) { \
      TabStripEmpty_ChromiumImpl();                                           \
    }                                                                         \
  }                                                                           \
  void UnloadController::TabStripEmpty_ChromiumImpl
#include "src/chrome/browser/ui/unload_controller.cc"
#undef TabStripEmpty
