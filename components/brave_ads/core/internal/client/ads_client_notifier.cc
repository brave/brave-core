/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/client/ads_client_notifier.h"

#include <memory>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_notifier_queue.h"
#include "url/gurl.h"

namespace brave_ads {

AdsClientNotifier::AdsClientNotifier()
    : pending_notifier_queue_(std::make_unique<AdsClientNotifierQueue>()) {}

AdsClientNotifier::~AdsClientNotifier() = default;

void AdsClientNotifier::AddObserver(AdsClientNotifierObserver* observer) {
  CHECK(observer);
  observers_.AddObserver(observer);
}

void AdsClientNotifier::RemoveObserver(AdsClientNotifierObserver* observer) {
  CHECK(observer);
  observers_.RemoveObserver(observer);
}

void AdsClientNotifier::NotifyPendingObservers() {
  should_queue_notifications_ = false;
  pending_notifier_queue_->Process();
}

void AdsClientNotifier::NotifyDidInitializeAds() const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyDidInitializeAds,
                       weak_factory_.GetWeakPtr()));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyDidInitializeAds();
  }
}

void AdsClientNotifier::NotifyRewardsWalletDidUpdate(
    const std::string& payment_id,
    const std::string& recovery_seed) const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyRewardsWalletDidUpdate,
                       weak_factory_.GetWeakPtr(), payment_id, recovery_seed));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyRewardsWalletDidUpdate(payment_id, recovery_seed);
  }
}

void AdsClientNotifier::NotifyLocaleDidChange(const std::string& locale) const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyLocaleDidChange,
                       weak_factory_.GetWeakPtr(), locale));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyLocaleDidChange(locale);
  }
}

void AdsClientNotifier::NotifyPrefDidChange(const std::string& path) const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyPrefDidChange,
                       weak_factory_.GetWeakPtr(), path));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyPrefDidChange(path);
  }
}

void AdsClientNotifier::NotifyDidUpdateResourceComponent(
    const std::string& manifest_version,
    const std::string& id) const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyDidUpdateResourceComponent,
                       weak_factory_.GetWeakPtr(), manifest_version, id));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyDidUpdateResourceComponent(manifest_version, id);
  }
}

void AdsClientNotifier::NotifyDidUnregisterResourceComponent(
    const std::string& id) const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyDidUnregisterResourceComponent,
                       weak_factory_.GetWeakPtr(), id));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyDidUnregisterResourceComponent(id);
  }
}

void AdsClientNotifier::NotifyTabTextContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(base::BindOnce(
        &AdsClientNotifier::NotifyTabTextContentDidChange,
        weak_factory_.GetWeakPtr(), tab_id, redirect_chain, text));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyTabTextContentDidChange(tab_id, redirect_chain, text);
  }
}

void AdsClientNotifier::NotifyTabHtmlContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(base::BindOnce(
        &AdsClientNotifier::NotifyTabHtmlContentDidChange,
        weak_factory_.GetWeakPtr(), tab_id, redirect_chain, html));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyTabHtmlContentDidChange(tab_id, redirect_chain, html);
  }
}

void AdsClientNotifier::NotifyTabDidStartPlayingMedia(
    const int32_t tab_id) const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyTabDidStartPlayingMedia,
                       weak_factory_.GetWeakPtr(), tab_id));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyTabDidStartPlayingMedia(tab_id);
  }
}

void AdsClientNotifier::NotifyTabDidStopPlayingMedia(
    const int32_t tab_id) const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyTabDidStopPlayingMedia,
                       weak_factory_.GetWeakPtr(), tab_id));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyTabDidStopPlayingMedia(tab_id);
  }
}

void AdsClientNotifier::NotifyTabDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const bool is_new_navigation,
    const bool is_restoring,
    const bool is_error_page,
    const bool is_visible) const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(base::BindOnce(
        &AdsClientNotifier::NotifyTabDidChange, weak_factory_.GetWeakPtr(),
        tab_id, redirect_chain, is_new_navigation, is_restoring, is_error_page,
        is_visible));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyTabDidChange(tab_id, redirect_chain, is_new_navigation,
                                  is_restoring, is_error_page, is_visible);
  }
}

void AdsClientNotifier::NotifyDidCloseTab(const int32_t tab_id) const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyDidCloseTab,
                       weak_factory_.GetWeakPtr(), tab_id));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyDidCloseTab(tab_id);
  }
}

void AdsClientNotifier::NotifyUserGestureEventTriggered(
    const int32_t page_transition_type) const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyUserGestureEventTriggered,
                       weak_factory_.GetWeakPtr(), page_transition_type));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyUserGestureEventTriggered(page_transition_type);
  }
}

void AdsClientNotifier::NotifyUserDidBecomeIdle() const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyUserDidBecomeIdle,
                       weak_factory_.GetWeakPtr()));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyUserDidBecomeIdle();
  }
}

void AdsClientNotifier::NotifyUserDidBecomeActive(
    const base::TimeDelta idle_time,
    const bool screen_was_locked) const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(base::BindOnce(
        &AdsClientNotifier::NotifyUserDidBecomeActive,
        weak_factory_.GetWeakPtr(), idle_time, screen_was_locked));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyUserDidBecomeActive(idle_time, screen_was_locked);
  }
}

void AdsClientNotifier::NotifyBrowserDidEnterForeground() const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyBrowserDidEnterForeground,
                       weak_factory_.GetWeakPtr()));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyBrowserDidEnterForeground();
  }
}

void AdsClientNotifier::NotifyBrowserDidEnterBackground() const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyBrowserDidEnterBackground,
                       weak_factory_.GetWeakPtr()));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyBrowserDidEnterBackground();
  }
}

void AdsClientNotifier::NotifyBrowserDidBecomeActive() const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyBrowserDidBecomeActive,
                       weak_factory_.GetWeakPtr()));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyBrowserDidBecomeActive();
  }
}

void AdsClientNotifier::NotifyBrowserDidResignActive() const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyBrowserDidResignActive,
                       weak_factory_.GetWeakPtr()));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyBrowserDidResignActive();
  }
}

void AdsClientNotifier::NotifyDidSolveAdaptiveCaptcha() const {
  if (should_queue_notifications_) {
    pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyDidSolveAdaptiveCaptcha,
                       weak_factory_.GetWeakPtr()));
    return;
  }

  for (auto& observer : observers_) {
    observer.OnNotifyDidSolveAdaptiveCaptcha();
  }
}

}  // namespace brave_ads
