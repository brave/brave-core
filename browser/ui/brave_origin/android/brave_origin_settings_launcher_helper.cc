/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_origin/android/brave_origin_settings_launcher_helper.h"

#include "base/android/jni_android.h"
#include "brave/browser/ui/brave_origin/android/jni_headers/BraveOriginSettingsLauncherHelper_jni.h"
#include "brave/components/brave_origin/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

namespace brave_origin {

void OpenOriginSettingsForRestart(Profile* profile) {
  // Android has two post-purchase paths that surface the restart prompt
  // differently, so they can't share one code path:
  //
  //   * Play Store purchase: the SKUs credentials aren't ready yet at purchase
  //     time. The Java subscription flow (BraveOriginSubscriptionPrefs) opens
  //     the Origin settings screen showing a spinner ("Disabling features")
  //     while fetchOrderCredentials runs, then transitions to the restart
  //     prompt once the credentials arrive.
  //   * Credential refresh (e.g. linked from account.brave.com): the
  //     credentials are already present when we detect the purchase, so there
  //     is nothing to wait for -- we go straight to the restart prompt with no
  //     spinner.
  //
  // A Play purchase's fetchOrderCredentials writes kSkusState, which the shared
  // C++ service observes and reports as a first purchase here too. Skip it in
  // that case (identified by a non-empty purchase token) so the Java spinner
  // flow owns the prompt and the settings screen isn't opened twice.
  if (!profile->GetPrefs()
           ->GetString(prefs::kBraveOriginPurchaseTokenAndroid)
           .empty()) {
    return;
  }
  Java_BraveOriginSettingsLauncherHelper_showOriginSettingsForRestart(
      base::android::AttachCurrentThread());
}

}  // namespace brave_origin
