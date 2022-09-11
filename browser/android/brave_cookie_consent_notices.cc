/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/android/brave_cookie_consent_notices.h"

#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "brave/build/android/jni_headers/BraveCookieConsentNotices_jni.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "url/gurl.h"

namespace chrome {
namespace android {

BraveCookieConsentNotices::BraveCookieConsentNotices(
    JNIEnv* env,
    const base::android::JavaRef<jobject>& obj)
    : jobj_(base::android::ScopedJavaGlobalRef<jobject>(obj)) {
  Java_BraveCookieConsentNotices_setNativePtr(env, obj,
                                              reinterpret_cast<intptr_t>(this));
}

BraveCookieConsentNotices::~BraveCookieConsentNotices() {}

void BraveCookieConsentNotices::Destroy(JNIEnv* env) {
  delete this;
}

void BraveCookieConsentNotices::EnableFilter(JNIEnv* env) {
  g_brave_browser_process->ad_block_service()
      ->regional_service_manager()
      ->EnableFilterList(brave_shields::kCookieListUuid, true);
}

bool BraveCookieConsentNotices::IsFilterListAvailable(JNIEnv* env) {
  return g_brave_browser_process->ad_block_service()
      ->regional_service_manager()
      ->IsFilterListAvailable(brave_shields::kCookieListUuid);
}

static void JNI_BraveCookieConsentNotices_Init(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& jcaller) {
  new BraveCookieConsentNotices(env, jcaller);
}

}  // namespace android
}  // namespace chrome
