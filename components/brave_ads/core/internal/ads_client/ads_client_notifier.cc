/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier.h"

#include <cstdint>
#include <memory>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/public/common/functional/once_closure_task_queue.h"
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

  for (auto& observer : observers_) {
    observer.OnNotifyDidInitializeAds();
  }
}

void AdsClientNotifier::NotifyRewardsWalletDidUpdate(
    const std::string& payment_id,
    const std::string& recovery_seed_base64) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(base::BindOnce(
        &AdsClientNotifier::NotifyRewardsWalletDidUpdate,
        weak_factory_.GetWeakPtr(), payment_id, recovery_seed_base64));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyRewardsWalletDidUpdate(payment_id, recovery_seed_base64);
  }
}

void AdsClientNotifier::NotifyLocaleDidChange(const std::string& locale) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyLocaleDidChange,
                       weak_factory_.GetWeakPtr(), locale));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyLocaleDidChange(locale);
  }
}

void AdsClientNotifier::NotifyPrefDidChange(const std::string& path) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyPrefDidChange,
                       weak_factory_.GetWeakPtr(), path));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyPrefDidChange(path);
  }
}

void AdsClientNotifier::NotifyResourceComponentDidChange(
    const std::string& manifest_version,
    const std::string& id) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyResourceComponentDidChange,
                       weak_factory_.GetWeakPtr(), manifest_version, id));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyResourceComponentDidChange(manifest_version, id);
  }
}

void AdsClientNotifier::NotifyDidUnregisterResourceComponent(
    const std::string& id) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyDidUnregisterResourceComponent,
                       weak_factory_.GetWeakPtr(), id));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyDidUnregisterResourceComponent(id);
  }
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

  for (auto& observer : observers_) {
    observer.OnNotifyTabTextContentDidChange(tab_id, redirect_chain, text);
  }
}

void AdsClientNotifier::NotifyTabHtmlContentDidChange(
    int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(base::BindOnce(
        &AdsClientNotifier::NotifyTabHtmlContentDidChange,
        weak_factory_.GetWeakPtr(), tab_id, redirect_chain, html));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyTabHtmlContentDidChange(tab_id, redirect_chain, html);
  }
}

void AdsClientNotifier::NotifyTabDidStartPlayingMedia(int32_t tab_id) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyTabDidStartPlayingMedia,
                       weak_factory_.GetWeakPtr(), tab_id));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyTabDidStartPlayingMedia(tab_id);
  }
}

void AdsClientNotifier::NotifyTabDidStopPlayingMedia(int32_t tab_id) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyTabDidStopPlayingMedia,
                       weak_factory_.GetWeakPtr(), tab_id));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyTabDidStopPlayingMedia(tab_id);
  }
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

  for (auto& observer : observers_) {
    observer.OnNotifyTabDidChange(tab_id, redirect_chain, is_new_navigation,
                                  is_restoring, is_visible);
  }
}

void AdsClientNotifier::NotifyTabDidLoad(int32_t tab_id, int http_status_code) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(base::BindOnce(&AdsClientNotifier::NotifyTabDidLoad,
                                           weak_factory_.GetWeakPtr(), tab_id,
                                           http_status_code));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyTabDidLoad(tab_id, http_status_code);
  }
}

void AdsClientNotifier::NotifyDidCloseTab(int32_t tab_id) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyDidCloseTab,
                       weak_factory_.GetWeakPtr(), tab_id));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyDidCloseTab(tab_id);
  }
}

void AdsClientNotifier::NotifyUserGestureEventTriggered(
    int32_t page_transition_type) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyUserGestureEventTriggered,
                       weak_factory_.GetWeakPtr(), page_transition_type));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyUserGestureEventTriggered(page_transition_type);
  }
}

void AdsClientNotifier::NotifyUserDidBecomeIdle() {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyUserDidBecomeIdle,
                       weak_factory_.GetWeakPtr()));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyUserDidBecomeIdle();
  }
}

void AdsClientNotifier::NotifyUserDidBecomeActive(base::TimeDelta idle_time,
                                                  bool screen_was_locked) {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(base::BindOnce(
        &AdsClientNotifier::NotifyUserDidBecomeActive,
        weak_factory_.GetWeakPtr(), idle_time, screen_was_locked));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyUserDidBecomeActive(idle_time, screen_was_locked);
  }
}

void AdsClientNotifier::NotifyBrowserDidEnterForeground() {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyBrowserDidEnterForeground,
                       weak_factory_.GetWeakPtr()));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyBrowserDidEnterForeground();
  }
}

void AdsClientNotifier::NotifyBrowserDidEnterBackground() {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyBrowserDidEnterBackground,
                       weak_factory_.GetWeakPtr()));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyBrowserDidEnterBackground();
  }
}

void AdsClientNotifier::NotifyBrowserDidBecomeActive() {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyBrowserDidBecomeActive,
                       weak_factory_.GetWeakPtr()));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyBrowserDidBecomeActive();
  }
}

void AdsClientNotifier::NotifyBrowserDidResignActive() {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyBrowserDidResignActive,
                       weak_factory_.GetWeakPtr()));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyBrowserDidResignActive();
  }
}

void AdsClientNotifier::NotifyDidSolveAdaptiveCaptcha() {
  if (task_queue_->should_queue()) {
    return task_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyDidSolveAdaptiveCaptcha,
                       weak_factory_.GetWeakPtr()));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyDidSolveAdaptiveCaptcha();
  }
}

}  // namespace brave_ads
