/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/common/pref_names.h"
#include "components/external_intents/android/jni_headers/InterceptNavigationDelegateImpl_jni.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
#include "components/navigation_interception/navigation_params.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "net/base/escape.h"
#include "url/gurl.h"

namespace external_intents {
namespace {

using navigation_interception::InterceptNavigationDelegate;
using navigation_interception::NavigationParams;

class BraveInterceptNavigationDelegate : public InterceptNavigationDelegate {
 public:
  BraveInterceptNavigationDelegate(JNIEnv* env,
                                   jobject jdelegate,
                                   PrefService* pref_service)
      : InterceptNavigationDelegate(env, jdelegate) {
    pref_service_ = pref_service;
  }

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

 private:
  bool ShouldPlayVideoInBrowser(const GURL& url) {
    if (!pref_service_) {
      NOTREACHED();
      return false;
    }

    if (!pref_service_->GetBoolean(kPlayYTVideoInBrowserEnabled)) {
      return false;
    }

    if (url.host().find("youtube.com") != std::string::npos ||
        url.host().find("youtu.be") != std::string::npos) {
      return true;
    }

    return false;
  }

  raw_ptr<PrefService> pref_service_ = nullptr;
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
      std::make_unique<BraveInterceptNavigationDelegate>(
          env, jdelegate,
          user_prefs::UserPrefs::Get(web_contents->GetBrowserContext())));
}

}  // namespace external_intents
