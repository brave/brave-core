/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/common/pref_names.h"
#include "chrome/android/chrome_jni_headers/InterceptNavigationDelegateImpl_jni.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
#include "components/navigation_interception/navigation_params.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "net/base/escape.h"
#include "url/gurl.h"

namespace {

using navigation_interception::InterceptNavigationDelegate;
using navigation_interception::NavigationParams;

Profile* GetOriginalProfile() {
  return ProfileManager::GetActiveUserProfile()->GetOriginalProfile();
}

bool ShouldPlayVideoInBrowser(const GURL& url) {
  if (!GetOriginalProfile()->GetPrefs()->GetBoolean(
          kPlayYTVideoInBrowserEnabled)) {
    return false;
  }

  if (url.host().find("youtube.com") != std::string::npos ||
      url.host().find("youtu.be") != std::string::npos) {
    return true;
  }

  return false;
}

class BraveInterceptNavigationDelegate : public InterceptNavigationDelegate {
 public:
  BraveInterceptNavigationDelegate(JNIEnv* env, jobject jdelegate)
      : InterceptNavigationDelegate(env, jdelegate) {}

  bool ShouldIgnoreNavigation(
      const NavigationParams& navigation_params) override {
    NavigationParams chrome_navigation_params(navigation_params);
    chrome_navigation_params.url() =
        GURL(net::EscapeExternalHandlerValue(navigation_params.url().spec()));

    if (ShouldPlayVideoInBrowser(chrome_navigation_params.url()))
      return false;

    return InterceptNavigationDelegate::ShouldIgnoreNavigation(
        chrome_navigation_params);
  }
};

}  // namespace

static void JNI_InterceptNavigationDelegateImpl_AssociateWithWebContents(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jdelegate,
    const base::android::JavaParamRef<jobject>& jweb_contents) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::WebContents* web_contents =
      content::WebContents::FromJavaWebContents(jweb_contents);
  InterceptNavigationDelegate::Associate(
      web_contents,
      std::make_unique<BraveInterceptNavigationDelegate>(env, jdelegate));
}
