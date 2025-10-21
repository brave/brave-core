// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/psst_rule_registry_impl.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/callback_forward.h"
#include "base/logging.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "base/task/thread_pool.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "brave/components/psst/browser/core/rule_data_reader.h"
#include "brave/components/psst/common/features.h"
#include "url/origin.h"

namespace psst {

namespace {

const base::FilePath::CharType kJsonFile[] = FILE_PATH_LITERAL("psst.json");

std::string ReadFile(const base::FilePath& file_path) {
  std::string contents;
  bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    VLOG(2) << "ReadFile: cannot " << "read file " << file_path;
  }
  return contents;
}

}  // namespace

// static
PsstRuleRegistry* PsstRuleRegistry::GetInstance() {
  static base::NoDestructor<PsstRuleRegistryImpl> instance;
  return instance.get();
}

PsstRuleRegistryImpl::PsstRuleRegistryImpl() = default;
PsstRuleRegistryImpl::~PsstRuleRegistryImpl() = default;

void PsstRuleRegistryImpl::CheckIfMatch(
    const GURL& url,
    base::OnceCallback<void(std::unique_ptr<MatchedRule>)> cb) {
  for (const PsstRule& rule : rules_) {
    if (rule.ShouldInsertScript(url)) {
      base::ThreadPool::PostTaskAndReplyWithResult(
          FROM_HERE, {base::MayBlock()},
          base::BindOnce(&MatchedRule::Create,
                         std::make_unique<RuleDataReader>(component_path_),
                         rule),
          std::move(cb));
      // Only ever find one matching rule.
      return;
    }
  }
}

void PsstRuleRegistryImpl::LoadRules(const base::FilePath& path,
                                     OnLoadCallback cb) {
  CHECK(base::FeatureList::IsEnabled(psst::features::kEnablePsst));
  component_path_ = path;

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadFile, path.Append(kJsonFile)),
      base::BindOnce(&PsstRuleRegistryImpl::OnLoadRules,
                     weak_factory_.GetWeakPtr(), std::move(cb)));
}

void PsstRuleRegistryImpl::OnLoadRules(OnLoadCallback cb,
                                       const std::string& contents) {
  LOG(INFO) << "[PSST] PsstRuleRegistryImpl::OnLoadRules contents: " << contents << "\ncb: " << cb.is_null();
  auto parsed_rules = PsstRule::ParseRules(contents);
  if (parsed_rules) {
    rules_ = std::move(parsed_rules.value());
  }

  if (!cb) {
    return;
  }

  std::move(cb).Run(contents, rules_);
}

}  // namespace psst
