// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/youtube_script_injector/browser/core/youtube_registry.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/callback_forward.h"
#include "base/logging.h"
#include "base/memory/singleton.h"
#include "base/task/thread_pool.h"
#include "brave/components/youtube_script_injector/browser/core/youtube_json.h"
#include "brave/components/youtube_script_injector/common/features.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace youtube_script_injector {

namespace {

const base::FilePath::CharType kJsonFile[] = FILE_PATH_LITERAL("youtube.json");
const base::FilePath::CharType kScriptsDir[] = FILE_PATH_LITERAL("scripts");

std::string ReadFile(const base::FilePath& file_path) {
  std::string contents;
  bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    VLOG(2) << "ReadFile: cannot "
            << "read file " << file_path;
  }
  return contents;
}

MatchedRule CreateMatchedRule(const base::FilePath& component_path,
                              const base::FilePath& policy_script_path,
                              const int version) {
  auto prefix = base::FilePath(component_path).Append(kScriptsDir);
  auto policy_script =
      ReadFile(base::FilePath(prefix).Append(policy_script_path));
  return {policy_script, version};
}

}  // namespace

// static
YouTubeRegistry* YouTubeRegistry::GetInstance() {
  // Check if feature flag is enabled.
  if (!base::FeatureList::IsEnabled(youtube_script_injector::features::kBraveYouTubeScriptInjector)) {
    return nullptr;
  }
  return base::Singleton<YouTubeRegistry>::get();
}

YouTubeRegistry::YouTubeRegistry() = default;

YouTubeRegistry::~YouTubeRegistry() = default;

void YouTubeRegistry::CheckIfMatch(
    const GURL& url,
    base::OnceCallback<void(MatchedRule)> cb) const {
  if (json_ && json_->IsYouTubeDomain(url)) {
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::MayBlock()},
        base::BindOnce(&CreateMatchedRule, component_path_,
                       json_->GetFeatureScript(),
                       json_->GetVersion()),
        std::move(cb));
  }
}

void YouTubeRegistry::LoadScripts(const base::FilePath& path) {
  SetComponentPath(path);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadFile, path.Append(kJsonFile)),
      base::BindOnce(&YouTubeRegistry::OnLoadScripts,
                     weak_factory_.GetWeakPtr()));
}

void YouTubeRegistry::OnLoadScripts(const std::string& contents) {
  json_ = YouTubeJson::ParseJson(contents);
}

void YouTubeRegistry::SetComponentPath(const base::FilePath& path) {
  component_path_ = path;
}

}  // namespace youtube_script_injector
