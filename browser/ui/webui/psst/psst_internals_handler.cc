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
#include "brave/components/psst/core/common/psst_metadata_schema.h"
#include "brave/components/psst/core/common/psst_ui_common.mojom-forward.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"

namespace psst {

namespace {

constexpr base::TimeDelta kFlushDelay = base::Milliseconds(250);

// psst::mojom::ConsentStatus must stay numerically identical to
// psst::ConsentStatus (psst_metadata_schema.idl)
static_assert(static_cast<int>(mojom::ConsentStatus::kNone) ==
              static_cast<int>(ConsentStatus::kNone));
static_assert(static_cast<int>(mojom::ConsentStatus::kAsk) ==
              static_cast<int>(ConsentStatus::kAsk));
static_assert(static_cast<int>(mojom::ConsentStatus::kAllow) ==
              static_cast<int>(ConsentStatus::kAllow));
static_assert(static_cast<int>(mojom::ConsentStatus::kBlock) ==
              static_cast<int>(ConsentStatus::kBlock));
static_assert(static_cast<int>(mojom::ConsentStatus::kMaxValue) ==
              static_cast<int>(ConsentStatus::kMaxValue));

std::vector<mojom::PsstWebsiteSettingsEntryPtr> BuildWebsiteSettingsEntries(
    const base::flat_map<std::string, PsstWebsiteSettingsExtended>&
        psst_websites_settings) {
  std::vector<mojom::PsstWebsiteSettingsEntryPtr> result;
  for (const auto& [site, settings] : psst_websites_settings) {
    if (settings.website_settings.empty()) {
      auto entry = mojom::PsstWebsiteSettingsEntry::New();
      entry->site = site;
      entry->infobar_shown_count = settings.infobar_shown_counter;
      entry->infobar_decline_count = settings.infobar_decline_counter;
      entry->infobar_review_sugg_counter = settings.infobar_review_sugg_counter;
      result.push_back(std::move(entry));
      continue;
    }

    for (const auto& wss : settings.website_settings) {
      auto entry = mojom::PsstWebsiteSettingsEntry::New();
      entry->site = site;
      entry->infobar_shown_count = settings.infobar_shown_counter;
      entry->infobar_decline_count = settings.infobar_decline_counter;
      entry->infobar_review_sugg_counter = settings.infobar_review_sugg_counter;
      entry->consent_status =
          static_cast<mojom::ConsentStatus>(wss.consent_status);
      entry->script_version = wss.script_version;
      entry->user_id = wss.user_id;
      entry->uids_to_perform = wss.uids_to_perform;
      result.push_back(std::move(entry));
    }
  }
  return result;
}

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

void PsstInternalsHandler::GetPsstWebsiteSettingsEntries(
    GetPsstWebsiteSettingsEntriesCallback callback) {
  callback = mojo::WrapCallbackWithDefaultInvokeIfNotRun(
      std::move(callback), std::vector<mojom::PsstWebsiteSettingsEntryPtr>());

  if (!settings_service_) {
    return;
  }

  std::move(callback).Run(BuildWebsiteSettingsEntries(
      settings_service_->GetAllPsstWebsitesSettings()));
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
    const base::flat_map<std::string, PsstWebsiteSettingsExtended>&
        psst_websites_settings) {
  if (!page_) {
    return;
  }

  if (!settings_service_) {
    return;
  }

  page_->OnPsstSettingsChanged(BuildWebsiteSettingsEntries(
      settings_service_->GetAllPsstWebsitesSettings()));
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
