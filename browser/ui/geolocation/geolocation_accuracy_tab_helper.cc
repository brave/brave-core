/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/geolocation/geolocation_accuracy_tab_helper.h"

#include "brave/browser/ui/browser_dialogs.h"
#include "brave/browser/ui/geolocation/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(IS_WIN)
#include "brave/browser/ui/geolocation/geolocation_accuracy_utils_win.h"
#endif

// static
void GeolocationAccuracyTabHelper::MaybeCreateForWebContents(
    content::WebContents* contents) {
#if BUILDFLAG(IS_WIN)
  content::WebContentsUserData<
      GeolocationAccuracyTabHelper>::CreateForWebContents(contents);
#endif
}

GeolocationAccuracyTabHelper::GeolocationAccuracyTabHelper(
    content::WebContents* contents)
    : WebContentsUserData(*contents), WebContentsObserver(contents) {}

GeolocationAccuracyTabHelper::~GeolocationAccuracyTabHelper() = default;

void GeolocationAccuracyTabHelper::LaunchAccuracyHelperDialogIfNeeded() {
  if (auto* prefs =
          user_prefs::UserPrefs::Get(web_contents()->GetBrowserContext());
      !prefs->GetBoolean(kShowGeolocationAccuracyHelperDialog)) {
    return;
  }

  if (is_dialog_running_) {
    return;
  }

  if (dialog_asked_in_current_navigation_) {
    return;
  }

#if BUILDFLAG(IS_WIN)
  if (IsSystemLocationSettingEnabled()) {
    DVLOG(2) << __func__ << " : system location service is enabled.";
    return;
  }
#endif

  dialog_asked_in_current_navigation_ = true;
  is_dialog_running_ = true;
  brave::ShowGeolocationAccuracyHelperDialog(
      web_contents(),
      base::BindOnce(&GeolocationAccuracyTabHelper::OnDialogClosed,
                     weak_ptr_factory_.GetWeakPtr()));
}

void GeolocationAccuracyTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  dialog_asked_in_current_navigation_ = false;
}

void GeolocationAccuracyTabHelper::OnDialogClosed() {
  is_dialog_running_ = false;
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(GeolocationAccuracyTabHelper);
