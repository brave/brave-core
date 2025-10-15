// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/todo_tool.h"

#include <algorithm>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "brave/components/ai_chat/core/browser/tools/tool_input_properties.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "services/data_decoder/public/cpp/data_decoder.h"

namespace ai_chat {

TodoTool::TodoTool() = default;

TodoTool::~TodoTool() = default;

std::string_view TodoTool::Name() const {
  return mojom::kTodoToolName;
}

std::string_view TodoTool::Description() const {
  return "Creates, updates, or manages a structured task list for tracking "
         "AI assistant task work progress. Use this tool to organize complex "
         "multi-step tasks, demonstrate thoroughness, and provide visibility "
         "into task progress. Do not use this tool if the task is not "
         "complex enough to break it down in to discrete steps with nice "
         "titles. The tool handles creating new todo lists, updating "
         "existing todos, and managing task status through pending, "
         "in_progress, completed, and cancelled states. The task list is "
         "reset on every new human message. Do not use this tool if the "
         "tasks are not expected to take more than a few seconds or there "
         "are fewer than 5 tasks. The user will see each other too's actions "
         "anyway so it will be duplicated if each task only equates to a "
         "single tool use by the assistant.";
}

std::optional<base::Value::Dict> TodoTool::InputProperties() const {
  return CreateInputProperties(
      {{"merge",
        BooleanProperty(
            "Whether to merge with existing todos. If true, updates existing "
            "todos based on ID and preserves others. If false, replaces the "
            "entire todo list.")},
       {"todos",
        ArrayProperty(
            "Array of todo items to write to the workspace (minimum 2 items)",
            ObjectProperty(
                "A todo item",
                {{"id", StringProperty("Unique identifier for the todo item")},
                 {"content",
                  StringProperty("Description/content of the todo item")},
                 {"status",
                  StringProperty("Current status of the todo item",
                                 std::vector<std::string>{
                                     "pending", "in_progress", "completed",
                                     "cancelled"})}}))}});
}

std::optional<std::vector<std::string>> TodoTool::RequiredProperties() const {
  return std::optional<std::vector<std::string>>({"merge", "todos"});
}

bool TodoTool::RequiresUserInteractionBeforeHandling() const {
  return false;
}

bool TodoTool::SupportsConversation(
    bool is_temporary,
    bool has_untrusted_content,
    mojom::ConversationCapability conversation_capability) const {
  // Until other conversation capability types get more powerful tools,
  // this tool is more of a hinderence in simple conversations.
  return conversation_capability ==
         mojom::ConversationCapability::CONTENT_AGENT;
}

void TodoTool::UseTool(const std::string& input_json,
                       UseToolCallback callback) {
  std::optional<base::Value::Dict> request_dict =
      base::JSONReader::ReadDict(input_json);

  if (!request_dict.has_value()) {
    // JSON parsing failed
    ToolResult error_result;
    error_result.push_back(mojom::ContentBlock::NewTextContentBlock(
        mojom::TextContentBlock::New("Error: Invalid JSON input")));
    std::move(callback).Run(std::move(error_result));
    return;
  }

  // Extract merge parameter
  std::optional<bool> merge = request_dict->FindBool("merge");
  if (!merge.has_value()) {
    ToolResult error_result;
    error_result.push_back(
        mojom::ContentBlock::NewTextContentBlock(mojom::TextContentBlock::New(
            "Error: 'merge' parameter is required and must be a boolean")));
    std::move(callback).Run(std::move(error_result));
    return;
  }

  // Extract todos array
  const base::Value::List* todos_list = request_dict->FindList("todos");
  if (!todos_list) {
    ToolResult error_result;
    error_result.push_back(
        mojom::ContentBlock::NewTextContentBlock(mojom::TextContentBlock::New(
            "Error: 'todos' parameter is required and must be an array")));
    std::move(callback).Run(std::move(error_result));
    return;
  }

  // Validate all todo items first
  for (const auto& todo_value : *todos_list) {
    const base::Value::Dict* todo_dict = todo_value.GetIfDict();
    if (!todo_dict || !ValidateTodoItem(*todo_dict)) {
      ToolResult error_result;
      error_result.push_back(
          mojom::ContentBlock::NewTextContentBlock(mojom::TextContentBlock::New(
              "Error: Invalid todo item format. Each todo must have 'id', "
              "'content', and valid 'status'")));
      std::move(callback).Run(std::move(error_result));
      return;
    }
  }

  // Invalid if it's a new list and less than 2 items
  if (!merge.value() && todos_list->size() < 2) {
    ToolResult error_result;
    error_result.push_back(
        mojom::ContentBlock::NewTextContentBlock(mojom::TextContentBlock::New(
            "Error: At least 2 todo items are required")));
    std::move(callback).Run(std::move(error_result));
    return;
  }

  // Process the todos
  if (merge.value()) {
    MergeTodos(*todos_list);
  } else {
    ReplaceTodos(*todos_list);
  }

  // Create and return response
  base::Value::Dict response = CreateResponse();
  std::string response_json;
  base::JSONWriter::Write(response, &response_json);

  ToolResult success_result;
  success_result.push_back(mojom::ContentBlock::NewTextContentBlock(
      mojom::TextContentBlock::New(response_json)));
  std::move(callback).Run(std::move(success_result));
}

void TodoTool::MergeTodos(const base::Value::List& new_todos_list) {
  for (const auto& todo_value : new_todos_list) {
    const base::Value::Dict* todo_dict = todo_value.GetIfDict();
    if (!todo_dict) {
      continue;
    }

    const std::string* id = todo_dict->FindString("id");
    const std::string* content = todo_dict->FindString("content");
    const std::string* status = todo_dict->FindString("status");

    if (!id || !content || !status) {
      continue;
    }

    // Find existing todo with same ID
    auto it =
        std::find_if(todos_.begin(), todos_.end(),
                     [&id](const TodoItem& item) { return item.id == *id; });

    if (it != todos_.end()) {
      // Update existing todo
      it->content = *content;
      it->status = *status;
    } else {
      // Add new todo
      todos_.push_back({*id, *content, *status});
    }
  }
}

void TodoTool::ReplaceTodos(const base::Value::List& new_todos_list) {
  todos_.clear();
  for (const auto& todo_value : new_todos_list) {
    const base::Value::Dict* todo_dict = todo_value.GetIfDict();
    if (!todo_dict) {
      continue;
    }

    const std::string* id = todo_dict->FindString("id");
    const std::string* content = todo_dict->FindString("content");
    const std::string* status = todo_dict->FindString("status");

    if (id && content && status) {
      todos_.push_back({*id, *content, *status});
    }
  }
}

base::Value::Dict TodoTool::CreateResponse() const {
  base::Value::Dict response;
  response.Set("status", "success");
  response.Set("total_todos", static_cast<int>(todos_.size()));

  base::Value::List todos_list;
  for (const auto& todo : todos_) {
    base::Value::Dict todo_dict;
    todo_dict.Set("id", todo.id);
    todo_dict.Set("content", todo.content);
    todo_dict.Set("status", todo.status);
    todos_list.Append(std::move(todo_dict));
  }
  response.Set("current_todos", std::move(todos_list));

  return response;
}

bool TodoTool::ValidateTodoItem(const base::Value::Dict& todo_dict) const {
  const std::string* id = todo_dict.FindString("id");
  const std::string* content = todo_dict.FindString("content");
  const std::string* status = todo_dict.FindString("status");

  if (!id || !content || !status) {
    return false;
  }

  if (id->empty() || content->empty()) {
    return false;
  }

  return IsValidStatus(*status);
}

bool TodoTool::IsValidStatus(const std::string& status) const {
  return status == "pending" || status == "in_progress" ||
         status == "completed" || status == "cancelled";
}

}  // namespace ai_chat
