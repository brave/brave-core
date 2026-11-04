/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/psst/psst_internals_handler.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/psst/core/browser/pref_names.h"
#include "brave/components/psst/core/browser/psst_settings_service.h"
#include "components/prefs/pref_service.h"

namespace psst {

namespace {

constexpr base::TimeDelta kFlushDelay = base::Milliseconds(250);

}  // namespace

PsstInternalsHandler::PsstInternalsHandler(
    PsstLogService* log_service,
    PsstSettingsService* settings_service,
    PrefService* prefs)
    : log_service_(log_service),
      settings_service_(settings_service),
      prefs_(prefs) {
  if (log_service_) {
    log_observation_.Observe(log_service_);
  }
  if (settings_service_) {
    settings_observation_.Observe(settings_service_);
  }
}

PsstInternalsHandler::~PsstInternalsHandler() = default;

void PsstInternalsHandler::BindInterface(
    mojo::PendingReceiver<mojom::PsstInternalsHandler> receiver) {
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

void PsstInternalsHandler::BindPage(
    mojo::PendingRemote<mojom::PsstInternalsPage> page,
    BindPageCallback callback) {
  page_.reset();
  page_.Bind(std::move(page));
  std::move(callback).Run(log_service_ ? log_service_->GetEntries()
                                       : std::vector<mojom::PsstLogEntryPtr>());
}

void PsstInternalsHandler::Clear() {
  if (log_service_) {
    log_service_->Clear();
  }
}

void PsstInternalsHandler::GetDiagnosticsInfo(
    GetDiagnosticsInfoCallback callback) {
  auto info = mojom::PsstDiagnosticsInfo::New();
  if (prefs_) {
    info->infobar_shown_count =
        prefs_->GetInteger(prefs::kPsstInfobarShownCounter);
    info->infobar_decline_count =
        prefs_->GetInteger(prefs::kPsstInfobarDeclineCounter);
  }
  if (settings_service_) {
    for (const auto& [site, settings] :
         settings_service_->GetAllPsstWebsitesSettings()) {
      auto entry = mojom::PsstWebsiteSettingsEntry::New();
      entry->site = site;
      entry->consent_status =
          static_cast<mojom::ConsentStatus>(settings.consent_status);
      entry->script_version = settings.script_version;
      entry->user_id = settings.user_id;
      entry->uids_to_perform = settings.uids_to_perform;
      info->website_settings.push_back(std::move(entry));
    }
  }
  std::move(callback).Run(std::move(info));
}

void PsstInternalsHandler::OnPsstLogEntryAdded(
    const mojom::PsstLogEntry& entry) {
  if (!page_) {
    return;
  }
  pending_entries_.push_back(entry.Clone());
  if (!flush_timer_.IsRunning()) {
    flush_timer_.Start(
        FROM_HERE, kFlushDelay,
        base::BindOnce(&PsstInternalsHandler::FlushPendingEntries,
                       base::Unretained(this)));
  }
}

void PsstInternalsHandler::OnPsstLogCleared() {
  pending_entries_.clear();
  flush_timer_.Stop();
  if (page_) {
    page_->OnCleared();
  }
}

void PsstInternalsHandler::OnPsstPrefsChange(
    int infobar_shown_counter,
    int infobar_decline_counter,
    const base::flat_map<std::string, PsstWebsiteSettings>&
        psst_websites_settings) {
  if (!page_) {
    return;
  }

  auto info = mojom::PsstDiagnosticsInfo::New();
  info->infobar_shown_count = infobar_shown_counter;
  info->infobar_decline_count = infobar_decline_counter;
  for (const auto& [site, settings] : psst_websites_settings) {
    auto entry = mojom::PsstWebsiteSettingsEntry::New();
    entry->site = site;
    entry->consent_status =
        static_cast<mojom::ConsentStatus>(settings.consent_status);
    entry->script_version = settings.script_version;
    entry->user_id = settings.user_id;
    entry->uids_to_perform = settings.uids_to_perform;
    info->website_settings.push_back(std::move(entry));
  }
  page_->OnPsstSettingsChanged(std::move(info));
}

void PsstInternalsHandler::FlushPendingEntries() {
  if (pending_entries_.empty()) {
    return;
  }
  if (page_) {
    page_->OnEntriesAdded(std::move(pending_entries_));
  }
  pending_entries_.clear();
}

}  // namespace psst
