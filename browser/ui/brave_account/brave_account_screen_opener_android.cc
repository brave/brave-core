/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/android/jni_android.h"
#include "base/functional/function_ref.h"
#include "brave/browser/ui/android/brave_account/jni_headers/BraveAccountScreenOpener_jni.h"
#include "brave/browser/ui/brave_account/brave_account_dialog_opener.h"
#include "brave/browser/ui/brave_account/brave_account_settings_opener_android.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

// Implements `brave_account_dialog_opener.h` and
// `brave_account_settings_opener_android.h` on Android.

namespace brave_account {

namespace {

// Marks a page that has just opened a screen, so that requests it makes before
// the screen covers it are ignored. The tracker removes itself once the page is
// hidden, which only happens if the screen covers the whole page.
class ScreenTracker : public content::WebContentsObserver,
                      public content::WebContentsUserData<ScreenTracker> {
 public:
  ~ScreenTracker() override = default;

 private:
  friend class content::WebContentsUserData<ScreenTracker>;

  explicit ScreenTracker(content::WebContents* web_contents)
      : content::WebContentsObserver(web_contents),
        content::WebContentsUserData<ScreenTracker>(*web_contents) {}

  // content::WebContentsObserver:
  void OnVisibilityChanged(content::Visibility visibility) override {
    if (visibility == content::Visibility::HIDDEN) {
      GetWebContents().RemoveUserData(UserDataKey());
    }
  }

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(ScreenTracker);

// Runs `open`, which opens a screen from `web_contents` and returns whether
// it did. Once a screen is open, further requests from `web_contents` are
// ignored until it is hidden.
bool MaybeOpenScreen(content::WebContents& web_contents,
                     base::FunctionRef<bool()> open) {
  if (ScreenTracker::FromWebContents(&web_contents) || !open()) {
    return false;
  }

  ScreenTracker::CreateForWebContents(&web_contents);
  return true;
}

}  // namespace

void OpenBraveAccountDialog(content::WebContents& web_contents,
                            mojom::DialogMode dialog_mode) {
  MaybeOpenScreen(web_contents, [&] {
    return Java_BraveAccountScreenOpener_openBraveAccountDialog(
        base::android::AttachCurrentThread(), web_contents.GetJavaWebContents(),
        std::to_underlying(dialog_mode));
  });
}

void OpenSettingsPage(content::WebContents& web_contents,
                      mojom::SettingsPage page) {
  if (MaybeOpenScreen(web_contents, [&] {
        return Java_BraveAccountScreenOpener_openSettingsPage(
            base::android::AttachCurrentThread(),
            web_contents.GetJavaWebContents(), std::to_underlying(page));
      })) {
    web_contents.Close();
  }
}

}  // namespace brave_account
