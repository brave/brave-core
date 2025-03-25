/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_user_agent/browser/brave_user_agent_service.h"

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/strings/string_split.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_user_agent/browser/brave_user_agent_component_installer.h"
#include "brave/components/brave_user_agent/common/features.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

constexpr char kBraveUserAgentExceptionsFile[] = "brave-checks.txt";

namespace brave_user_agent {

// static
BraveUserAgentService* BraveUserAgentService::GetInstance() {
  // Check if feature flag is enabled.
  if (!base::FeatureList::IsEnabled(
          brave_user_agent::features::kUseBraveUserAgent)) {
    return nullptr;
  }
  return base::Singleton<BraveUserAgentService>::get();
}

BraveUserAgentService::BraveUserAgentService() = default;

BraveUserAgentService::~BraveUserAgentService() {
  exceptional_domains_.clear();
}

void BraveUserAgentService::OnExceptionalDomainsLoaded(
    const std::string& contents) {
  if (contents.empty()) {
    // We don't have the file yet.
    return;
  }
  std::vector<std::string> lines = base::SplitString(
      contents, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  exceptional_domains_.insert(lines.begin(), lines.end());
  is_ready_ = true;
  return;
}

void BraveUserAgentService::OnComponentReady(const base::FilePath& path) {
  component_path_ = path;

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(
          &brave_component_updater::GetDATFileAsString,
          component_path_.AppendASCII(kBraveUserAgentExceptionsFile)),
      base::BindOnce(&BraveUserAgentService::OnExceptionalDomainsLoaded,
                     weak_factory_.GetWeakPtr()));
}

bool BraveUserAgentService::CanShowBrave(const GURL& url) {
  if (!is_ready_) {
    // We don't have the exceptions list loaded yet. To avoid breakage,
    // show Brave for any website.
    return true;
  }

  std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
      url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  // Show Brave only if the domain is not on the exceptions list.
  return !base::Contains(exceptional_domains_, domain);
}

}  // namespace brave_user_agent
