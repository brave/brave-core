/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/notifications/ad_notification_platform_bridge.h"

#include "base/memory/scoped_refptr.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/ui/brave_ads/ad_notification_delegate.h"
#include "brave/browser/ui/brave_ads/ad_notification_popup_handler.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "ui/gfx/native_widget_types.h"

namespace brave_ads {

namespace {

gfx::NativeWindow GetBrowserNativeWindow() {
  gfx::NativeWindow browser_native_window = gfx::kNullNativeWindow;

  if (Browser* last_active_browser = chrome::FindLastActive()) {
    if (BrowserWindow* browser_window = last_active_browser->window()) {
      browser_native_window = browser_window->GetNativeWindow();
    }
  }

  return browser_native_window;
}

// An AdNotificationDelegate that passes through events to the ads service
class PassThroughDelegate : public AdNotificationDelegate {
 public:
  PassThroughDelegate(Profile* profile, const AdNotification& ad_notification)
      : profile_(profile), ad_notification_(ad_notification) {}

  void OnShow() override {
    AdsService* ads_service = AdsServiceFactory::GetForProfile(profile_);
    DCHECK(ads_service);

    ads_service->OnShowAdNotification(ad_notification_.id());
  }

  void OnClose(const bool by_user) override {
    AdsService* ads_service = AdsServiceFactory::GetForProfile(profile_);
    DCHECK(ads_service);

    ads_service->OnCloseAdNotification(ad_notification_.id(), by_user);
  }

  void OnClick() override {
    AdsService* ads_service = AdsServiceFactory::GetForProfile(profile_);
    DCHECK(ads_service);

    ads_service->OnClickAdNotification(ad_notification_.id());
  }

 protected:
  ~PassThroughDelegate() override = default;

 private:
  Profile* profile_ = nullptr;  // NOT OWNED

  AdNotification ad_notification_;

  PassThroughDelegate(const PassThroughDelegate&) = delete;
  PassThroughDelegate& operator=(const PassThroughDelegate&) = delete;
};

}  // namespace

AdNotificationPlatformBridge::AdNotificationPlatformBridge(Profile* profile)
    : profile_(profile) {
  DCHECK(profile_);
}

AdNotificationPlatformBridge::~AdNotificationPlatformBridge() = default;

void AdNotificationPlatformBridge::ShowAdNotification(
    AdNotification ad_notification) {
  // If there's no delegate, replace it with a PassThroughDelegate so clicks go
  // back to the appropriate handler
  ad_notification.set_delegate(
      base::WrapRefCounted(new PassThroughDelegate(profile_, ad_notification)));

  const gfx::NativeWindow browser_native_window = GetBrowserNativeWindow();
  AdNotificationPopupHandler::Show(profile_, ad_notification,
                                   browser_native_window);
}

void AdNotificationPlatformBridge::CloseAdNotification(
    const std::string& notification_id) {
  AdNotificationPopupHandler::Close(notification_id, /* by_user */ false);
}

}  // namespace brave_ads
