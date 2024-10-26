/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NOTIFICATIONS_ADS_NOTIFICATION_HANDLER_H_
#define BRAVE_BROWSER_NOTIFICATIONS_ADS_NOTIFICATION_HANDLER_H_

#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ref.h"
#include "chrome/browser/notifications/notification_handler.h"

class GURL;
class Profile;

namespace brave_ads {

class AdsNotificationHandler : public NotificationHandler {
 public:
  explicit AdsNotificationHandler(Profile& profile);

  AdsNotificationHandler(const AdsNotificationHandler&) = delete;
  AdsNotificationHandler& operator=(const AdsNotificationHandler&) = delete;
  AdsNotificationHandler(AdsNotificationHandler&&) = delete;
  AdsNotificationHandler& operator=(AdsNotificationHandler&&) = delete;

  ~AdsNotificationHandler() override;

  // NotificationHandler:
  void OnShow(Profile* profile, const std::string& id) override;
  void OnClose(Profile* profile,
               const GURL& origin,
               const std::string& id,
               bool by_user,
               base::OnceClosure completed_closure) override;
  void OnClick(Profile* profile,
               const GURL& origin,
               const std::string& id,
               const std::optional<int>& action_index,
               const std::optional<std::u16string>& reply,
               base::OnceClosure completed_closure) override;
  void OpenSettings(Profile* profile, const GURL& origin) override;

 private:
  raw_ref<Profile> profile_;

  bool did_click_notification_ad_ = false;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_NOTIFICATIONS_ADS_NOTIFICATION_HANDLER_H_
