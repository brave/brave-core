/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_notification_handler.h"

#include <memory>
#include <utility>

#include "brave/components/brave_ads/browser/ads_service_impl.h"
#include "content/public/browser/browser_context.h"

#if defined(OS_ANDROID)
#include "base/android/application_status_listener.h"
#include "chrome/browser/lifetime/application_lifetime_android.h"
#endif

namespace brave_ads {

int kUserDataKey;  // The value is not important, the address is a key.

AdsNotificationHandler::AdsNotificationHandler(
    content::BrowserContext* browser_context)
    : browser_context_(browser_context), ads_service_(nullptr) {
  DCHECK(browser_context);
  browser_context_->SetUserData(UserDataKey(),
                                std::make_unique<UnownedPointer>(this));
}

AdsNotificationHandler::~AdsNotificationHandler() {
  browser_context_->RemoveUserData(UserDataKey());
}

void AdsNotificationHandler::OnShow(
    Profile* profile,
    const std::string& id) {

  if (!ads_service_) {
    auto notification = base::BindOnce(&AdsNotificationHandler::OnShow,
                                       base::Unretained(this), profile, id);
    pending_notifications_.push(std::move(notification));
    return;
  }

#if defined (OS_ANDROID)
  headless_shutdown_timer_.Stop();
#endif

  ads_service_->OnShow(profile, id);
}

void AdsNotificationHandler::OnClose(
    Profile* profile,
    const GURL& origin,
    const std::string& id,
    bool by_user,
    base::OnceClosure completed_closure) {

  base::OnceClosure completed_closure_local =
      base::BindOnce(&AdsNotificationHandler::CloseOperationCompleted,
      base::Unretained(this), id);
  pending_close_callbacks_.emplace(id, std::move(completed_closure));

  if (!ads_service_) {
    auto notification = base::BindOnce(
        &AdsNotificationHandler::OnClose, base::Unretained(this), profile,
        origin, id, by_user, std::move(completed_closure_local));
    pending_notifications_.push(std::move(notification));
    return;
  }

  #if defined (OS_ANDROID)
    last_dismissed_notification_id_ = id;
    headless_shutdown_timer_.Stop();
  #endif

  ads_service_->OnClose(profile, origin, id, by_user,
                        std::move(completed_closure_local));
}

void AdsNotificationHandler::OnClick(
    Profile* profile,
    const GURL& origin,
    const std::string& id,
    const base::Optional<int>& action_index,
    const base::Optional<base::string16>& reply,
    base::OnceClosure completed_closure) {

  if (!ads_service_) {
    auto notification = base::BindOnce(
        &AdsNotificationHandler::OnClick, base::Unretained(this), profile,
        origin, id, action_index, reply, std::move(completed_closure));
    pending_notifications_.push(std::move(notification));
    return;
  }

#if defined (OS_ANDROID)
  headless_shutdown_timer_.Stop();
#endif

  ads_service_->ViewAdNotification(id);
}

void AdsNotificationHandler::DisableNotifications(
    Profile* profile,
    const GURL& origin) {

#if defined (OS_ANDROID)
  headless_shutdown_timer_.Stop();
#endif
}

void AdsNotificationHandler::OpenSettings(
      Profile* profile,
      const GURL& origin) {
  DCHECK(origin.has_query());
  auto id = origin.query();

  if (!ads_service_) {
    auto notification = base::BindOnce(&AdsNotificationHandler::OpenSettings,
                                       base::Unretained(this), profile, origin);
    pending_notifications_.push(std::move(notification));
    return;
  }

#if defined (OS_ANDROID)
  headless_shutdown_timer_.Stop();
#endif

  ads_service_->ViewAdNotification(id);
}

void AdsNotificationHandler::SetAdsService(
    brave_ads::AdsServiceImpl* ads_service) {
  if (ads_service) {
    ads_service_ = ads_service;
    SendPendingNotifications();
  } else {
    ads_service = nullptr;
  }
}

void AdsNotificationHandler::SendPendingNotifications() {
  // Flush any pending notification events that have yet to execute.
  while (!pending_notifications_.empty()) {
    std::move(pending_notifications_.front()).Run();
    pending_notifications_.pop();
  }
}

// static
const void* AdsNotificationHandler::UserDataKey() {
  return &kUserDataKey;
}

void AdsNotificationHandler::CloseOperationCompleted(
    const std::string& notification_id) {

  auto iter = pending_close_callbacks_.find(notification_id);
  if (iter != pending_close_callbacks_.end()) {
    std::move(iter->second).Run();
    pending_close_callbacks_.erase(iter);
  }
#if defined(OS_ANDROID)
  StartShutDownTimerIfNecessary(notification_id);
#endif
}

#if defined(OS_ANDROID)
bool AdsNotificationHandler::IsHeadless() {
  using base::android::ApplicationState;
  auto state = base::android::ApplicationStatusListener::GetState();
  bool headless = (state == ApplicationState::APPLICATION_STATE_UNKNOWN ||
      state == ApplicationState::APPLICATION_STATE_HAS_DESTROYED_ACTIVITIES);
  return headless;
}

void AdsNotificationHandler::StartShutDownTimerIfNecessary(
    const std::string & last_processed_notification_id) {
  if (IsHeadless() && last_dismissed_notification_id_ ==
      last_processed_notification_id) {
    last_dismissed_notification_id_ = "";
    // wait and close the browser if running headless
    headless_shutdown_timer_.Start(FROM_HERE,
        base::TimeDelta::FromSeconds(kWaitBeforeShutdownWhenRunHeadless),
        this, &AdsNotificationHandler::ShutdownTimerCallback);
  }
}

// Check if browser is runnning without UI and close it.
// If during shutdown a new notification event arrives-
// timer is stopped in corresponding handler function
void AdsNotificationHandler::ShutdownTimerCallback() {
  if (IsHeadless()) {
    chrome::TerminateAndroid();
  }
}
#endif
}  // namespace brave_ads
