/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/core/browser/playlist_exclusion.h"

#include <string_view>
#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/string_util.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

namespace playlist {

namespace {

constexpr char kPlaylistExclusionsFile[] = "playlist_exclusions.json";
constexpr char kRules[] = "rules";
constexpr char kRegistrableDomain[] = "registrable_domain";
constexpr char kDenyRootPath[] = "deny_root_path";
constexpr char kPathPrefixes[] = "path_prefixes";

bool GetPathPrefixesFromValue(const base::Value* value,
                              std::vector<std::string>* result) {
  const base::ListValue* list = value->GetIfList();
  if (!list) {
    return false;
  }
  result->clear();
  for (const base::Value& prefix : *list) {
    const std::string* prefix_string = prefix.GetIfString();
    if (prefix_string) {
      result->push_back(*prefix_string);
    }
  }
  return true;
}

}  // namespace

PlaylistResolveRule::PlaylistResolveRule() = default;

PlaylistResolveRule::~PlaylistResolveRule() = default;

PlaylistResolveRule::PlaylistResolveRule(const PlaylistResolveRule&) = default;

PlaylistResolveRule& PlaylistResolveRule::operator=(
    const PlaylistResolveRule&) = default;

PlaylistResolveRule::PlaylistResolveRule(PlaylistResolveRule&&) noexcept =
    default;

PlaylistResolveRule& PlaylistResolveRule::operator=(
    PlaylistResolveRule&&) noexcept = default;

// static
void PlaylistResolveRule::RegisterJSONConverter(
    base::JSONValueConverter<PlaylistResolveRule>* converter) {
  converter->RegisterStringField(kRegistrableDomain,
                                 &PlaylistResolveRule::registrable_domain);
  converter->RegisterBoolField(kDenyRootPath,
                               &PlaylistResolveRule::deny_root_path);
  converter->RegisterCustomValueField<std::vector<std::string>>(
      kPathPrefixes, &PlaylistResolveRule::path_prefixes,
      GetPathPrefixesFromValue);
}

// static
PlaylistExclusions* PlaylistExclusions::GetInstance() {
  return base::Singleton<PlaylistExclusions>::get();
}

PlaylistExclusions::PlaylistExclusions() = default;

PlaylistExclusions::~PlaylistExclusions() = default;

void PlaylistExclusions::OnComponentReady(const base::FilePath& component_dir) {
  component_path_ = component_dir;

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     component_path_.AppendASCII(kPlaylistExclusionsFile)),
      base::BindOnce(&PlaylistExclusions::OnPlaylistExclusionsLoaded,
                     weak_factory_.GetWeakPtr()));
}

void PlaylistExclusions::OnPlaylistExclusionsLoaded(
    const std::string& contents) {
  rules_.clear();
  is_ready_ = false;

  // Bail out if the component file was not loaded.
  if (contents.empty()) {
    return;
  }

  std::optional<base::DictValue> root =
      base::JSONReader::ReadDict(contents, base::JSON_PARSE_RFC);
  // Skip malformed JSON payloads.
  if (!root) {
    return;
  }

  const base::ListValue* rules_list = root->FindList(kRules);
  // Skip payloads without a rules list.
  if (!rules_list) {
    return;
  }

  base::JSONValueConverter<PlaylistResolveRule> converter;
  for (const base::Value& rule_val : *rules_list) {
    PlaylistResolveRule rule;
    if (!converter.Convert(rule_val, &rule)) {
      continue;
    }
    if (rule.registrable_domain.empty()) {
      continue;
    }
    rules_.push_back(std::move(rule));
  }

  is_ready_ = true;
}

bool PlaylistExclusions::CanResolvePageSrcLater(const GURL& url) const {
  if (!is_ready_) {
    // We don't have the exclusions list loaded yet. To avoid breakage,
    // allow re-resolution for any page.
    return true;
  }

  const std::string domain =
      net::registry_controlled_domains::GetDomainAndRegistry(
          url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  const std::string_view path = url.path();

  for (const PlaylistResolveRule& rule : rules_) {
    if (rule.registrable_domain != domain) {
      continue;
    }
    if (rule.deny_root_path && (path.empty() || path == "/")) {
      return false;
    }
    for (const std::string& prefix : rule.path_prefixes) {
      if (prefix.empty()) {
        continue;
      }
      if (base::StartsWith(path, prefix)) {
        return false;
      }
    }
  }
  return true;
}

}  // namespace playlist
