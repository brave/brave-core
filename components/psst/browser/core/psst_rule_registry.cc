// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/browser/core/psst_rule_registry.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/check_is_test.h"
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
    std::unique_ptr<RuleDataReader> rule_data_reader,
    const PsstRule& rule) {
  return MatchedRule::Create(std::move(rule_data_reader), rule);
}

}  // namespace

// static
PsstRuleRegistry* PsstRuleRegistry::GetInstance() {
  static base::NoDestructor<PsstRuleRegistry> instance;
  return instance.get();
}

void PsstRuleRegistry::SetOnLoadCallbackForTest(
    base::OnceCallback<void(const std::string&, const std::vector<PsstRule>&)>
        callback) {
  onload_test_callback_ = std::move(callback);
}

void PsstRuleRegistry::ResetRuleRegistryForTest() {
  rules_.clear();
  component_path_.clear();
  onload_test_callback_.reset();
}

PsstRuleRegistry::PsstRuleRegistry() = default;
PsstRuleRegistry::~PsstRuleRegistry() = default;

void PsstRuleRegistry::CheckIfMatch(
    const GURL& url,
    base::OnceCallback<void(const std::optional<MatchedRule>&)> cb) {
  for (const PsstRule& rule : rules_) {
    if (rule.ShouldInsertScript(url)) {
      base::ThreadPool::PostTaskAndReplyWithResult(
          FROM_HERE, {base::MayBlock()},
          base::BindOnce(&CreateMatchedRule,
                         std::make_unique<RuleDataReader>(component_path_),
                         rule),
          std::move(cb));
      // Only ever find one matching rule.
      return;
    }
  }
}

void PsstRuleRegistry::LoadRules(const base::FilePath& path) {
  if (path.empty() || !base::PathExists(path)) {
    return;
  }
  component_path_ = path;

  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&ReadFile, path.Append(kJsonFile)),
      base::BindOnce(&PsstRuleRegistry::OnLoadRules,
                     weak_factory_.GetWeakPtr()));
}

void PsstRuleRegistry::OnLoadRules(const std::string& contents) {
  auto parsed_rules = PsstRule::ParseRules(contents);
  if (parsed_rules) {
    rules_ = std::move(parsed_rules.value());
  }

  if (onload_test_callback_) {
    CHECK_IS_TEST();
    std::move(*onload_test_callback_).Run(contents, rules_);
  }
}

}  // namespace psst
