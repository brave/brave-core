/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/devtools/devtools_ui_bindings.h"

#define IsValidRemoteFrontendURL IsValidRemoteFrontendURL_ChromiumImpl
#include "../../../../../../chrome/browser/devtools/devtools_ui_bindings.cc"
#undef IsValidRemoteFrontendURL

bool DevToolsUIBindings::IsValidRemoteFrontendURL(const GURL& url) {
  PrefService* local_state = g_browser_process->local_state();
  if (local_state) {
    if (!local_state->GetBoolean(kRemoteDebuggingEnabled)) {
      LOG(ERROR)
          << "Remote debugging is DISABLED. If you want to use it, please "
             "enable in brave://settings/privacy";
      return false;
    }
  }

  return IsValidRemoteFrontendURL_ChromiumImpl(url);
}
