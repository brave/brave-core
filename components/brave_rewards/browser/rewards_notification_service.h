/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATION_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATION_SERVICE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "base/observer_list.h"

namespace brave_rewards {

class RewardsNotificationServiceObserver;

class RewardsNotificationService {
 public:
  RewardsNotificationService();
  virtual ~RewardsNotificationService();

  typedef std::string RewardsNotificationID;
  typedef uint64_t RewardsNotificationTimestamp;
  typedef std::vector<std::string> RewardsNotificationArgs;

  enum RewardsNotificationType {
    REWARDS_NOTIFICATION_INVALID = 0,
    REWARDS_NOTIFICATION_AUTO_CONTRIBUTE,
    REWARDS_NOTIFICATION_GRANT,
    REWARDS_NOTIFICATION_GRANT_ADS,
    REWARDS_NOTIFICATION_FAILED_CONTRIBUTION,
    REWARDS_NOTIFICATION_IMPENDING_CONTRIBUTION,
    REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS,
    REWARDS_NOTIFICATION_BACKUP_WALLET,
    REWARDS_NOTIFICATION_TIPS_PROCESSED,
    REWARDS_NOTIFICATION_ADS_LAUNCH,
  };

  struct RewardsNotification {
    RewardsNotification();
    RewardsNotification(const RewardsNotification& prop);
    RewardsNotification(RewardsNotificationID id,
                        RewardsNotificationType type,
                        RewardsNotificationTimestamp timestamp,
                        RewardsNotificationArgs args);
    ~RewardsNotification();
    RewardsNotificationID id_;
    RewardsNotificationType type_ = REWARDS_NOTIFICATION_INVALID;
    RewardsNotificationTimestamp timestamp_ = 0;
    RewardsNotificationArgs args_;
  };

  typedef std::vector<RewardsNotification> RewardsNotificationsList;
  typedef std::map<RewardsNotificationID, RewardsNotification>
      RewardsNotificationsMap;

  virtual void AddNotification(RewardsNotificationType type,
                               RewardsNotificationArgs args,
                               RewardsNotificationID id = "",
                               bool only_once = false) = 0;
  virtual void DeleteNotification(RewardsNotificationID id) = 0;
  virtual void DeleteAllNotifications() = 0;
  virtual void GetNotification(RewardsNotificationID id) = 0;
  virtual void GetNotifications() = 0;
  virtual const RewardsNotificationsMap& GetAllNotifications() const = 0;
  virtual void ReadRewardsNotificationsJSON() = 0;
  virtual void StoreRewardsNotifications() = 0;

  void AddObserver(RewardsNotificationServiceObserver* observer);
  void RemoveObserver(RewardsNotificationServiceObserver* observer);

 protected:
  base::ObserverList<RewardsNotificationServiceObserver> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RewardsNotificationService);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATION_SERVICE_H_
