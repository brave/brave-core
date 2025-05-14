// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/psst_rule_registry.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

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
  bool success = base::ReadFileToString(file_path, &contents);
  if (!success || contents.empty()) {
    VLOG(2) << "ReadFile: cannot " << "read file " << file_path;
  }
  return contents;
}

std::optional<MatchedRule> CreateMatchedRule(
    base::WeakPtr<PsstRuleRegistryImpl> registry,
    RuleDataReader* rule_data_reader,
    const PsstRule& rule) {
  if (!registry) {
    return std::nullopt;
  }
  return MatchedRuleFactory(rule_data_reader, rule.Name(), rule.Version())
      .Create(rule);
}

}  // namespace

// static
PsstRuleRegistryAccessor* PsstRuleRegistryAccessor::GetInstance() {
  return base::Singleton<PsstRuleRegistryAccessor>::get();
}

PsstRuleRegistryAccessor::PsstRuleRegistryAccessor()
    : impl_(std::make_unique<PsstRuleRegistryImpl>()) {}
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
    base::OnceCallback<void(const std::optional<MatchedRule>&)> cb) {
  for (const PsstRule& rule : rules_) {
    if (rule.ShouldInsertScript(url)) {
      base::ThreadPool::PostTaskAndReplyWithResult(
          FROM_HERE, {base::MayBlock()},
          base::BindOnce(&CreateMatchedRule, weak_factory_.GetWeakPtr(),
                         rule_data_reader_.get(), rule),
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
  if (!rule_data_reader_) {
    rule_data_reader_ = std::make_unique<RuleDataReader>(path);
  }

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
