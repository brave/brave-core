/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Pull in the header override first so its declaration-splitting macros run and
// are undefined before we define the definition-redirecting macros below. This
// mirrors the pattern in brave/chromium_src/net/http/http_util.cc.
#include "chrome/browser/password_manager/chrome_password_manager_client.h"

#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"

#define IsGuestSession                                                   \
  IsGuestSession() ||                                                    \
      (!profile->GetPrefs()->GetBoolean(kBraveAutofillPrivateWindows) && \
       (IsOffTheRecord() || profile->IsTor())) ||                        \
      profile->IsGuestSession

// Redirect the upstream bodies to the *_ChromiumImpl methods declared by the
// accompanying header override, so Brave can gate them on the
// kBravePasswordManagerFillEnabled pref below.
#define IsSavingAndFillingEnabled IsSavingAndFillingEnabled_ChromiumImpl
#define IsFillingEnabled IsFillingEnabled_ChromiumImpl
#include <chrome/browser/password_manager/chrome_password_manager_client.cc>
#undef IsFillingEnabled
#undef IsSavingAndFillingEnabled
#undef IsGuestSession

namespace {

// Returns true when Brave's built-in password autofill has been disabled by the
// user via chrome://password-manager/settings. Disabling it suppresses both the
// fill dropdown and the "offer to save password" prompt.
bool IsBravePasswordFillingDisabled(content::WebContents* web_contents) {
  const Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  return profile &&
         !profile->GetPrefs()->GetBoolean(kBravePasswordManagerFillEnabled);
}

}  // namespace

bool ChromePasswordManagerClient::IsFillingEnabled(const GURL& url) const {
  if (IsBravePasswordFillingDisabled(web_contents())) {
    return false;
  }
  return IsFillingEnabled_ChromiumImpl(url);
}

bool ChromePasswordManagerClient::IsSavingAndFillingEnabled(
    const GURL& url) const {
  if (IsBravePasswordFillingDisabled(web_contents())) {
    return false;
  }
  return IsSavingAndFillingEnabled_ChromiumImpl(url);
}
