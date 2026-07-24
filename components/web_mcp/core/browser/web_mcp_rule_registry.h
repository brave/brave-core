// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_WEB_MCP_CORE_BROWSER_WEB_MCP_RULE_REGISTRY_H_
#define BRAVE_COMPONENTS_WEB_MCP_CORE_BROWSER_WEB_MCP_RULE_REGISTRY_H_

#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "base/sequence_checker.h"
#include "brave/components/web_mcp/core/browser/web_mcp_injection_rule.h"

namespace base {
class FilePath;
}  // namespace base

namespace web_mcp {

// Process-wide registry of WebMCP injection rules. The rules are loaded from
// the WebMCP component's `scripts/` directory whenever the component updater
// delivers a new version (see WebMcpComponentInstallerPolicy::ComponentReady),
// and read synchronously by the per-tab WebMcpInjector at injection time.
//
// All access happens on the UI thread; only the file reads are off-thread.
class WebMcpRuleRegistry {
 public:
  static WebMcpRuleRegistry* GetInstance();

  WebMcpRuleRegistry(const WebMcpRuleRegistry&) = delete;
  WebMcpRuleRegistry& operator=(const WebMcpRuleRegistry&) = delete;

  // Asynchronously parses every `scripts/*.js` file under `install_dir` and
  // replaces the in-memory rule set with the result.
  void LoadRules(const base::FilePath& install_dir);

  // The currently loaded rules. Empty until the component has been delivered
  // and parsed.
  const std::vector<WebMcpInjectionRule>& rules() const {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    return rules_;
  }

 private:
  friend base::NoDestructor<WebMcpRuleRegistry>;

  WebMcpRuleRegistry();
  ~WebMcpRuleRegistry();

  void OnRulesLoaded(std::vector<WebMcpInjectionRule> rules);

  SEQUENCE_CHECKER(sequence_checker_);
  std::vector<WebMcpInjectionRule> rules_;
  base::WeakPtrFactory<WebMcpRuleRegistry> weak_factory_{this};
};

}  // namespace web_mcp

#endif  // BRAVE_COMPONENTS_WEB_MCP_CORE_BROWSER_WEB_MCP_RULE_REGISTRY_H_
