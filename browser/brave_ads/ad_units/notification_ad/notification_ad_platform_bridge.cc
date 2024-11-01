/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/ad_units/notification_ad/notification_ad_platform_bridge.h"

#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/ui/brave_ads/notification_ad_delegate.h"
#include "brave/browser/ui/brave_ads/notification_ad_popup_handler.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "ui/gfx/native_widget_types.h"

namespace brave_ads {

namespace {

gfx::NativeWindow GetBrowserNativeWindow() {
  gfx::NativeWindow browser_native_window = gfx::NativeWindow();

  if (Browser* last_active_browser = chrome::FindLastActive()) {
    if (BrowserWindow* browser_window = last_active_browser->window()) {
      browser_native_window = browser_window->GetNativeWindow();
    }
  }

  return browser_native_window;
}

// A NotificationAdDelegate that passes through events to the ads service
class PassThroughDelegate : public NotificationAdDelegate {
 public:
  PassThroughDelegate(Profile& profile, const NotificationAd& notification_ad)
      : profile_(profile), notification_ad_(notification_ad) {}

  PassThroughDelegate(const PassThroughDelegate&) = delete;

  PassThroughDelegate& operator=(const PassThroughDelegate&) = delete;

  void OnShow() override {
    AdsService* ads_service = AdsServiceFactory::GetForProfile(&*profile_);
    CHECK(ads_service);

    ads_service->OnNotificationAdShown(notification_ad_.id());
  }

  void OnClose(const bool by_user) override {
    AdsService* ads_service = AdsServiceFactory::GetForProfile(&*profile_);
    CHECK(ads_service);

    ads_service->OnNotificationAdClosed(notification_ad_.id(), by_user);
  }

  void OnClick() override {
    AdsService* ads_service = AdsServiceFactory::GetForProfile(&*profile_);
    CHECK(ads_service);

    ads_service->OnNotificationAdClicked(notification_ad_.id());
  }

 protected:
  ~PassThroughDelegate() override = default;

 private:
  const raw_ref<Profile> profile_;

  NotificationAd notification_ad_;
};

}  // namespace

NotificationAdPlatformBridge::NotificationAdPlatformBridge(Profile& profile)
    : profile_(profile) {}

NotificationAdPlatformBridge::~NotificationAdPlatformBridge() = default;

void NotificationAdPlatformBridge::ShowNotificationAd(
    NotificationAd notification_ad) {
  // If there's no delegate, replace it with a PassThroughDelegate so clicks go
  // back to the appropriate handler
  notification_ad.set_delegate(base::WrapRefCounted(
      new PassThroughDelegate(*profile_, notification_ad)));

  const gfx::NativeWindow browser_native_window = GetBrowserNativeWindow();
  const gfx::NativeView browser_native_view =
      platform_util::GetViewForWindow(browser_native_window);
  NotificationAdPopupHandler::Show(*profile_, notification_ad,
                                   browser_native_window, browser_native_view);
}

void NotificationAdPlatformBridge::CloseNotificationAd(
    const std::string& notification_id) {
  NotificationAdPopupHandler::Close(notification_id, /*by_user*/ false);
}

}  // namespace brave_ads
