/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ephemeral_storage/fpsa_entry.h"

#include "base/json/values_util.h"
#include "base/logging.h"
#include "base/values.h"

namespace ephemeral_storage {

namespace {

constexpr char kUrlKey[] = "u";
constexpr char kPartitionDomainKey[] = "pd";
constexpr char kPartitionNameKey[] = "pn";
constexpr char kClosedAtKey[] = "t";

content::StoragePartitionConfig MakeStoragePartitionConfig(
    content::BrowserContext* browser_context,
    const std::string& partition_domain,
    const std::string& partition_name) {
  // The default storage partition is stored with empty partition values,
  // which StoragePartitionConfig::Create() doesn't accept.
  if (partition_domain.empty()) {
    return content::StoragePartitionConfig::CreateDefault(browser_context);
  }
  return content::StoragePartitionConfig::Create(
      browser_context, partition_domain, partition_name,
      /*in_memory=*/false);
}

}  // namespace

FirstPartyStorageAreaEntry::FirstPartyStorageAreaEntry(
    const GURL& url,
    const content::StoragePartitionConfig& storage_partition_config,
    base::Time closed_at)
    : url_(url),
      storage_partition_config_(storage_partition_config),
      closed_at_(closed_at) {}

FirstPartyStorageAreaEntry::~FirstPartyStorageAreaEntry() = default;

// static
std::optional<FirstPartyStorageAreaEntry>
FirstPartyStorageAreaEntry::FromValue(
    const base::Value& value,
    content::BrowserContext* browser_context) {
  // Legacy format: a bare url spec string for the default storage partition.
  if (value.is_string()) {
    return FirstPartyStorageAreaEntry(
        GURL(value.GetString()),
        content::StoragePartitionConfig::CreateDefault(browser_context),
        // No stored close time -> treat as expired via base::Time::Min().
        base::Time::Min());
  }

  const base::DictValue* dict = value.GetIfDict();
  if (!dict) {
    return std::nullopt;
  }
  const std::string* url_spec = dict->FindString(kUrlKey);
  const std::string* partition_domain = dict->FindString(kPartitionDomainKey);
  const std::string* partition_name = dict->FindString(kPartitionNameKey);
  if (!url_spec || !partition_domain || !partition_name) {
    return std::nullopt;
  }

  // Older dict entries have no close time; treat those as expired.
  const base::Time closed_at =
      base::ValueToTime(dict->Find(kClosedAtKey)).value_or(base::Time::Min());

  return FirstPartyStorageAreaEntry(
      GURL(*url_spec),
      MakeStoragePartitionConfig(browser_context, *partition_domain,
                                 *partition_name),
      closed_at);
}

// static
FirstPartyStorageAreaEntry FirstPartyStorageAreaEntry::Create(
    const GURL& url,
    const content::StoragePartitionConfig& storage_partition_config) {
  return FirstPartyStorageAreaEntry(url, storage_partition_config,
                                    base::Time::Now());
}

base::Value FirstPartyStorageAreaEntry::ToValue() const {
  return base::Value(
      base::DictValue()
          .Set(kUrlKey, url_.spec())
          .Set(kPartitionDomainKey,
               storage_partition_config_.partition_domain())
          .Set(kPartitionNameKey, storage_partition_config_.partition_name())
          .Set(kClosedAtKey, base::TimeToValue(closed_at_)));
}

bool FirstPartyStorageAreaEntry::Matches(
    const GURL& url,
    const content::StoragePartitionConfig& storage_partition_config) const {
  return url_ == url && storage_partition_config_ == storage_partition_config;
}

bool FirstPartyStorageAreaEntry::IsKeepAliveExpired(
    base::TimeDelta keep_alive) const {
  const base::TimeDelta elapsed = base::Time::Now() - closed_at_;
  DVLOG(1) << __func__ << " closed_at:" << closed_at_
           << " elapsed:" << elapsed.InSeconds()
           << " keep_alive:" << keep_alive;
  // A backwards clock jump is treated as an expired keepalive.
  return elapsed.is_negative() || elapsed >= keep_alive;
}

}  // namespace ephemeral_storage