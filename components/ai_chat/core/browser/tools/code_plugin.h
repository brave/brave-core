// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_CODE_PLUGIN_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_CODE_PLUGIN_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/values.h"

namespace ai_chat {

// Interface for code execution plugins that provide additional utilities
// to the JavaScript execution environment.
class CodePlugin {
 public:
  virtual ~CodePlugin();

  // Description of the plugin's capabilities for the tool description
  virtual std::string_view Description() const = 0;

  // Keyword that triggers inclusion of this plugin's setup script
  virtual std::string_view InclusionKeyword() const = 0;

  // JavaScript setup script to inject into the execution environment
  virtual std::string_view SetupScript() const = 0;

  // Validates the output from script execution. Returns an error message
  // if validation fails, or std::nullopt if validation succeeds.
  virtual std::optional<std::string> ValidateOutput(
      const base::Value::Dict& output) const;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_CODE_PLUGIN_H_
