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
  should_queue_notifications_ = false;
  pending_notifier_queue_->Process();
}

void AdsClientNotifier::NotifyDidInitializeAds() {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyDidInitializeAds,
                       weak_factory_.GetWeakPtr()));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyDidInitializeAds();
  }
}

void AdsClientNotifier::NotifyRewardsWalletDidUpdate(
    const std::string& payment_id,
    const std::string& recovery_seed) {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyRewardsWalletDidUpdate,
                       weak_factory_.GetWeakPtr(), payment_id, recovery_seed));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyRewardsWalletDidUpdate(payment_id, recovery_seed);
  }
}

void AdsClientNotifier::NotifyLocaleDidChange(const std::string& locale) {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyLocaleDidChange,
                       weak_factory_.GetWeakPtr(), locale));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyLocaleDidChange(locale);
  }
}

void AdsClientNotifier::NotifyPrefDidChange(const std::string& path) {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(
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
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyResourceComponentDidChange,
                       weak_factory_.GetWeakPtr(), manifest_version, id));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyResourceComponentDidChange(manifest_version, id);
  }
}

void AdsClientNotifier::NotifyDidUnregisterResourceComponent(
    const std::string& id) {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyDidUnregisterResourceComponent,
                       weak_factory_.GetWeakPtr(), id));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyDidUnregisterResourceComponent(id);
  }
}

void AdsClientNotifier::NotifyTabTextContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& text) {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(base::BindOnce(
        &AdsClientNotifier::NotifyTabTextContentDidChange,
        weak_factory_.GetWeakPtr(), tab_id, redirect_chain, text));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyTabTextContentDidChange(tab_id, redirect_chain, text);
  }
}

void AdsClientNotifier::NotifyTabHtmlContentDidChange(
    const int32_t tab_id,
    const std::vector<GURL>& redirect_chain,
    const std::string& html) {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(base::BindOnce(
        &AdsClientNotifier::NotifyTabHtmlContentDidChange,
        weak_factory_.GetWeakPtr(), tab_id, redirect_chain, html));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyTabHtmlContentDidChange(tab_id, redirect_chain, html);
  }
}

void AdsClientNotifier::NotifyTabDidStartPlayingMedia(const int32_t tab_id) {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyTabDidStartPlayingMedia,
                       weak_factory_.GetWeakPtr(), tab_id));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyTabDidStartPlayingMedia(tab_id);
  }
}

void AdsClientNotifier::NotifyTabDidStopPlayingMedia(const int32_t tab_id) {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyTabDidStopPlayingMedia,
                       weak_factory_.GetWeakPtr(), tab_id));
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
    const bool is_visible) {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(base::BindOnce(
        &AdsClientNotifier::NotifyTabDidChange, weak_factory_.GetWeakPtr(),
        tab_id, redirect_chain, is_new_navigation, is_restoring, is_error_page,
        is_visible));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyTabDidChange(tab_id, redirect_chain, is_new_navigation,
                                  is_restoring, is_error_page, is_visible);
  }
}

void AdsClientNotifier::NotifyDidCloseTab(const int32_t tab_id) {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyDidCloseTab,
                       weak_factory_.GetWeakPtr(), tab_id));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyDidCloseTab(tab_id);
  }
}

void AdsClientNotifier::NotifyUserGestureEventTriggered(
    const int32_t page_transition_type) {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyUserGestureEventTriggered,
                       weak_factory_.GetWeakPtr(), page_transition_type));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyUserGestureEventTriggered(page_transition_type);
  }
}

void AdsClientNotifier::NotifyUserDidBecomeIdle() {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyUserDidBecomeIdle,
                       weak_factory_.GetWeakPtr()));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyUserDidBecomeIdle();
  }
}

void AdsClientNotifier::NotifyUserDidBecomeActive(
    const base::TimeDelta idle_time,
    const bool screen_was_locked) {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(base::BindOnce(
        &AdsClientNotifier::NotifyUserDidBecomeActive,
        weak_factory_.GetWeakPtr(), idle_time, screen_was_locked));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyUserDidBecomeActive(idle_time, screen_was_locked);
  }
}

void AdsClientNotifier::NotifyBrowserDidEnterForeground() {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyBrowserDidEnterForeground,
                       weak_factory_.GetWeakPtr()));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyBrowserDidEnterForeground();
  }
}

void AdsClientNotifier::NotifyBrowserDidEnterBackground() {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyBrowserDidEnterBackground,
                       weak_factory_.GetWeakPtr()));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyBrowserDidEnterBackground();
  }
}

void AdsClientNotifier::NotifyBrowserDidBecomeActive() {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyBrowserDidBecomeActive,
                       weak_factory_.GetWeakPtr()));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyBrowserDidBecomeActive();
  }
}

void AdsClientNotifier::NotifyBrowserDidResignActive() {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyBrowserDidResignActive,
                       weak_factory_.GetWeakPtr()));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyBrowserDidResignActive();
  }
}

void AdsClientNotifier::NotifyDidSolveAdaptiveCaptcha() {
  if (should_queue_notifications_) {
    return pending_notifier_queue_->Add(
        base::BindOnce(&AdsClientNotifier::NotifyDidSolveAdaptiveCaptcha,
                       weak_factory_.GetWeakPtr()));
  }

  for (auto& observer : observers_) {
    observer.OnNotifyDidSolveAdaptiveCaptcha();
  }
}

}  // namespace brave_ads
