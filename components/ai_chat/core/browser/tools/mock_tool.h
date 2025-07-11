// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_MOCK_TOOL_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_MOCK_TOOL_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"

namespace ai_chat {

// Mock Tool class for testing
class MockTool : public Tool {
 public:
  explicit MockTool(
      std::string_view name,
      std::string_view description = "",
      std::string_view type = "",
      std::optional<base::Value::Dict> input_properties = std::nullopt,
      std::optional<std::vector<std::string>> required_properties =
          std::nullopt,
      std::optional<base::Value::Dict> extra_params = std::nullopt);
  ~MockTool() override;

  std::string_view Name() const override;
  std::string_view Description() const override;
  std::string_view Type() const override;
  std::optional<base::Value::Dict> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  std::optional<base::Value::Dict> ExtraParams() const override;

 private:
  std::string name_;
  std::string description_;
  std::string type_;
  std::optional<base::Value::Dict> input_properties_;
  std::optional<std::vector<std::string>> required_properties_;
  std::optional<base::Value::Dict> extra_params_;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_MOCK_TOOL_H_
