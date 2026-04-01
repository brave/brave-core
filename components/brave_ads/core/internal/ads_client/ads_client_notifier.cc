/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier.h"

#include <cstdint>
#include <memory>

#include "base/check.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/public/common/functional/once_closure_task_queue.h"
#include "ui/base/page_transition_types.h"
#include "url/gurl.h"

namespace brave_ads {

AdsClientNotifier::AdsClientNotifier()
    : task_queue_(std::make_unique<OnceClosureTaskQueue>()) {}

AdsClientNotifier::~AdsClientNotifier() = default;

void AdsClientNotifier::AddObserver(AdsClientNotifierObserver* const observer) {
  CHECK(observer);

  observers_.AddObserver(observer);
}

void AdsClientNotifier::RemoveObserver(
    AdsClientNotifierObserver* const observer) {
  CHECK(observer);

  observers_.RemoveObserver(observer);
}

void AdsClientNotifier::NotifyPendingObservers() {
  task_queue_->FlushAndStopQueueing();
}

void AdsClientNotifier::NotifyDidInitializeAds() {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyDidInitializeAds,
                       weak_factory_.GetWeakPtr()));
  }

  observers_.Notify(&AdsClientNotifierObserver::OnNotifyDidInitializeAds);
}

void AdsClientNotifier::NotifyRewardsWalletDidUpdate(
    const std::string& payment_id,
    const std::string& recovery_seed_base64) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(base::BindOnce(
        &AdsClientNotifier::NotifyRewardsWalletDidUpdate,
        weak_factory_.GetWeakPtr(), payment_id, recovery_seed_base64));
  }

  observers_.Notify(&AdsClientNotifierObserver::OnNotifyRewardsWalletDidUpdate,
                    payment_id, recovery_seed_base64);
}

void AdsClientNotifier::NotifyPrefDidChange(const std::string& path) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyPrefDidChange,
                       weak_factory_.GetWeakPtr(), path));
  }

  observers_.Notify(&AdsClientNotifierObserver::OnNotifyPrefDidChange, path);
}

void AdsClientNotifier::NotifyResourceComponentDidChange(
    const std::string& manifest_version,
    const std::string& id) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyResourceComponentDidChange,
                       weak_factory_.GetWeakPtr(), manifest_version, id));
  }

  observers_.Notify(
      &AdsClientNotifierObserver::OnNotifyResourceComponentDidChange,
      manifest_version, id);
}

void AdsClientNotifier::NotifyDidUnregisterResourceComponent(
    const std::string& id) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyDidUnregisterResourceComponent,
                       weak_factory_.GetWeakPtr(), id));
  }

  observers_.Notify(
      &AdsClientNotifierObserver::OnNotifyDidUnregisterResourceComponent, id);
}

void AdsClientNotifier::NotifyTabTextContentDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(base::BindOnce(
        &AdsClientNotifier::NotifyTabTextContentDidChange,
        weak_factory_.GetWeakPtr(), tab_id, redirect_chain, text));
  }

  observers_.Notify(&AdsClientNotifierObserver::OnNotifyTabTextContentDidChange,
                    tab_id, redirect_chain, text);
}

void AdsClientNotifier::NotifyTabDidStartPlayingMedia(int32_t tab_id) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyTabDidStartPlayingMedia,
                       weak_factory_.GetWeakPtr(), tab_id));
  }

  observers_.Notify(&AdsClientNotifierObserver::OnNotifyTabDidStartPlayingMedia,
                    tab_id);
}

void AdsClientNotifier::NotifyTabDidStopPlayingMedia(int32_t tab_id) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyTabDidStopPlayingMedia,
                       weak_factory_.GetWeakPtr(), tab_id));
  }

  observers_.Notify(&AdsClientNotifierObserver::OnNotifyTabDidStopPlayingMedia,
                    tab_id);
}

void AdsClientNotifier::NotifyTabDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    bool is_new_navigation,
    bool is_restoring,
    bool is_visible) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(base::BindOnce(
        &AdsClientNotifier::NotifyTabDidChange, weak_factory_.GetWeakPtr(),
        tab_id, redirect_chain, is_new_navigation, is_restoring, is_visible));
  }

  observers_.Notify(&AdsClientNotifierObserver::OnNotifyTabDidChange, tab_id,
                    redirect_chain, is_new_navigation, is_restoring,
                    is_visible);
}

void AdsClientNotifier::NotifyTabDidLoad(int32_t tab_id, int http_status_code) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(base::BindOnce(&AdsClientNotifier::NotifyTabDidLoad,
                                           weak_factory_.GetWeakPtr(), tab_id,
                                           http_status_code));
  }

  observers_.Notify(&AdsClientNotifierObserver::OnNotifyTabDidLoad, tab_id,
                    http_status_code);
}

void AdsClientNotifier::NotifyDidCloseTab(int32_t tab_id) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyDidCloseTab,
                       weak_factory_.GetWeakPtr(), tab_id));
  }

  observers_.Notify(&AdsClientNotifierObserver::OnNotifyDidCloseTab, tab_id);
}

void AdsClientNotifier::NotifyUserGestureEventTriggered(
    ui::PageTransition page_transition) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyUserGestureEventTriggered,
                       weak_factory_.GetWeakPtr(), page_transition));
  }

  observers_.Notify(
      &AdsClientNotifierObserver::OnNotifyUserGestureEventTriggered,
      page_transition);
}

void AdsClientNotifier::NotifyUserDidBecomeIdle() {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyUserDidBecomeIdle,
                       weak_factory_.GetWeakPtr()));
  }

  observers_.Notify(&AdsClientNotifierObserver::OnNotifyUserDidBecomeIdle);
}

void AdsClientNotifier::NotifyUserDidBecomeActive(base::TimeDelta idle_time,
                                                  bool screen_was_locked) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(base::BindOnce(
        &AdsClientNotifier::NotifyUserDidBecomeActive,
        weak_factory_.GetWeakPtr(), idle_time, screen_was_locked));
  }

  observers_.Notify(&AdsClientNotifierObserver::OnNotifyUserDidBecomeActive,
                    idle_time, screen_was_locked);
}

void AdsClientNotifier::NotifyBrowserDidEnterForeground() {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyBrowserDidEnterForeground,
                       weak_factory_.GetWeakPtr()));
  }

  observers_.Notify(
      &AdsClientNotifierObserver::OnNotifyBrowserDidEnterForeground);
}

void AdsClientNotifier::NotifyBrowserDidEnterBackground() {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyBrowserDidEnterBackground,
                       weak_factory_.GetWeakPtr()));
  }

  observers_.Notify(
      &AdsClientNotifierObserver::OnNotifyBrowserDidEnterBackground);
}

void AdsClientNotifier::NotifyBrowserDidBecomeActive() {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyBrowserDidBecomeActive,
                       weak_factory_.GetWeakPtr()));
  }

  observers_.Notify(&AdsClientNotifierObserver::OnNotifyBrowserDidBecomeActive);
}

void AdsClientNotifier::NotifyBrowserDidResignActive() {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyBrowserDidResignActive,
                       weak_factory_.GetWeakPtr()));
  }

  observers_.Notify(&AdsClientNotifierObserver::OnNotifyBrowserDidResignActive);
}

void AdsClientNotifier::NotifyDidSolveAdaptiveCaptcha() {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyDidSolveAdaptiveCaptcha,
                       weak_factory_.GetWeakPtr()));
  }

  observers_.Notify(
      &AdsClientNotifierObserver::OnNotifyDidSolveAdaptiveCaptcha);
}

}  // namespace brave_ads
