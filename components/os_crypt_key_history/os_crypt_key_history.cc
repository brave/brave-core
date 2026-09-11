/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/os_crypt_key_history/os_crypt_key_history.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/json/values_util.h"
#include "base/no_destructor.h"
#include "base/values.h"

namespace brave {

namespace {

// Bumped only for a format change that older builds cannot read. A file
// written by a newer version is treated as unreadable rather than replaced.
constexpr int kCurrentVersion = 1;

constexpr char kVersionKey[] = "version";
constexpr char kProvidersKey[] = "providers";
constexpr char kWrappedKeyKey[] = "wrapped_key";
constexpr char kFirstSeenKey[] = "first_seen";
constexpr char kLastVerifiedKey[] = "last_verified";

constexpr char kHistogramSuffix[] = "OSCryptKeyHistory";

}  // namespace

OSCryptKeyHistory::OSCryptKeyHistory(base::FilePath path)
    : path_(std::move(path)) {}

OSCryptKeyHistory::~OSCryptKeyHistory() = default;

OSCryptKeyHistory::LoadResult OSCryptKeyHistory::Load() {
  providers_.clear();

  std::string contents;
  if (!base::ReadFileToString(path_, &contents)) {
    // No file at all is a clean slate. A file we cannot read is not, and must
    // not be overwritten.
    return base::PathExists(path_) ? LoadResult::kUnreadable
                                   : LoadResult::kNoFile;
  }

  std::optional<base::DictValue> root =
      base::JSONReader::ReadDict(contents, base::JSON_PARSE_RFC);
  if (!root) {
    return LoadResult::kUnreadable;
  }

  const std::optional<int> version = root->FindInt(kVersionKey);
  if (!version || *version > kCurrentVersion) {
    return LoadResult::kUnreadable;
  }

  const base::DictValue* providers = root->FindDict(kProvidersKey);
  if (!providers) {
    // Well formed, just holding nothing.
    return LoadResult::kLoaded;
  }

  // Iterating a DictValue hands back a pair of references by value, so
  // this binds by value rather than by reference.
  for (const auto [provider, value] : *providers) {
    const base::ListValue* list = value.GetIfList();
    if (!list) {
      continue;
    }
    std::vector<OSCryptKeyRecord> records;
    for (const base::Value& item : *list) {
      const base::DictValue* entry = item.GetIfDict();
      if (!entry) {
        continue;
      }
      const std::string* wrapped_key = entry->FindString(kWrappedKeyKey);
      if (!wrapped_key || wrapped_key->empty()) {
        continue;
      }
      records.push_back(OSCryptKeyRecord{
          *wrapped_key,
          base::ValueToTime(entry->Find(kFirstSeenKey)).value_or(base::Time()),
          base::ValueToTime(entry->Find(kLastVerifiedKey))
              .value_or(base::Time())});
      if (records.size() == kMaxRecordsPerProvider) {
        break;
      }
    }
    if (!records.empty()) {
      providers_.emplace(provider, std::move(records));
    }
  }

  return LoadResult::kLoaded;
}

bool OSCryptKeyHistory::Save() const {
  base::DictValue providers;
  for (const auto& [provider, records] : providers_) {
    base::ListValue list;
    for (const OSCryptKeyRecord& record : records) {
      base::DictValue entry;
      entry.Set(kWrappedKeyKey, record.wrapped_key);
      entry.Set(kFirstSeenKey, base::TimeToValue(record.first_seen));
      entry.Set(kLastVerifiedKey, base::TimeToValue(record.last_verified));
      list.Append(std::move(entry));
    }
    providers.Set(provider, std::move(list));
  }

  base::DictValue root;
  root.Set(kVersionKey, kCurrentVersion);
  root.Set(kProvidersKey, std::move(providers));

  std::string json;
  if (!base::JSONWriter::WriteWithOptions(
          root, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json)) {
    return false;
  }

  return base::ImportantFileWriter::WriteFileAtomically(path_, json,
                                                        kHistogramSuffix);
}

void OSCryptKeyHistory::RecordVerifiedKey(std::string_view provider,
                                          std::string_view wrapped_key,
                                          base::Time now,
                                          const SameKeyPredicate& is_same_key) {
  if (wrapped_key.empty()) {
    return;
  }

  std::vector<OSCryptKeyRecord>& records = providers_[std::string(provider)];

  for (OSCryptKeyRecord& record : records) {
    if (is_same_key.Run(record.wrapped_key)) {
      record.last_verified = std::max(record.last_verified, now);
      return;
    }
  }

  records.insert(records.begin(),
                 OSCryptKeyRecord{std::string(wrapped_key), now, now});
  if (records.size() > kMaxRecordsPerProvider) {
    records.resize(kMaxRecordsPerProvider);
  }
}

const std::vector<OSCryptKeyRecord>& OSCryptKeyHistory::GetRecords(
    std::string_view provider) const {
  static const base::NoDestructor<std::vector<OSCryptKeyRecord>> kEmpty;
  const auto it = providers_.find(provider);
  return it == providers_.end() ? *kEmpty : it->second;
}

}  // namespace brave
