/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_ads/notifications/notification_ad_platform_bridge.h"

#include "base/memory/scoped_refptr.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/ui/brave_ads/notification_ad_delegate.h"
#include "brave/browser/ui/brave_ads/notification_ad_popup_handler.h"
#include "chrome/browser/platform_util.h"
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

// A NotificationAdDelegate that passes through events to the ads service
class PassThroughDelegate : public NotificationAdDelegate {
 public:
  PassThroughDelegate(Profile* profile, const NotificationAd& notification_ad)
      : profile_(profile), notification_ad_(notification_ad) {}

  void OnShow() override {
    AdsService* ads_service = AdsServiceFactory::GetForProfile(profile_);
    DCHECK(ads_service);

    ads_service->OnShowNotificationAd(notification_ad_.id());
  }

  void OnClose(const bool by_user) override {
    AdsService* ads_service = AdsServiceFactory::GetForProfile(profile_);
    DCHECK(ads_service);

    ads_service->OnCloseNotificationAd(notification_ad_.id(), by_user);
  }

  void OnClick() override {
    AdsService* ads_service = AdsServiceFactory::GetForProfile(profile_);
    DCHECK(ads_service);

    ads_service->OnClickNotificationAd(notification_ad_.id());
  }

 protected:
  ~PassThroughDelegate() override = default;

 private:
  Profile* profile_ = nullptr;  // NOT OWNED

  NotificationAd notification_ad_;

  PassThroughDelegate(const PassThroughDelegate&) = delete;
  PassThroughDelegate& operator=(const PassThroughDelegate&) = delete;
};

}  // namespace

NotificationAdPlatformBridge::NotificationAdPlatformBridge(Profile* profile)
    : profile_(profile) {
  DCHECK(profile_);
}

NotificationAdPlatformBridge::~NotificationAdPlatformBridge() = default;

void NotificationAdPlatformBridge::ShowNotificationAd(
    NotificationAd notification_ad) {
  // If there's no delegate, replace it with a PassThroughDelegate so clicks go
  // back to the appropriate handler
  notification_ad.set_delegate(
      base::WrapRefCounted(new PassThroughDelegate(profile_, notification_ad)));

  const gfx::NativeWindow browser_native_window = GetBrowserNativeWindow();
  const gfx::NativeView browser_native_view =
      platform_util::GetViewForWindow(browser_native_window);
  NotificationAdPopupHandler::Show(profile_, notification_ad,
                                   browser_native_window, browser_native_view);
}

void NotificationAdPlatformBridge::CloseNotificationAd(
    const std::string& notification_id) {
  NotificationAdPopupHandler::Close(notification_id, /* by_user */ false);
}

}  // namespace brave_ads
