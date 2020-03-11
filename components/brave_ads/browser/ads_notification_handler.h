/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_NOTIFICATION_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_NOTIFICATION_HANDLER_H_

#include <queue>
#include <string>
#include <utility>

#include "base/callback.h"
#include "base/containers/queue.h"
#include "base/memory/weak_ptr.h"
#include "base/supports_user_data.h"
#include "chrome/browser/notifications/notification_handler.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace brave_ads {

class AdsServiceImpl;

const void* const kAdsNotificationHandlerKey = &kAdsNotificationHandlerKey;

class AdsNotificationHandler : public NotificationHandler {
 public:
  explicit AdsNotificationHandler(content::BrowserContext* browser_context);
  ~AdsNotificationHandler() override;

  // NotificationHandler implementation
  void OnShow(
      Profile* profile,
      const std::string& id) override;
  void OnClose(
      Profile* profile,
      const GURL& origin,
      const std::string& id,
      bool by_user,
      base::OnceClosure completed_closure) override;
  void OnClick(
      Profile* profile,
      const GURL& origin,
      const std::string& id,
      const base::Optional<int>& action_index,
      const base::Optional<base::string16>& reply,
      base::OnceClosure completed_closure) override;
  void DisableNotifications(
      Profile* profile,
      const GURL& origin) override;
  void OpenSettings(
      Profile* profile,
      const GURL& origin) override;

  void SetAdsService(brave_ads::AdsServiceImpl* ads_service);

  static const void* UserDataKey();

  // This class is used to set the AdsNotificationHandler as user data
  // on a BrowserContext. This is used instead of AdsNotificationHandler
  // directly because SetUserData requires a std::unique_ptr. This is
  // safe because we remove the user data in AdsNotificationHandler's
  // destructor.
  class UnownedPointer : public base::SupportsUserData::Data {
   public:
    explicit UnownedPointer(AdsNotificationHandler* pointer)
        : pointer_(pointer) {}

    AdsNotificationHandler* get() const { return pointer_; }

   private:
    AdsNotificationHandler* const pointer_;

    DISALLOW_COPY_AND_ASSIGN(UnownedPointer);
  };

 private:
  void SendPendingNotifications();

  content::BrowserContext* browser_context_;
  brave_ads::AdsServiceImpl* ads_service_;
  base::queue<base::OnceClosure> pending_notifications_;

  DISALLOW_COPY_AND_ASSIGN(AdsNotificationHandler);
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_NOTIFICATION_HANDLER_H_
