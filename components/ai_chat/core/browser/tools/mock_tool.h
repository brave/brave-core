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

#include "base/functional/callback.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/mojom/common.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"

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
      std::optional<base::Value::Dict> extra_params = std::nullopt,
      bool requires_user_interaction_before_handling = false);
  ~MockTool() override;

  std::string_view Name() const override;
  std::string_view Description() const override;
  std::string_view Type() const override;
  std::optional<base::Value::Dict> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  std::optional<base::Value::Dict> ExtraParams() const override;
  bool RequiresUserInteractionBeforeHandling() const override;
  bool IsSupportedByModel(const mojom::Model& model) const override;
  bool SupportsConversation(
      bool is_temporary,
      bool has_untrusted_content,
      mojom::ConversationCapability conversation_capability) const override;

  void set_requires_user_interaction_before_handling(
      bool requires_user_interaction_before_handling) {
    requires_user_interaction_before_handling_ =
        requires_user_interaction_before_handling;
  }

  void set_is_supported_by_model(bool is_supported_by_model) {
    is_supported_by_model_ = is_supported_by_model;
  }

  void set_supports_conversation(bool supports_conversation) {
    supports_conversation_ = supports_conversation;
  }

  MOCK_METHOD(void,
              UseTool,
              (const std::string& input_json, UseToolCallback callback),
              (override));

 private:
  std::string name_;
  std::string description_;
  std::string type_;
  std::optional<base::Value::Dict> input_properties_;
  std::optional<std::vector<std::string>> required_properties_;
  std::optional<base::Value::Dict> extra_params_;
  bool requires_user_interaction_before_handling_;
  bool is_supported_by_model_ = true;
  bool supports_conversation_ = true;
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_MOCK_TOOL_H_
