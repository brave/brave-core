// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TODO_TOOL_H_
#define BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TODO_TOOL_H_

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "services/data_decoder/public/cpp/data_decoder.h"

namespace ai_chat {

// Todo management tool for tracking and organizing coding tasks
class TodoTool : public Tool {
 public:
  TodoTool();
  ~TodoTool() override;

  TodoTool(const TodoTool&) = delete;
  TodoTool& operator=(const TodoTool&) = delete;

  // Tool interface implementation
  std::string_view Name() const override;
  std::string_view Description() const override;
  std::optional<base::Value::Dict> InputProperties() const override;
  std::optional<std::vector<std::string>> RequiredProperties() const override;
  bool IsContentAssociationRequired() const override;
  bool RequiresUserInteractionBeforeHandling() const override;
  void UseTool(const std::string& input_json,
               UseToolCallback callback) override;

 private:
  struct TodoItem {
    std::string id;
    std::string content;
    std::string status;  // "pending", "in_progress", "completed", "cancelled"
  };

  // Internal state management
  std::vector<TodoItem> todos_;

  // JSON parsing callback
  void OnJsonParsed(UseToolCallback callback,
                    data_decoder::DataDecoder::ValueOrError result);
  void MergeTodos(const base::Value::List& new_todos_list);
  void ReplaceTodos(const base::Value::List& new_todos_list);
  base::Value::Dict CreateResponse() const;

  // Validation helpers
  bool ValidateTodoItem(const base::Value::Dict& todo_dict) const;
  bool IsValidStatus(const std::string& status) const;

  base::WeakPtrFactory<TodoTool> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CORE_BROWSER_TOOLS_TODO_TOOL_H_
