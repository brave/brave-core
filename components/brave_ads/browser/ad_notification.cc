/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/components/brave_ads/browser/ad_notification.h"
#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "brave/browser/version_info.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/message_center/public/cpp/notification.h"
#include "ui/message_center/public/cpp/notification_types.h"
#include "ui/message_center/public/cpp/notifier_id.h"
#include "chrome/browser/profiles/profile.h"

#if defined(OS_ANDROID)
#include "chrome/android/jni_headers/chrome/jni/BraveAds_jni.h"
// #include "jni/BraveAds_jni.h"
#include "chrome/browser/ui/android/tab_model/tab_model_list.h"
#include "chrome/browser/profiles/profile_android.h"
#include "base/android/jni_android.h"
#include "net/android/network_library.h"
#endif

namespace brave_ads {

namespace {

const char kNotifierIdPrefix[] = "service.ads_service.";
const char kNotifierId[] = "service.ads_service";

}  // namespace

using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;

// static
// (Albert Wang): Copied syntax from profile_android.cc
// void BraveAds::OnShowHelper(
void BraveAds::OnShowHelper(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android,
    jstring uuid) {

  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile_android);
  auto* ads_service_ = brave_ads::AdsServiceFactory::GetForProfile(profile);
  LOG(WARNING) << "albert got on show!";
  ads_service_->OnShow(profile, base::android::ConvertJavaStringToUTF8(env, uuid));
}

// static
void BraveAds::OnClickHelper(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android,
    jstring url,
    bool should_close) {

  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile_android);
  auto* ads_service_ = brave_ads::AdsServiceFactory::GetForProfile(profile);
  LOG(WARNING) << "albert got on click!" << base::android::ConvertJavaStringToUTF8(env, url);
  ads_service_->OpenSettings(profile, GURL(base::android::ConvertJavaStringToUTF8(env, url)), should_close);
}

// static
void BraveAds::OnDismissHelper(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_profile_android,
    jstring url,
    jstring uuid,
    bool dismissed_by_user) {

  Profile* profile = ProfileAndroid::FromProfileAndroid(j_profile_android);
  auto* ads_service_ = brave_ads::AdsServiceFactory::GetForProfile(profile);
  // (Albert Wang): What does OnceClosure() do??
  LOG(WARNING) << "albert got on dismiss!";
  ads_service_->OnClose(profile, GURL(base::android::ConvertJavaStringToUTF8(env, url)), base::android::ConvertJavaStringToUTF8(env, uuid), dismissed_by_user, base::OnceClosure());
}

// static
std::unique_ptr<message_center::Notification> CreateAdNotification(
    const ads::NotificationInfo& notification_info,
    std::string* notification_id) {
  *notification_id = kNotifierIdPrefix + notification_info.uuid;
  message_center::RichNotificationData notification_data;

  base::string16 advertiser;
  if (base::IsStringUTF8(notification_info.advertiser)) {
    base::UTF8ToUTF16(notification_info.advertiser.c_str(),
                      notification_info.advertiser.length(), &advertiser);
  }

  base::string16 text;
  if (base::IsStringUTF8(notification_info.text)) {
    base::UTF8ToUTF16(notification_info.text.c_str(),
                      notification_info.text.length(), &text);
  }

  // hack to prevent origin from showing in the notification
  // since we're using that to get the notification_id to OpenSettings
  notification_data.context_message = base::ASCIIToUTF16(" ");
  auto notification = std::make_unique<message_center::Notification>(
      message_center::NOTIFICATION_TYPE_SIMPLE,
      *notification_id,
      advertiser,
      text,
      gfx::Image(),
      base::string16(),
      GURL("chrome://brave_ads/?" + *notification_id),
      message_center::NotifierId(
          message_center::NotifierType::SYSTEM_COMPONENT,
          kNotifierId),
      notification_data,
      nullptr);
#if !defined(OS_MACOSX) || defined(OFFICIAL_BUILD)
  // set_never_timeout uses an XPC service which requires signing
  // so for now we don't set this for macos dev builds
  notification->set_never_timeout(true);
#endif

#if defined(OS_ANDROID)
  JNIEnv* env = base::android::AttachCurrentThread();
  base::android::ScopedJavaGlobalRef<jobject> java_obj_;
  java_obj_.Reset(env, Java_BraveAds_create(env, 0).obj());
  base::android::ScopedJavaLocalRef<jstring> jadvertiser = base::android::ConvertUTF8ToJavaString(env, notification_info.advertiser.c_str());
  base::android::ScopedJavaLocalRef<jstring> jtext = base::android::ConvertUTF8ToJavaString(env, notification_info.text.c_str());
  base::android::ScopedJavaLocalRef<jstring> jurl = base::android::ConvertUTF8ToJavaString(env, notification_info.url.c_str());
  base::android::ScopedJavaLocalRef<jstring> juuid = base::android::ConvertUTF8ToJavaString(env, notification_info.uuid.c_str());
  Java_BraveAds_showNotificationFromNative(env, java_obj_, jadvertiser, jtext, jurl, juuid);
#endif

  return notification;
}

}  // namespace brave_ads
