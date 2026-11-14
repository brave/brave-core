/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"

namespace {

// Returns true when the user has turned off "Autofill passwords using Brave" in
// brave://password-manager/settings. The plaster for this file checks it at the
// top of ChromePasswordManagerClient::IsFillingEnabled() and
// IsSavingAndFillingEnabled(), which suppresses the fill dropdown, filling on
// page load and the "Save password?" prompt.
bool IsBravePasswordFillingDisabled(content::WebContents* web_contents) {
  const Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  return profile &&
         !profile->GetPrefs()->GetBoolean(kBravePasswordManagerFillEnabled);
}

}  // namespace

#define IsGuestSession                                                   \
  IsGuestSession() ||                                                    \
      (!profile->GetPrefs()->GetBoolean(kBraveAutofillPrivateWindows) && \
       (IsOffTheRecord() || profile->IsTor())) ||                        \
      profile->IsGuestSession
#include <chrome/browser/password_manager/chrome_password_manager_client.cc>
#undef IsGuestSession
