// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/browser/notifications/notification_platform_bridge_brave_custom_notification.h"

#include "base/run_loop.h"
#include "base/bind.h"
#include "base/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/post_task.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/notifications/notification_display_service_impl.h"
#include "chrome/browser/notifications/notification_ui_manager.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "brave/ui/brave_custom_notification/public/cpp/notification.h"
#include "brave/ui/brave_custom_notification/message_popup_view.h"
#include "chrome/browser/profiles/profile.h"
#include "brave/components/brave_ads/browser/ads_notification_handler.h"

namespace {

// A NotificationDelegate that passes through click actions to the notification
// display service (and on to the appropriate handler). This is a temporary
// class to ease the transition from NotificationDelegate to
// NotificationHandler.
// TODO(estade): also handle other NotificationDelegate actions as needed.
class PassThroughDelegate : public brave_custom_notification::NotificationDelegate {
 public:
  PassThroughDelegate(Profile* profile,
                      const brave_custom_notification::Notification& notification,
                      NotificationHandler::Type notification_type)
      : profile_(profile),
        notification_(notification),
        notification_type_(notification_type) {
    DCHECK_NE(notification_type, NotificationHandler::Type::TRANSIENT);
  }

  void SettingsClick() override {
    NotificationDisplayServiceImpl::GetForProfile(profile_)
        ->ProcessNotificationOperation(
            NotificationCommon::OPERATION_SETTINGS, notification_type_,
            notification_.origin_url(), notification_.id(), base::nullopt,
            base::nullopt, base::nullopt /* by_user */);
  }

  void DisableNotification() override {
    NotificationDisplayServiceImpl::GetForProfile(profile_)
        ->ProcessNotificationOperation(
            NotificationCommon::OPERATION_DISABLE_PERMISSION,
            notification_type_, notification_.origin_url(), notification_.id(),
            base::nullopt /* action_index */, base::nullopt /* reply */,
            base::nullopt /* by_user */);
  }

  void Close(bool by_user) override {
    brave_ads::AdsNotificationHandler* handler = new brave_ads::AdsNotificationHandler(static_cast<content::BrowserContext*>(profile_));
    handler->OnClose(profile_, notification_.origin_url(), notification_.id(), by_user, base::OnceClosure());
  }

  void Click(const base::Optional<int>& button_index,
             const base::Optional<base::string16>& reply) override {
    brave_ads::AdsNotificationHandler* handler = new brave_ads::AdsNotificationHandler(static_cast<content::BrowserContext*>(profile_));
    handler->OnClick(profile_, notification_.origin_url(), notification_.id(), button_index, reply, base::OnceClosure());
  }

 protected:
  ~PassThroughDelegate() override = default;

 private:
  Profile* profile_;
  brave_custom_notification::Notification notification_;
  NotificationHandler::Type notification_type_;

  DISALLOW_COPY_AND_ASSIGN(PassThroughDelegate);
};

}  // namespace

NotificationPlatformBridgeBraveCustomNotification::
    NotificationPlatformBridgeBraveCustomNotification(Profile* profile)
    : profile_(profile) {}

NotificationPlatformBridgeBraveCustomNotification::
    ~NotificationPlatformBridgeBraveCustomNotification() = default;

// Albert: Need to keep this as this overrides the parent class
void NotificationPlatformBridgeBraveCustomNotification::Display(
    NotificationHandler::Type notification_type,
    Profile* profile,
    const message_center::Notification& notification,
    std::unique_ptr<NotificationCommon::Metadata> /* metadata */) {}

void NotificationPlatformBridgeBraveCustomNotification::Display(
    NotificationHandler::Type notification_type,
    Profile* profile,
    brave_custom_notification::Notification& notification,
    std::unique_ptr<NotificationCommon::Metadata> /* metadata */) {
  DCHECK_EQ(profile, profile_);
  LOG(INFO) << "albert NPBBCN::DISPLAY";

  // If there's no delegate, replace it with a PassThroughDelegate so clicks
  // go back to the appropriate handler.
  brave_custom_notification::Notification notification_with_delegate(notification);
  notification_with_delegate.set_delegate(base::WrapRefCounted(
      new PassThroughDelegate(profile_, notification, notification_type)));
  brave_custom_notification::MessagePopupView::Show(notification_with_delegate);
  brave_ads::AdsNotificationHandler* handler = new brave_ads::AdsNotificationHandler(static_cast<content::BrowserContext*>(profile));
  handler->OnShow(profile_, notification.id());
}

void NotificationPlatformBridgeBraveCustomNotification::Close(
    Profile* profile,
    const std::string& notification_id) {
  /*
  DCHECK_EQ(profile, profile_);

  NotificationUIManager* ui_manager =
      g_browser_process->notification_ui_manager();
  if (!ui_manager)
    return;  // the process is shutting down

  ui_manager->CancelById(notification_id,
                         NotificationUIManager::GetProfileID(profile_));
                         */
}

void NotificationPlatformBridgeBraveCustomNotification::GetDisplayed(
    Profile* profile,
    GetDisplayedNotificationsCallback callback) const {
  return;
}

void NotificationPlatformBridgeBraveCustomNotification::SetReadyCallback(
    NotificationBridgeReadyCallback callback) {
  std::move(callback).Run(true /* success */);
}

void NotificationPlatformBridgeBraveCustomNotification::DisplayServiceShutDown(
    Profile* profile) {}
