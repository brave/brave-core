/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/psst/core/browser/psst_log_service.h"

#include <optional>
#include <string>
#include <utility>

#include "base/containers/to_vector.h"
#include "base/json/json_writer.h"
#include "base/time/time.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

namespace psst {

namespace {

// eTLD+1, falling back to the host for URLs that have no registry (IP
// literals, localhost) so the entry is still attributable.
std::string GetSite(const GURL& url) {
  if (!url.is_valid()) {
    return std::string();
  }
  std::string site = net::registry_controlled_domains::GetDomainAndRegistry(
      url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
  return site.empty() ? std::string(url.host()) : site;
}

}  // namespace

PsstLogService::PsstLogService() = default;
PsstLogService::~PsstLogService() = default;

void PsstLogService::AddObserver(Observer* observer) {
  observers_.AddObserver(observer);
}

void PsstLogService::RemoveObserver(Observer* observer) {
  observers_.RemoveObserver(observer);
}

void PsstLogService::AddEntry(mojom::PsstLogSource source,
                              mojom::PsstLogLevel level,
                              std::optional<int32_t> tab_id,
                              const GURL& url,
                              std::string_view message,
                              std::optional<base::DictValue> data) {
  auto entry = mojom::PsstLogEntry::New();
  entry->timestamp_ms =
      base::Time::Now().InMillisecondsFSinceUnixEpochIgnoringNull();
  entry->tab_id = tab_id;
  entry->level = level;
  entry->source = source;
  entry->site = GetSite(url);
  entry->message = std::string(message);
  if (data && !data->empty()) {
    entry->data = base::WriteJson(*data);
  }

  if (entries_.size() >= kMaxLogEntries) {
    entries_.pop_front();
  }
  entries_.push_back(std::move(entry));

  observers_.Notify(&Observer::OnPsstLogEntryAdded, *entries_.back());
}

void PsstLogService::AddNativeEntry(mojom::PsstLogLevel level,
                                    std::optional<int32_t> tab_id,
                                    const GURL& url,
                                    std::string_view message,
                                    std::optional<base::DictValue> data) {
  AddEntry(mojom::PsstLogSource::kNative, level, tab_id, url, message,
           std::move(data));
}

std::vector<mojom::PsstLogEntryPtr> PsstLogService::GetEntries() const {
  return base::ToVector(entries_,
                        [](const auto& entry) { return entry->Clone(); });
}

void PsstLogService::Clear() {
  if (entries_.empty()) {
    return;
  }
  entries_.clear();
  observers_.Notify(&Observer::OnPsstLogCleared);
}

base::WeakPtr<PsstLogService> PsstLogService::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void PsstLogService::Shutdown() {
  // Drop the diagnostics eagerly, and stop renderers that are still holding a
  // weak reference from reaching a half-torn-down service.
  entries_.clear();
  weak_ptr_factory_.InvalidateWeakPtrs();
}

}  // namespace psst
