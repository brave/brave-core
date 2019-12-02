/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wayback_machine/brave_wayback_machine_util.h"

#include "base/containers/flat_set.h"
#include "brave/browser/brave_wayback_machine/brave_wayback_machine_infobar_delegate.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"

void CheckWaybackMachineIfNeeded(content::WebContents* contents,
                                 int response_code) {
  static base::flat_set<int> responses =
      { 404, 408, 410, 451, 500, 502, 503, 504,
        509, 520, 521, 523, 524, 525, 526 };

  if (responses.find(response_code) != responses.end())
    BraveWaybackMachineInfoBarDelegate::Create(contents);
}

bool IsWaybackMachineEnabled(content::BrowserContext* context) {
  Profile* profile = Profile::FromBrowserContext(context);
  return profile->GetPrefs()->GetBoolean(kBraveWaybackMachineEnabled);
}
