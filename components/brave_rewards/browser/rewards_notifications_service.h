/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATIONS_SERVICE_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATIONS_SERVICE_

#include <memory>

#include "base/macros.h"
#include "base/observer_list.h"
#include "components/keyed_service/core/keyed_service.h"

namespace brave_rewards {

class RewardsNotificationsServiceObserver;

class RewardsNotificationsService : public KeyedService {
 public:
  RewardsNotificationsService();
  ~RewardsNotificationsService() override;

  typedef std::string RewardsNotificationID;
  typedef uint64_t RewardsNotificationTimestamp;
  typedef std::vector<std::string> RewardsNotificationArgs;

  enum RewardsNotificationType {
    REWARDS_NOTIFICATION_INVALID = 0,
    REWARDS_NOTIFICATION_AUTO_CONTRIBUTE,
    REWARDS_NOTIFICATION_GRANT,
    REWARDS_NOTIFICATION_FAILED_CONTRIBUTION,
    REWARDS_NOTIFICATION_IMPENDING_CONTRIBUTION,
    REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS,
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
    RewardsNotificationType type_ = RewardsNotificationsService::REWARDS_NOTIFICATION_INVALID;
    RewardsNotificationTimestamp timestamp_ = 0;
    RewardsNotificationArgs args_;
  };

  typedef std::vector<RewardsNotification> RewardsNotificationsList;

  virtual void AddNotification(RewardsNotificationType type,
                               RewardsNotificationArgs args,
                               RewardsNotificationID id = "") = 0;
  virtual void DeleteNotification(RewardsNotificationID id) = 0;
  virtual void DeleteAllNotifications() = 0;
  virtual void GetNotification(RewardsNotificationID id) = 0;
  virtual void GetAllNotifications() = 0;

  void AddObserver(RewardsNotificationsServiceObserver* observer);
  void RemoveObserver(RewardsNotificationsServiceObserver* observer);

 protected:
  base::ObserverList<RewardsNotificationsServiceObserver> observers_;

 private:
  DISALLOW_COPY_AND_ASSIGN(RewardsNotificationsService);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_REWARDS_NOTIFICATIONS_SERVICE_
