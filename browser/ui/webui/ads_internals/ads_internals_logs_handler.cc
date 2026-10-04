// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ads_internals/ads_internals_logs_handler.h"

#include <string>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "brave/components/brave_rewards/content/rewards_service.h"
#include "brave/components/brave_rewards/core/features.h"
#include "chrome/browser/about_flags.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "components/prefs/pref_service.h"
#include "components/webui/flags/pref_service_flags_storage.h"

namespace {

// `RewardsService::LoadDiagnosticLog` treats this as "no limit, return the
// full log".
constexpr int kFullLogNumLines = -1;

}  // namespace

AdsInternalsLogsHandler::AdsInternalsLogsHandler(
    brave_rewards::RewardsService* rewards_service,
    PrefService* local_state)
    : rewards_service_(rewards_service),
      local_state_(local_state),
      attempt_restart_callback_(base::BindRepeating(&chrome::AttemptRestart)) {}

AdsInternalsLogsHandler::~AdsInternalsLogsHandler() = default;

void AdsInternalsLogsHandler::BindInterface(
    mojo::PendingReceiver<bat_ads::mojom::AdsInternalsLogs> pending_receiver) {
  if (receiver_.is_bound()) {
    receiver_.reset();
  }

  receiver_.Bind(std::move(pending_receiver));
}

void AdsInternalsLogsHandler::SetAttemptRestartCallbackForTesting(  // IN-TEST
    base::RepeatingClosure attempt_restart_callback) {
  attempt_restart_callback_ = std::move(attempt_restart_callback);
}

///////////////////////////////////////////////////////////////////////////////

void AdsInternalsLogsHandler::GetLog(std::optional<uint32_t> num_lines,
                                     GetLogCallback callback) {
  if (!rewards_service_) {
    return std::move(callback).Run(/*log=*/"");
  }
  return rewards_service_->LoadDiagnosticLog(
      num_lines ? static_cast<int>(*num_lines) : kFullLogNumLines,
      std::move(callback));
}

void AdsInternalsLogsHandler::ClearLog(ClearLogCallback callback) {
  if (!rewards_service_) {
    return std::move(callback).Run(/*success=*/false);
  }
  return rewards_service_->ClearDiagnosticLog(std::move(callback));
}

void AdsInternalsLogsHandler::ToggleVerboseLoggingAndRestart() {
  std::string internal_name = "brave-rewards-verbose-logging";
  if (base::FeatureList::IsEnabled(
          brave_rewards::features::kVerboseLoggingFeature)) {
    internal_name += "@0";  // The "Default" (disabled) option.
  } else {
    internal_name += "@1";  // The "Enabled" option.
  }
  flags_ui::PrefServiceFlagsStorage flags_storage(local_state_);
  about_flags::SetFeatureEntryEnabled(&flags_storage, internal_name, true);
  attempt_restart_callback_.Run();
}
