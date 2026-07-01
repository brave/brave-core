// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/web_mcp/core/browser/web_mcp_rule_registry.h"

#include <string>
#include <utility>

#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/task/thread_pool.h"

namespace web_mcp {

namespace {

// Subdirectory of the component holding one `.js` file per WebMCP tool.
constexpr base::FilePath::CharType kScriptsDir[] = FILE_PATH_LITERAL("scripts");

// Reads and parses every `*.js` file under `scripts_dir`. Runs on a blocking
// thread pool sequence.
std::vector<WebMcpInjectionRule> LoadRulesFromDir(
    const base::FilePath& scripts_dir) {
  std::vector<WebMcpInjectionRule> rules;
  base::FileEnumerator enumerator(scripts_dir, /*recursive=*/false,
                                  base::FileEnumerator::FILES,
                                  FILE_PATH_LITERAL("*.js"));
  for (base::FilePath path = enumerator.Next(); !path.empty();
       path = enumerator.Next()) {
    std::string contents;
    if (!base::ReadFileToString(path, &contents)) {
      continue;
    }
    if (auto rule = WebMcpInjectionRule::ParseScript(contents)) {
      rules.push_back(std::move(*rule));
    }
  }
  return rules;
}

}  // namespace

// static
WebMcpRuleRegistry* WebMcpRuleRegistry::GetInstance() {
  static base::NoDestructor<WebMcpRuleRegistry> instance;
  return instance.get();
}

WebMcpRuleRegistry::WebMcpRuleRegistry() = default;
WebMcpRuleRegistry::~WebMcpRuleRegistry() = default;

void WebMcpRuleRegistry::LoadRules(const base::FilePath& install_dir) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&LoadRulesFromDir, install_dir.Append(kScriptsDir)),
      base::BindOnce(&WebMcpRuleRegistry::OnRulesLoaded,
                     weak_factory_.GetWeakPtr()));
}

void WebMcpRuleRegistry::OnRulesLoaded(std::vector<WebMcpInjectionRule> rules) {
  rules_ = std::move(rules);
}

}  // namespace web_mcp
