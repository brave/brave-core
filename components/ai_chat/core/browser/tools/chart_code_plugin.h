// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_CHART_CODE_PLUGIN_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_CHART_CODE_PLUGIN_H_

#include <optional>
#include <string>
#include <string_view>

#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/code_plugin.h"

namespace ai_chat {

// Plugin that provides chart creation utilities for code execution.
// Enables creating line charts compatible with Recharts.
class ChartCodePlugin : public CodePlugin {
 public:
  ChartCodePlugin();
  ~ChartCodePlugin() override;

  ChartCodePlugin(const ChartCodePlugin&) = delete;
  ChartCodePlugin& operator=(const ChartCodePlugin&) = delete;

  static bool IsEnabled();

  // CodePlugin implementation
  std::string_view Description() const override;
  std::string_view InclusionKeyword() const override;
  std::string_view SetupScript() const override;
  std::optional<std::string> ValidateOutput(
      const base::Value::Dict& output) const override;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_CHART_CODE_PLUGIN_H_
