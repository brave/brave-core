/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_p3a.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/metrics/histogram_macros.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {

namespace {

const int kRefreshP3AFrequencyHours = 24;

// Has the Wallet keyring been created?
// 0) No, 1) Yes
void RecordKeyringCreated(bool created) {
  UMA_HISTOGRAM_BOOLEAN(kKeyringCreatedHistogramName,
                        static_cast<int>(created));
}

}  // namespace

BraveWalletP3A::BraveWalletP3A(BraveWalletService* wallet_service,
                               KeyringService* keyring_service,
                               PrefService* profile_prefs,
                               PrefService* local_state)
    : wallet_service_(wallet_service),
      keyring_service_(keyring_service),
      profile_prefs_(profile_prefs),
      local_state_(local_state) {
  DCHECK(profile_prefs);
  DCHECK(local_state);

  RecordInitialBraveWalletP3AState();
  AddObservers();

  local_state_change_registrar_.Init(local_state_);
  local_state_change_registrar_.Add(
      kBraveWalletLastUnlockTime,
      base::BindRepeating(&BraveWalletP3A::ReportUsage, base::Unretained(this),
                          true));
}

BraveWalletP3A::BraveWalletP3A() = default;

BraveWalletP3A::~BraveWalletP3A() = default;

void BraveWalletP3A::AddObservers() {
  keyring_service_->AddObserver(
      keyring_service_observer_receiver_.BindNewPipeAndPassRemote());
  update_timer_.Start(FROM_HERE, base::Hours(kRefreshP3AFrequencyHours), this,
                      &BraveWalletP3A::OnUpdateTimerFired);
  OnUpdateTimerFired();  // Also call on startup
}

void BraveWalletP3A::Bind(
    mojo::PendingReceiver<mojom::BraveWalletP3A> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void BraveWalletP3A::ReportUsage(bool unlocked) {
  base::Time wallet_last_used =
      local_state_->GetTime(kBraveWalletLastUnlockTime);

  if (unlocked) {
    p3a_utils::RecordFeatureUsage(local_state_, kBraveWalletP3AFirstUnlockTime,
                                  kBraveWalletP3ALastUnlockTime);
    WriteUsageStatsToHistogram();
  } else {
    // Maybe record existing timestamp in case the user is not new.
    p3a_utils::MaybeRecordFeatureExistingUsageTimestamp(
        local_state_, kBraveWalletP3AFirstUnlockTime,
        kBraveWalletP3ALastUnlockTime, wallet_last_used);
  }

  p3a_utils::RecordFeatureNewUserReturning(
      local_state_, kBraveWalletP3AFirstUnlockTime,
      kBraveWalletP3ALastUnlockTime, kBraveWalletP3AUsedSecondDay,
      kBraveWalletNewUserReturningHistogramName);
  p3a_utils::RecordFeatureLastUsageTimeMetric(
      local_state_, kBraveWalletP3ALastUnlockTime,
      kBraveWalletLastUsageTimeHistogramName);
}

void BraveWalletP3A::OnUpdateTimerFired() {
  ReportUsage(false);
}

void BraveWalletP3A::WriteUsageStatsToHistogram() {
  UMA_HISTOGRAM_BOOLEAN(kBraveWalletMonthlyHistogramName, true);
  UMA_HISTOGRAM_BOOLEAN(kBraveWalletWeeklyHistogramName, true);
  UMA_HISTOGRAM_BOOLEAN(kBraveWalletDailyHistogramName, true);
}

void BraveWalletP3A::RecordInitialBraveWalletP3AState() {
  RecordKeyringCreated(keyring_service_->IsWalletCreatedSync());
}

// KeyringServiceObserver
void BraveWalletP3A::WalletCreated() {
  RecordKeyringCreated(keyring_service_->IsWalletCreatedSync());
}

}  // namespace brave_wallet
