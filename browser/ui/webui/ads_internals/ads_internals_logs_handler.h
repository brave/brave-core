// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_ADS_INTERNALS_ADS_INTERNALS_LOGS_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_ADS_INTERNALS_ADS_INTERNALS_LOGS_HANDLER_H_

#include <cstdint>
#include <optional>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_REWARDS));

class PrefService;

namespace brave_rewards {
class RewardsService;
}  // namespace brave_rewards

// Forwards brave://ads-internals' Logs tab to the Brave Rewards diagnostic
// log, which is the same log shown on brave://rewards-internals.
class AdsInternalsLogsHandler final : public bat_ads::mojom::AdsInternalsLogs {
 public:
  AdsInternalsLogsHandler(brave_rewards::RewardsService* rewards_service,
                          PrefService* local_state);

  AdsInternalsLogsHandler(const AdsInternalsLogsHandler&) = delete;
  AdsInternalsLogsHandler& operator=(const AdsInternalsLogsHandler&) = delete;

  ~AdsInternalsLogsHandler() override;

  void BindInterface(
      mojo::PendingReceiver<bat_ads::mojom::AdsInternalsLogs> pending_receiver);

  // Overrides the restart attempt so tests do not actually restart the
  // browser.
  void SetAttemptRestartCallbackForTesting(
      base::RepeatingClosure attempt_restart_callback);

 private:
  // bat_ads::mojom::AdsInternalsLogs:
  void GetLog(std::optional<uint32_t> num_lines,
              GetLogCallback callback) override;
  void ClearLog(ClearLogCallback callback) override;
  void ToggleVerboseLoggingAndRestart() override;

  const raw_ptr<brave_rewards::RewardsService> rewards_service_;  // Not owned.
  const raw_ptr<PrefService> local_state_;                        // Not owned.

  base::RepeatingClosure attempt_restart_callback_;

  mojo::Receiver<bat_ads::mojom::AdsInternalsLogs> receiver_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_ADS_INTERNALS_ADS_INTERNALS_LOGS_HANDLER_H_
