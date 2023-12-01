// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/psst_rule_registry.h"

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
#include "brave/components/psst/browser/core/psst_rule.h"
#include "brave/components/psst/common/features.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace psst {

namespace {

const base::FilePath::CharType kJsonFile[] = FILE_PATH_LITERAL("psst.json");
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
                              const base::FilePath& test_script_path,
                              const base::FilePath& policy_script_path,
                              const int version) {
  auto prefix = base::FilePath(component_path).Append(kScriptsDir);
  auto test_script = ReadFile(base::FilePath(prefix).Append(test_script_path));
  auto policy_script =
      ReadFile(base::FilePath(prefix).Append(policy_script_path));
  return {test_script, policy_script, version};
}

}  // namespace

// static
PsstRuleRegistry* PsstRuleRegistry::GetInstance() {
  // Check if feature flag is enabled.
  if (!base::FeatureList::IsEnabled(psst::features::kBravePsst)) {
    return nullptr;
  }
  return base::Singleton<PsstRuleRegistry>::get();
}

PsstRuleRegistry::PsstRuleRegistry() = default;

PsstRuleRegistry::~PsstRuleRegistry() = default;

void PsstRuleRegistry::CheckIfMatch(
    const GURL& url,
    base::OnceCallback<void(MatchedRule)> cb) const {
  for (const PsstRule& rule : rules_) {
    if (rule.ShouldInsertScript(url)) {
      base::ThreadPool::PostTaskAndReplyWithResult(
          FROM_HERE, {base::MayBlock()},
          base::BindOnce(&CreateMatchedRule, component_path_,
                         rule.GetTestScript(), rule.GetPolicyScript(),
                         rule.GetVersion()),
          std::move(cb));
      // Only ever find one matching rule.
      return;
    }
  }
}

void PsstRuleRegistry::LoadRules(const base::FilePath& path) {
  SetComponentPath(path);
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadFile, path.Append(kJsonFile)),
      base::BindOnce(&PsstRuleRegistry::OnLoadRules,
                     weak_factory_.GetWeakPtr()));
}

void PsstRuleRegistry::SetComponentPath(const base::FilePath& path) {
  component_path_ = path;
}

void PsstRuleRegistry::OnLoadRules(const std::string& contents) {
  auto parsed_rules = PsstRule::ParseRules(contents);
  if (!parsed_rules) {
    return;
  }
  rules_ = std::move(parsed_rules.value());
}

}  // namespace psst
