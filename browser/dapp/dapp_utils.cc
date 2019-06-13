/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/dapp/dapp_utils.h"

#include "base/callback.h"
#include "brave/browser/dapp/wallet_installation_permission_request.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/permissions/permission_request_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

namespace {
base::Closure g_quit_closure_for_test;
}  // namespace

bool DappDetectionEnabled(content::BrowserContext* browser_context) {
  Profile* profile = Profile::FromBrowserContext(browser_context);
  return profile->GetPrefs()->GetBoolean(kDappDetectionEnabled);
}

void RequestWalletInstallationPermission(content::WebContents* web_contents) {
  DCHECK(web_contents);

  if (g_quit_closure_for_test)
    g_quit_closure_for_test.Run();

  PermissionRequestManager::FromWebContents(web_contents)->AddRequest(
      new WalletInstallationPermissionRequest(web_contents));
}

void SetQuitClosureForDappDetectionTest(const base::Closure& quit_closure) {
  g_quit_closure_for_test = quit_closure;
}
