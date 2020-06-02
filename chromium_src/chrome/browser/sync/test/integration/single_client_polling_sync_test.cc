/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/prefs/session_startup_pref.h"

#define BRAVE_SET_DEFAULT_STARTUP_PREF \
  SessionStartupPref::SetStartupPref(  \
      GetProfile(0)->GetPrefs(),       \
      SessionStartupPref(SessionStartupPref::GetDefaultStartupType()));

#include "../../../../../../../chrome/browser/sync/test/integration/single_client_polling_sync_test.cc"

#undef BRAVE_SET_DEFAULT_STARTUP_PREF
