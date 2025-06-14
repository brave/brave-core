/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_user_agent/browser/brave_user_agent_exceptions.h"

#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <thread>
#include <dlfcn.h>

#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/strings/string_split.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_user_agent/browser/brave_user_agent_component_installer.h"
#include "brave/components/brave_user_agent/common/features.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "base/no_destructor.h"

constexpr char kBraveUserAgentExceptionsFile[] = "brave-checks.txt";

namespace brave_user_agent {

// static
BraveUserAgentExceptions* BraveUserAgentExceptions::GetInstance() {
  // Check if feature flag is enabled.
  if (!base::FeatureList::IsEnabled(
          brave_user_agent::features::kUseBraveUserAgent)) {
    return nullptr;
  }
  static base::NoDestructor<BraveUserAgentExceptions> instance;
  void* fn_addr = reinterpret_cast<void*>(reinterpret_cast<void (*)()>(&BraveUserAgentExceptions::GetInstance));
  Dl_info info;
  dladdr(fn_addr, &info);
  std::cout << "GetInstance in " << (info.dli_fname ? info.dli_fname : "unknown") << std::endl;
  std::cout << "BraveUserAgentExceptions::GetInstance" << ", instance="
            << instance.get() << ", pid=" << getpid()
            << ", thread=" << std::this_thread::get_id() << std::endl;
  return instance.get();
}

BraveUserAgentExceptions::BraveUserAgentExceptions() = default;

BraveUserAgentExceptions::~BraveUserAgentExceptions() {
  excepted_domains_.clear();
}

void BraveUserAgentExceptions::OnExceptedDomainsLoaded(
    const std::string& contents) {
  if (contents.empty()) {
    // We don't have the file yet.
    return;
  }
  std::vector<std::string> lines = base::SplitString(
      contents, "\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  excepted_domains_.insert(lines.begin(), lines.end());
  is_ready_ = true;
  return;
}

void BraveUserAgentExceptions::OnComponentReady(const base::FilePath& path) {
  component_path_ = path;
  std::cout << "BraveUserAgentExceptions::OnComponentReady"
            << ", path=" << path << ", this=" << this << std::endl;
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(
          &brave_component_updater::GetDATFileAsString,
          component_path_.AppendASCII(kBraveUserAgentExceptionsFile)),
      base::BindOnce(&BraveUserAgentExceptions::OnExceptedDomainsLoaded,
                     weak_factory_.GetWeakPtr()));
}

bool BraveUserAgentExceptions::CanShowBrave(const GURL& url) {
  Dl_info info;
  void* fn_addr = reinterpret_cast<void*>(reinterpret_cast<void (*)()>(&BraveUserAgentExceptions::GetInstance));
  dladdr(fn_addr, &info);
  std::cout << "CanShowBrave in " << (info.dli_fname ? info.dli_fname : "unknown") << std::endl;
  std::cout << "CanShowBrave" << ", url=" << url << ", this=" << this
            << ", pid=" << getpid() << ", thread=" << std::this_thread::get_id()
            << ", is_ready_=" << (is_ready_ ? "true" : "false") << std::endl;
  if (!is_ready_) {
    // We don't have the exceptions list loaded yet. To avoid breakage,
    // show Brave for any website.
    return true;
  }

  std::string domain = net::registry_controlled_domains::GetDomainAndRegistry(
      url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);

  std::cout << "domain=" << domain << std::endl;

  // Show Brave only if the domain is not on the exceptions list.
  return !base::Contains(excepted_domains_, domain);
}

}  // namespace brave_user_agent
