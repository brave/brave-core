/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/strings/escape.h"
#include "brave/components/constants/pref_names.h"
#include "components/external_intents/android/jni_headers/InterceptNavigationDelegateImpl_jni.h"
#include "components/navigation_interception/intercept_navigation_delegate.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

#define JNI_InterceptNavigationDelegateImpl_AssociateWithWebContents        \
  JNI_InterceptNavigationDelegateImpl_AssociateWithWebContents_ChromiumImpl \
      [[maybe_unused]]

#include "src/components/external_intents/android/intercept_navigation_delegate_impl.cc"

#undef JNI_InterceptNavigationDelegateImpl_AssociateWithWebContents

namespace external_intents {
namespace {

using navigation_interception::InterceptNavigationDelegate;

class BraveInterceptNavigationDelegate : public InterceptNavigationDelegate {
 public:
  BraveInterceptNavigationDelegate(JNIEnv* env,
                                   const jni_zero::JavaRef<jobject>& jdelegate,
                                   PrefService* pref_service)
      : InterceptNavigationDelegate(env, jdelegate) {
    pref_service_ = pref_service;
  }

  bool ShouldIgnoreNavigation(
      content::NavigationHandle* navigation_handle) override {
    if (ShouldPlayVideoInBrowser(GURL(base::EscapeExternalHandlerValue(
            navigation_handle->GetURL().spec()))))
      return false;

    return InterceptNavigationDelegate::ShouldIgnoreNavigation(
        navigation_handle);
  }

 private:
  bool ShouldPlayVideoInBrowser(const GURL& url) {
    if (!pref_service_ ||
        !pref_service_->GetBoolean(kPlayYTVideoInBrowserEnabled)) {
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
