// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/web_applications/pwa_install_page_action.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"

// Consider pref in UpdateVisibility() before calling Show().
#define IsProbablyPromotableWebApp() \
  IsProbablyPromotableWebApp() && CheckPinPwaInstallButtonPref(web_contents)
#include <chrome/browser/ui/web_applications/pwa_install_page_action.cc>
#undef IsProbablyPromotableWebApp

bool PwaInstallPageActionController::CheckPinPwaInstallButtonPref(
    content::WebContents* web_contents) {
  CHECK(web_contents);

  auto* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  CHECK(profile);

  PrefService* prefs = profile->GetPrefs();
  CHECK(prefs);

  if (!pin_pwa_install_button_pref_member_) {
    pin_pwa_install_button_pref_member_ = std::make_unique<BooleanPrefMember>();
    pin_pwa_install_button_pref_member_->Init(
        prefs::kPinPwaInstallButton, prefs,
        base::BindRepeating(&PwaInstallPageActionController::UpdateVisibility,
                            base::Unretained(this)));
  }

  return pin_pwa_install_button_pref_member_->GetValue();
}
