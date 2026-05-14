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
constexpr base::TimeDelta kOnboardingRecordDelay = base::Seconds(120);

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

  // try to record the onboarding histogram
  // just in the case the user quit the app
  // before the 120 second deadline in the last
  // app session
  RecordOnboardingHistogram();
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

std::optional<mojom::OnboardingAction>
BraveWalletP3A::GetLastOnboardingAction() {
  if (local_state_->HasPrefPath(kBraveWalletP3AOnboardingLastStep)) {
    int pref_value =
        local_state_->GetInteger(kBraveWalletP3AOnboardingLastStep);
    return static_cast<mojom::OnboardingAction>(pref_value);
  }
  return std::nullopt;
}

void BraveWalletP3A::ReportOnboardingAction(mojom::OnboardingAction action) {
  if (action == mojom::OnboardingAction::StartRestore) {
    // We do not want to monitor wallet restores; cancel the
    // histogram record timer and wipe out the last onboarding step.
    local_state_->ClearPref(kBraveWalletP3AOnboardingLastStep);
    onboarding_report_timer_.Stop();
    return;
  }
  std::optional<mojom::OnboardingAction> last_step = GetLastOnboardingAction();
  if (!last_step.has_value() || *last_step < action) {
    // Only record steps that are ahead of the previous step so we
    // don't record back navigation.
    local_state_->SetInteger(kBraveWalletP3AOnboardingLastStep,
                             static_cast<int>(action));
  }
  if (onboarding_report_timer_.IsRunning() ||
      action == mojom::OnboardingAction::Shown) {
    // If the event is the first possible action (aka the shown event),
    // or if timer is already running (re)start the timer to debounce.
    onboarding_report_timer_.Start(
        FROM_HERE, kOnboardingRecordDelay,
        base::BindOnce(&BraveWalletP3A::RecordOnboardingHistogram,
                       base::Unretained(this)));
  } else {
    // If the timer is not running and the action is after the first possible
    // event, report it right away since it probably missed the 120 sec
    // deadline.
    RecordOnboardingHistogram();
  }
}

void BraveWalletP3A::RecordOnboardingHistogram() {
  std::optional<mojom::OnboardingAction> last_step = GetLastOnboardingAction();
  if (!last_step.has_value()) {
    return;
  }
  local_state_->ClearPref(kBraveWalletP3AOnboardingLastStep);
  UMA_HISTOGRAM_ENUMERATION(kOnboardingConversionHistogramName, *last_step);
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
