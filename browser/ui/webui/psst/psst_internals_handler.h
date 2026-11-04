/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_PSST_PSST_INTERNALS_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_PSST_PSST_INTERNALS_HANDLER_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/timer/timer.h"
#include "brave/components/psst/core/browser/psst_log_service.h"
#include "brave/components/psst/core/browser/psst_settings_service.h"
#include "brave/components/psst/core/common/psst_ui_common.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

class PrefService;

namespace psst {

class PsstSettingsService;

// Serves the brave://psst-internals timeline from the profile's
// `PsstLogService`.
class PsstInternalsHandler : public mojom::PsstInternalsHandler,
                             public PsstLogService::Observer,
                             public PsstSettingsService::PrefObserver {
 public:
  // `log_service` may be null — the feature is off or the profile is
  // off-the-record — in which case the page gets an empty timeline, no pushes,
  // and `Clear()` does nothing. `settings_service` and `prefs` may also be
  // null for the same off-the-record reason, in which case
  // `GetDiagnosticsInfo()` reports zeroed/empty data.
  PsstInternalsHandler(PsstLogService* log_service,
                       PsstSettingsService* settings_service,
                       PrefService* prefs);
  PsstInternalsHandler(const PsstInternalsHandler&) = delete;
  PsstInternalsHandler& operator=(const PsstInternalsHandler&) = delete;
  ~PsstInternalsHandler() override;

  void BindInterface(
      mojo::PendingReceiver<mojom::PsstInternalsHandler> receiver);

  // mojom::PsstInternalsHandler:
  void BindPage(mojo::PendingRemote<mojom::PsstInternalsPage> page,
                BindPageCallback callback) override;
  void Clear() override;
  void GetDiagnosticsInfo(GetDiagnosticsInfoCallback callback) override;

 private:
  // PsstLogService::Observer:
  void OnPsstLogEntryAdded(const mojom::PsstLogEntry& entry) override;
  void OnPsstLogCleared() override;

  // PsstSettingsService::PrefObserver:
  void OnPsstPrefsChange(
        int infobar_shown_counter,
        int infobar_decline_counter,
        const base::flat_map<std::string, PsstWebsiteSettings>&
            psst_websites_settings) override;

  void FlushPendingEntries();

  const raw_ptr<PsstLogService> log_service_;
  const raw_ptr<PsstSettingsService> settings_service_;
  const raw_ptr<PrefService> prefs_;

  // New entries are coalesced over a short window so that a chatty script
  // can't turn into one IPC per line.
  std::vector<mojom::PsstLogEntryPtr> pending_entries_;
  base::OneShotTimer flush_timer_;

  mojo::Receiver<mojom::PsstInternalsHandler> receiver_{this};
  mojo::Remote<mojom::PsstInternalsPage> page_;
  base::ScopedObservation<PsstLogService, PsstLogService::Observer>
      log_observation_{this};
  base::ScopedObservation<PsstSettingsService, PsstSettingsService::PrefObserver>
      settings_observation_{this};
};

}  // namespace psst

#endif  // BRAVE_BROWSER_UI_WEBUI_PSST_PSST_INTERNALS_HANDLER_H_
