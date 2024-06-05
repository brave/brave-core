/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/signature_basename.h"

#include <string>
#include <utility>

#include "base/hash/hash.h"
#include "base/json/json_writer.h"
#include "base/rand_util.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/web_discovery/browser/payload_generator.h"
#include "brave/components/web_discovery/browser/pref_names.h"
#include "brave/components/web_discovery/browser/server_config_loader.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "crypto/sha2.h"
#include "third_party/re2/src/re2/re2.h"

namespace web_discovery {

namespace {

constexpr char kPunctuationRegex[] = "[!\"'()*,-./:;?[\\]^_`{|}~%$=&+#]";
constexpr char kUrlNormalizationFunc[] = "url";
constexpr char kFlattenObjNormalizationFunc[] = "obj";
constexpr size_t kMsInHour = 60 * 60 * 1000;

constexpr char kExpiresAtKey[] = "expires_at";
constexpr char kUsedCountsKey[] = "counts";

void RecurseFlattenObject(const base::Value& value,
                          const base::Value::List& parent_keys,
                          base::Value::List& output) {
  if (value.is_dict()) {
    const auto& dict = value.GetDict();
    base::flat_set<std::string> keys;
    // insert into set so we can sort keys
    for (const auto [key, _] : dict) {
      keys.insert(key);
    }
    for (const auto& key : keys) {
      base::Value::List next_parent_keys = parent_keys.Clone();
      next_parent_keys.Append(key);
      RecurseFlattenObject(*dict.Find(key), next_parent_keys, output);
    }
  } else if (value.is_list()) {
    const auto& list = value.GetList();
    for (size_t i = 0; i < list.size(); i++) {
      base::Value::List next_parent_keys = parent_keys.Clone();
      next_parent_keys.Append(base::NumberToString(i));
      RecurseFlattenObject(list[i], next_parent_keys, output);
    }
  } else {
    base::Value::List flattened_value;
    flattened_value.Append(parent_keys.Clone());
    flattened_value.Append(value.Clone());
    output.Append(std::move(flattened_value));
  }
}

base::Value FlattenObject(const base::Value& obj) {
  base::Value::List result;
  RecurseFlattenObject(obj, base::Value::List(), result);
  return base::Value(std::move(result));
}

base::Value CleanURL(const base::Value& url) {
  if (!url.is_string()) {
    return base::Value();
  }
  auto url_str = base::ToLowerASCII(url.GetString());
  base::RemoveChars(url_str, " ", &url_str);
  base::ReplaceSubstringsAfterOffset(&url_str, 0, "https://", "");
  base::ReplaceSubstringsAfterOffset(&url_str, 0, "http://", "");
  base::ReplaceSubstringsAfterOffset(&url_str, 0, "www.", "");

  re2::RE2 punc_regex(kPunctuationRegex);
  re2::RE2::GlobalReplace(&url_str, punc_regex, "");
  return base::Value(std::move(url_str));
}

int GetPeriodHoursSinceEpoch(size_t period_hours) {
  auto hours_since_epoch =
      base::Time::Now().InMillisecondsSinceUnixEpoch() / kMsInHour;
  auto epoch_period_hours = period_hours * (hours_since_epoch / period_hours);
  return epoch_period_hours;
}

std::optional<int> GetBasenameCount(PrefService* profile_prefs,
                                    const base::Value::List& interim_tag_list,
                                    const SourceMapActionConfig& action_config,
                                    size_t period_hours) {
  std::string interim_tag_json;
  if (!base::JSONWriter::Write(base::Value(interim_tag_list.Clone()),
                               &interim_tag_json)) {
    return std::nullopt;
  }
  auto tag_hash = base::NumberToString(base::PersistentHash(interim_tag_json));

  // clean up expired counts
  ScopedDictPrefUpdate update(profile_prefs, kUsedBasenameCounts);
  base::Time now = base::Time::Now();
  for (auto it = update->begin(); it != update->end();) {
    const auto* value_dict = it->second.GetIfDict();
    if (!value_dict) {
      it = update->erase(it);
      continue;
    }
    const auto expire_time = value_dict->FindDouble(kExpiresAtKey);
    if (!expire_time ||
        now >= base::Time::FromTimeT(static_cast<time_t>(*expire_time))) {
      it = update->erase(it);
      continue;
    }
    it++;
  }

  auto* count_dict = update->EnsureDict(tag_hash);
  if (!count_dict->contains(kExpiresAtKey)) {
    auto expire_time =
        base::Time::FromMillisecondsSinceUnixEpoch(static_cast<int64_t>(
            (period_hours + action_config.period) * kMsInHour));
    count_dict->Set(kExpiresAtKey, static_cast<double>(expire_time.ToTimeT()));
  }

  auto* used_counts_list = count_dict->EnsureList(kUsedCountsKey);
  if (used_counts_list->size() >= action_config.limit) {
    return std::nullopt;
  }

  while (auto count = base::RandInt(0, action_config.limit - 1)) {
    if (base::ranges::find(used_counts_list->begin(), used_counts_list->end(),
                           count) != used_counts_list->end()) {
      continue;
    }
    used_counts_list->Append(count);
    return count;
  }
  NOTREACHED();
  return 0;
}

}  // namespace

BasenameResult::BasenameResult(std::vector<const uint8_t> basename,
                               size_t count)
    : basename(basename), count(count) {}

BasenameResult::~BasenameResult() = default;

std::optional<BasenameResult> GenerateBasename(
    PrefService* profile_prefs,
    ServerConfig* server_config,
    const base::Value::Dict& payload) {
  const std::string* action = payload.FindString(kActionKey);
  if (!action || action->empty()) {
    return std::nullopt;
  }
  const auto action_config = server_config->source_map_actions.find(*action);
  if (action_config == server_config->source_map_actions.end()) {
    return std::nullopt;
  }
  const auto* inner_payload = payload.FindDict(kInnerPayloadKey);
  if (!inner_payload) {
    return std::nullopt;
  }
  base::Value::List tag_list;
  tag_list.Append(*action);
  tag_list.Append(static_cast<int>(action_config->second.period));
  tag_list.Append(static_cast<int>(action_config->second.limit));

  base::Value::List key_values;
  for (const auto& key : action_config->second.keys) {
    auto parts =
        base::SplitString(key, "->", base::WhitespaceHandling::TRIM_WHITESPACE,
                          base::SPLIT_WANT_ALL);
    if (parts.empty() || parts[0].empty()) {
      continue;
    }
    const auto* value = inner_payload->FindByDottedPath(parts[0]);
    auto final_value = value ? value->Clone() : base::Value();
    if (parts.size() > 1) {
      if (parts[1] == kUrlNormalizationFunc) {
        final_value = CleanURL(final_value);
      } else if (parts[1] == kFlattenObjNormalizationFunc) {
        final_value = FlattenObject(final_value);
      }
    }
    key_values.Append(std::move(final_value));
  }

  auto period_hours = GetPeriodHoursSinceEpoch(action_config->second.period);
  tag_list.Append(std::move(key_values));
  tag_list.Append(period_hours);

  auto basename_count = GetBasenameCount(profile_prefs, tag_list,
                                         action_config->second, period_hours);
  if (!basename_count) {
    return std::nullopt;
  }
  tag_list.Append(*basename_count);

  std::string tag_json;
  if (!base::JSONWriter::Write(base::Value(std::move(tag_list)), &tag_json)) {
    return std::nullopt;
  }

  auto tag_hash = crypto::SHA256HashString(tag_json);
  std::vector<const uint8_t> tag_hash_vector(tag_hash.begin(), tag_hash.end());
  return std::make_optional<BasenameResult>(tag_hash_vector, *basename_count);
}

}  // namespace web_discovery
