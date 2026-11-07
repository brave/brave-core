/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_account/brave_account_settings_opener_android.h"

#include <utility>

#include "base/android/jni_android.h"
#include "brave/browser/ui/android/brave_account/jni_headers/BraveAccountSettingsOpener_jni.h"
#include "content/public/browser/web_contents.h"

namespace brave_account {

void OpenSettingsPage(content::WebContents& web_contents,
                      mojom::SettingsPage page) {
  if (Java_BraveAccountSettingsOpener_openSettingsPage(
          base::android::AttachCurrentThread(),
          web_contents.GetJavaWebContents(), std::to_underlying(page))) {
    web_contents.Close();
  }
}

}  // namespace brave_account
