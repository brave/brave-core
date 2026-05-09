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

namespace playlist {

namespace {

constexpr char kPlaylistExclusionsFile[] = "playlist_exclusions.json";

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
PlaylistExclusions* PlaylistExclusions::GetInstance() {
  return base::Singleton<PlaylistExclusions>::get();
}

PlaylistExclusions::PlaylistExclusions() = default;

PlaylistExclusions::~PlaylistExclusions() = default;

void PlaylistExclusions::ResetForTesting() {
  rules_.clear();
  is_ready_ = false;
  component_path_.clear();
}

void PlaylistExclusions::OnComponentReady(const base::FilePath& component_dir) {
  component_path_ = component_dir;

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::GetDATFileAsString,
                     component_path_.AppendASCII(kPlaylistExclusionsFile)),
      base::BindOnce(&PlaylistExclusions::OnPlaylistExclusionsLoaded,
                     weak_factory_.GetWeakPtr()));
}

void PlaylistExclusions::OnPlaylistExclusionsLoaded(
    const std::string& contents) {
  rules_.clear();
  is_ready_ = false;

  if (contents.empty()) {
    return;
  }

  std::optional<base::DictValue> root =
      base::JSONReader::ReadDict(contents, base::JSON_PARSE_RFC);
  if (!root) {
    return;
  }

  const auto* rules_list = root->FindList("rules");
  if (!rules_list) {
    return;
  }

  for (const base::Value& rule_val : *rules_list) {
    const auto* rule_dict = rule_val.GetIfDict();
    if (!rule_dict) {
      continue;
    }
    const std::string* domain = rule_dict->FindString("registrable_domain");
    if (!domain || domain->empty()) {
      continue;
    }

    PlaylistResolveRule rule;
    rule.registrable_domain = *domain;
    rule.deny_root_path = rule_dict->FindBool("deny_root_path").value_or(false);

    const auto* prefixes = rule_dict->FindList("path_prefixes");
    if (prefixes) {
      for (const base::Value& p : *prefixes) {
        const std::string* ps = p.GetIfString();
        if (ps) {
          rule.path_prefixes.push_back(*ps);
        }
      }
    }
    rules_.push_back(std::move(rule));
  }

  is_ready_ = true;
}

bool PlaylistExclusions::CanResolvePageSrcLater(const GURL& url) const {
  if (!is_ready_) {
    return true;
  }

  if (!url.SchemeIsHTTPOrHTTPS()) {
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

std::vector<std::string> PlaylistExclusions::ListPlaylistExclusions() const {
  std::vector<std::string> out;
  for (const PlaylistResolveRule& rule : rules_) {
    if (rule.deny_root_path) {
      out.push_back(rule.registrable_domain + "\t<empty_or_root_path>");
    }
    for (const std::string& prefix : rule.path_prefixes) {
      out.push_back(rule.registrable_domain + "\t" + prefix);
    }
  }
  return out;
}

}  // namespace playlist
