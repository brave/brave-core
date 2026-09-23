/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/android/chrome_jni_headers/BraveRelaunchUtils_jni.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/prefs/pref_service.h"

namespace chrome {
namespace android {

void JNI_BraveRelaunchUtils_Restart(JNIEnv* env) {
  // Pref changes are batched and written to disk asynchronously. The Android
  // restart path finishes activities and then SIGKILLs the browser process
  // from a sibling process (BrowserRestartActivity) without a graceful prefs
  // flush, so a setting changed shortly before a restart can be lost if it
  // hasn't been written out yet. Flush pending pref writes synchronously here,
  // mirroring BrowserProcessImpl::EndSession() on desktop, so changes survive
  // the restart.
  if (auto* local_state = g_browser_process->local_state()) {
    local_state->CommitPendingWrite();
  }
  if (auto* profile_manager = g_browser_process->profile_manager()) {
    for (Profile* profile : profile_manager->GetLoadedProfiles()) {
      if (profile->GetPrefs()) {
        profile->GetPrefs()->CommitPendingWrite();
      }
    }
  }

  chrome::AttemptRestart();
}

}  // namespace android
}  // namespace chrome

DEFINE_JNI(BraveRelaunchUtils)
