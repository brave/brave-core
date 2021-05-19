/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/notifications/platform_bridge.h"

#include <memory>
#include <string>

#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/task/post_task.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/brave_ads/browser/ads_notification_handler.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_impl.h"
#include "brave/ui/brave_ads/message_popup_view.h"
#include "brave/ui/brave_ads/public/cpp/notification.h"

#if defined(OS_ANDROID)
#include "base/android/jni_android.h"
#include "base/android/jni_array.h"
#include "base/android/jni_string.h"
#include "brave/browser/brave_ads/android/jni_headers/BraveAdsNotificationDialog_jni.h"
using base::android::AttachCurrentThread;
using base::android::ConvertJavaStringToUTF16;
using base::android::ConvertJavaStringToUTF8;
using base::android::ConvertUTF16ToJavaString;
using base::android::ConvertUTF8ToJavaString;
using base::android::JavaParamRef;
using base::android::ScopedJavaLocalRef;
#endif

namespace {

class PassThroughDelegate : public brave_ads::NotificationDelegate {
 public:
  PassThroughDelegate(Profile* profile,
                      const brave_ads::Notification& notification)
      : profile_(profile), notification_(notification) {}

  void Close(bool by_user) override {
    std::unique_ptr<brave_ads::AdsNotificationHandler> handler =
        std::make_unique<brave_ads::AdsNotificationHandler>(
            static_cast<content::BrowserContext*>(profile_));
    handler->SetAdsService(static_cast<brave_ads::AdsServiceImpl*>(
        brave_ads::AdsServiceFactory::GetForProfile(profile_)));
    handler->OnClose(profile_, notification_.origin_url(), notification_.id(),
                     by_user, base::OnceClosure());
  }

  void Click(const base::Optional<int>& button_index,
             const base::Optional<std::u16string>& reply) override {
    std::unique_ptr<brave_ads::AdsNotificationHandler> handler =
        std::make_unique<brave_ads::AdsNotificationHandler>(
            static_cast<content::BrowserContext*>(profile_));
    handler->SetAdsService(static_cast<brave_ads::AdsServiceImpl*>(
        brave_ads::AdsServiceFactory::GetForProfile(profile_)));
    handler->OnClick(profile_, notification_.origin_url(), notification_.id(),
                     button_index, reply, base::OnceClosure());
  }

 protected:
  ~PassThroughDelegate() override = default;

 private:
  Profile* profile_;
  brave_ads::Notification notification_;

  DISALLOW_COPY_AND_ASSIGN(PassThroughDelegate);
};

}  // namespace

PlatformBridge::PlatformBridge(Profile* profile) : profile_(profile) {}

PlatformBridge::~PlatformBridge() {}

void PlatformBridge::Display(
    Profile* profile,
    const std::unique_ptr<brave_ads::Notification>& notification) {
  DCHECK_EQ(profile, profile_);

  // If there's no delegate, replace it with a PassThroughDelegate so clicks
  // go back to the appropriate handler.
  notification->set_delegate(
      base::WrapRefCounted(new PassThroughDelegate(profile_, *notification)));

#if defined(OS_WIN) || defined(OS_MAC) || defined(OS_LINUX)
  brave_ads::MessagePopupView::Show(*notification);
#elif defined(OS_ANDROID)
  ShowAndroidAdsNotification(profile, notification);
#endif

  std::unique_ptr<brave_ads::AdsNotificationHandler> handler =
      std::make_unique<brave_ads::AdsNotificationHandler>(
          static_cast<content::BrowserContext*>(profile));
  handler->SetAdsService(static_cast<brave_ads::AdsServiceImpl*>(
      brave_ads::AdsServiceFactory::GetForProfile(profile_)));
  handler->OnShow(profile_, notification->id());
}

#if defined(OS_ANDROID)
void PlatformBridge::ShowAndroidAdsNotification(
    Profile* profile,
    const std::unique_ptr<brave_ads::Notification>& notification) {
  GURL origin_url(notification->origin_url().GetOrigin());

  JNIEnv* env = AttachCurrentThread();

  base::android::ScopedJavaLocalRef<jstring> j_notification_id =
      ConvertUTF8ToJavaString(env, notification->id());
  base::android::ScopedJavaLocalRef<jstring> j_origin =
      ConvertUTF8ToJavaString(env, origin_url.spec());
  base::android::ScopedJavaLocalRef<jstring> title =
      ConvertUTF16ToJavaString(env, notification->title());
  base::android::ScopedJavaLocalRef<jstring> body =
      ConvertUTF16ToJavaString(env, notification->message());

  Java_BraveAdsNotificationDialog_displayAdsNotification(env, j_notification_id,
                                                         j_origin, title, body);
}

void PlatformBridge::CloseAndroidAdsNotification(
    Profile* profile,
    const std::string& notification_id) {
  JNIEnv* env = AttachCurrentThread();

  ScopedJavaLocalRef<jstring> j_notification_id =
      ConvertUTF8ToJavaString(env, notification_id);

  Java_BraveAdsNotificationDialog_closeAdsNotification(env, j_notification_id);
}
#endif

void PlatformBridge::Close(Profile* profile,
                           const std::string& notification_id) {
#if defined(OS_WIN) || defined(OS_MAC) || defined(OS_LINUX)
  brave_ads::MessagePopupView::ClosePopup(false);
#elif defined(OS_ANDROID)
  PlatformBridge::CloseAndroidAdsNotification(profile, notification_id);
#endif
}
