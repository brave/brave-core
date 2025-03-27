// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/psst_rule_registry.h"

#include <iostream>
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
#include "base/memory/weak_ptr.h"
#include "base/task/thread_pool.h"
#include "brave/components/psst/browser/core/matched_rule.h"
#include "brave/components/psst/browser/core/psst_rule.h"
#include "brave/components/psst/browser/core/rule_data_reader.h"
#include "brave/components/psst/common/features.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace psst {

namespace {

const base::FilePath::CharType kJsonFile[] = FILE_PATH_LITERAL("psst.json");

std::string ReadFile(const base::FilePath& file_path) {
  std::string contents;
  LOG(INFO) << "[PSST] Read File #100 file_path:" << file_path;
  bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    VLOG(2) << "ReadFile: cannot " << "read file " << file_path;
  }
  return contents;
}

std::optional<MatchedRule> CreateMatchedRule(
    std::unique_ptr<MatchedRuleFactory> factory,
    const PsstRule& rule) {
  return factory->Create(rule);
}

}  // namespace

// static
PsstRuleRegistryAccessor* PsstRuleRegistryAccessor::GetInstance() {
  return base::Singleton<PsstRuleRegistryAccessor>::get();
}

PsstRuleRegistryAccessor::PsstRuleRegistryAccessor()
: impl_(std::make_unique<PsstRuleRegistryImpl>()) {

}
PsstRuleRegistryAccessor::~PsstRuleRegistryAccessor() = default;

PsstRuleRegistry* PsstRuleRegistryAccessor::Registry() {
  if (!base::FeatureList::IsEnabled(psst::features::kBravePsst)) {
    return nullptr;
  }

  return impl_.get();
}

void PsstRuleRegistryAccessor::SetRegistryForTesting(
    std::unique_ptr<PsstRuleRegistry> new_inst) {
  impl_ = std::move(new_inst);
}

PsstRuleRegistryImpl::PsstRuleRegistryImpl() = default;

PsstRuleRegistryImpl::~PsstRuleRegistryImpl() = default;

void PsstRuleRegistryImpl::CheckIfMatch(
    const GURL& url,
    base::OnceCallback<void(const std::optional<MatchedRule>&)> cb) const {
//LOG(INFO) << "[PSST] PsstRuleRegistryImpl::CheckIfMatch url:" << url << " rules_count:" << rules_.size();
  for (const PsstRule& rule : rules_) {
//LOG(INFO) << "[PSST] rule:" << rule.Name();
    if (rule.ShouldInsertScript(url)) {
      base::ThreadPool::PostTaskAndReplyWithResult(
          FROM_HERE, {base::MayBlock()},
          base::BindOnce(
              &CreateMatchedRule,
              std::make_unique<MatchedRuleFactory>(rule_data_reader_.get(),
                                                   rule.Name(), rule.Version()),
              rule),
          std::move(cb));
      // Only ever find one matching rule.
      return;
    }
  }
}

void PsstRuleRegistryImpl::SetRuleDataReaderForTest(
    std::unique_ptr<RuleDataReader> rule_data_reader) {
  rule_data_reader_ = std::move(rule_data_reader);
}

void PsstRuleRegistryImpl::LoadRules(const base::FilePath& path) {
LOG(INFO) << "[PSST] LoadRules #100";
  if (!rule_data_reader_) {
    rule_data_reader_ = std::make_unique<RuleDataReader>(path);
  }

LOG(INFO) << "[PSST] LoadRules #200";
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadFile, path.Append(kJsonFile)),
      base::BindOnce(&PsstRuleRegistryImpl::OnLoadRules,
                     weak_factory_.GetWeakPtr()));
}

void PsstRuleRegistryImpl::OnLoadRules(const std::string& contents) {
  auto parsed_rules = PsstRule::ParseRules(contents);
  if (!parsed_rules) {
    return;
  }
  rules_ = std::move(parsed_rules.value());
}

}  // namespace psst
