// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/tools/todo_tool.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class TodoToolTest : public testing::Test {
 public:
  void SetUp() override { todo_tool_ = std::make_unique<TodoTool>(); }

  void TearDown() override { todo_tool_.reset(); }

 protected:
  base::test::TaskEnvironment task_environment_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  std::unique_ptr<TodoTool> todo_tool_;

  // Helper method to run tool and capture result
  std::vector<mojom::ContentBlockPtr> RunTool(const std::string& input_json) {
    base::RunLoop run_loop;
    std::vector<mojom::ContentBlockPtr> result;

    todo_tool_->UseTool(input_json,
                        base::BindOnce(
                            [](base::RunLoop* run_loop,
                               std::vector<mojom::ContentBlockPtr>* result,
                               std::vector<mojom::ContentBlockPtr> output) {
                              *result = std::move(output);
                              run_loop->Quit();
                            },
                            &run_loop, &result));

    run_loop.Run();
    return result;
  }

  // Helper to get text content from result
  std::string GetResultText(const std::vector<mojom::ContentBlockPtr>& result) {
    if (result.empty() || !result[0]->is_text_content_block()) {
      return "";
    }
    return result[0]->get_text_content_block()->text;
  }

  // Helper to parse JSON result
  base::Value::Dict ParseResultAsDict(
      const std::vector<mojom::ContentBlockPtr>& result) {
    std::string text = GetResultText(result);
    auto parsed = base::JSONReader::Read(text);
    if (!parsed || !parsed->is_dict()) {
      return base::Value::Dict();
    }
    return std::move(parsed->GetDict());
  }
};

TEST_F(TodoToolTest, BasicProperties) {
  EXPECT_EQ(todo_tool_->Name(), "todo_write");
  EXPECT_FALSE(todo_tool_->Description().empty());
  EXPECT_FALSE(todo_tool_->RequiresUserInteractionBeforeHandling());

  auto props = todo_tool_->InputProperties();
  EXPECT_TRUE(props.has_value());

  auto required = todo_tool_->RequiredProperties();
  EXPECT_TRUE(required.has_value());
  EXPECT_EQ(required->size(), 2u);
  EXPECT_TRUE(std::find(required->begin(), required->end(), "merge") !=
              required->end());
  EXPECT_TRUE(std::find(required->begin(), required->end(), "todos") !=
              required->end());
}

TEST_F(TodoToolTest, CreateNewTodoList) {
  std::string input = R"({
    "merge": false,
    "todos": [
      {"id": "task1", "content": "First task", "status": "pending"},
      {"id": "task2", "content": "Second task", "status": "in_progress"}
    ]
  })";

  auto result = RunTool(input);
  EXPECT_EQ(result.size(), 1u);

  auto response = ParseResultAsDict(result);
  EXPECT_EQ(*response.FindString("status"), "success");
  EXPECT_EQ(*response.FindInt("total_todos"), 2);

  const base::Value::List* todos = response.FindList("current_todos");
  ASSERT_TRUE(todos);
  EXPECT_EQ(todos->size(), 2u);

  const base::Value::Dict& first_todo = (*todos)[0].GetDict();
  EXPECT_EQ(*first_todo.FindString("id"), "task1");
  EXPECT_EQ(*first_todo.FindString("content"), "First task");
  EXPECT_EQ(*first_todo.FindString("status"), "pending");
}

TEST_F(TodoToolTest, MergeTodoList) {
  // First, create initial todos
  std::string initial_input = R"({
    "merge": false,
    "todos": [
      {"id": "task1", "content": "Original task", "status": "pending"},
      {"id": "task2", "content": "Another task", "status": "completed"}
    ]
  })";
  RunTool(initial_input);

  // Then merge with updates
  std::string merge_input = R"({
    "merge": true,
    "todos": [
      {"id": "task1", "content": "Updated task", "status": "in_progress"},
      {"id": "task3", "content": "New task", "status": "pending"}
    ]
  })";

  auto result = RunTool(merge_input);
  auto response = ParseResultAsDict(result);

  EXPECT_EQ(*response.FindInt("total_todos"), 3);

  const base::Value::List* todos = response.FindList("current_todos");
  ASSERT_TRUE(todos);
  EXPECT_EQ(todos->size(), 3u);

  // Check that task1 was updated
  bool found_updated_task = false;
  bool found_preserved_task = false;
  bool found_new_task = false;

  for (const auto& todo : *todos) {
    const base::Value::Dict& todo_dict = todo.GetDict();
    const std::string* id = todo_dict.FindString("id");

    if (*id == "task1") {
      EXPECT_EQ(*todo_dict.FindString("content"), "Updated task");
      EXPECT_EQ(*todo_dict.FindString("status"), "in_progress");
      found_updated_task = true;
    } else if (*id == "task2") {
      EXPECT_EQ(*todo_dict.FindString("content"), "Another task");
      EXPECT_EQ(*todo_dict.FindString("status"), "completed");
      found_preserved_task = true;
    } else if (*id == "task3") {
      EXPECT_EQ(*todo_dict.FindString("content"), "New task");
      EXPECT_EQ(*todo_dict.FindString("status"), "pending");
      found_new_task = true;
    }
  }

  EXPECT_TRUE(found_updated_task);
  EXPECT_TRUE(found_preserved_task);
  EXPECT_TRUE(found_new_task);
}

TEST_F(TodoToolTest, ValidateAllStatusTypes) {
  std::string input = R"({
    "merge": false,
    "todos": [
      {"id": "task1", "content": "Pending task", "status": "pending"},
      {"id": "task2", "content": "In progress task", "status": "in_progress"},
      {"id": "task3", "content": "Completed task", "status": "completed"},
      {"id": "task4", "content": "Cancelled task", "status": "cancelled"}
    ]
  })";

  auto result = RunTool(input);
  auto response = ParseResultAsDict(result);

  EXPECT_EQ(*response.FindString("status"), "success");
  EXPECT_EQ(*response.FindInt("total_todos"), 4);
}

TEST_F(TodoToolTest, ErrorInvalidJSON) {
  std::string invalid_json = R"({"merge": false, "todos": [}})";

  auto result = RunTool(invalid_json);
  EXPECT_EQ(result.size(), 1u);

  std::string error_text = GetResultText(result);
  EXPECT_TRUE(error_text.find("Error: Invalid JSON input") !=
              std::string::npos);
}

TEST_F(TodoToolTest, ErrorNotJSONObject) {
  std::string input = R"(["not", "an", "object"])";

  auto result = RunTool(input);
  std::string error_text = GetResultText(result);
  EXPECT_TRUE(error_text.find("Error: Invalid JSON input") !=
              std::string::npos);
}

TEST_F(TodoToolTest, ErrorMissingMergeParameter) {
  std::string input = R"({
    "todos": [
      {"id": "task1", "content": "Task", "status": "pending"}
    ]
  })";

  auto result = RunTool(input);
  std::string error_text = GetResultText(result);
  EXPECT_TRUE(error_text.find("Error: 'merge' parameter is required") !=
              std::string::npos);
}

TEST_F(TodoToolTest, ErrorMissingTodosParameter) {
  std::string input = R"({
    "merge": false
  })";

  auto result = RunTool(input);
  std::string error_text = GetResultText(result);
  EXPECT_TRUE(error_text.find("Error: 'todos' parameter is required") !=
              std::string::npos);
}

TEST_F(TodoToolTest, ErrorTodosNotArray) {
  std::string input = R"({
    "merge": false,
    "todos": "not an array"
  })";

  auto result = RunTool(input);
  std::string error_text = GetResultText(result);
  EXPECT_TRUE(
      error_text.find(
          "Error: 'todos' parameter is required and must be an array") !=
      std::string::npos);
}

TEST_F(TodoToolTest, ErrorInvalidTodoItem) {
  std::string input = R"({
    "merge": false,
    "todos": [
      {"id": "task1", "content": "Valid task", "status": "pending"},
      {"id": "", "content": "Invalid empty id", "status": "pending"}
    ]
  })";

  auto result = RunTool(input);
  std::string error_text = GetResultText(result);
  EXPECT_TRUE(error_text.find("Error: Invalid todo item format") !=
              std::string::npos);
}

TEST_F(TodoToolTest, ErrorMissingTodoFields) {
  std::string input = R"({
    "merge": false,
    "todos": [
      {"id": "task1", "status": "pending"},
      {"id": "task2", "content": "Task 2", "status": "pending"}
    ]
  })";

  auto result = RunTool(input);
  std::string error_text = GetResultText(result);
  EXPECT_TRUE(error_text.find("Error: Invalid todo item format") !=
              std::string::npos);
}

TEST_F(TodoToolTest, ErrorInvalidStatus) {
  std::string input = R"({
    "merge": false,
    "todos": [
      {"id": "task1", "content": "Task 1", "status": "pending"},
      {"id": "task2", "content": "Task 2", "status": "invalid_status"}
    ]
  })";

  auto result = RunTool(input);
  std::string error_text = GetResultText(result);
  EXPECT_TRUE(error_text.find("Error: Invalid todo item format") !=
              std::string::npos);
}

TEST_F(TodoToolTest, ErrorNotEnoughTodosForNewList) {
  std::string input = R"({
    "merge": false,
    "todos": [
      {"id": "task1", "content": "Only one task", "status": "pending"}
    ]
  })";

  auto result = RunTool(input);
  std::string error_text = GetResultText(result);
  EXPECT_TRUE(error_text.find("Error: At least 2 todo items are required") !=
              std::string::npos);
}

TEST_F(TodoToolTest, MergeWithSingleTodoAllowed) {
  // First create initial list
  std::string initial_input = R"({
    "merge": false,
    "todos": [
      {"id": "task1", "content": "First task", "status": "pending"},
      {"id": "task2", "content": "Second task", "status": "pending"}
    ]
  })";
  RunTool(initial_input);

  // Then merge with single todo (should be allowed)
  std::string merge_input = R"({
    "merge": true,
    "todos": [
      {"id": "task1", "content": "Updated task", "status": "completed"}
    ]
  })";

  auto result = RunTool(merge_input);
  auto response = ParseResultAsDict(result);
  EXPECT_EQ(*response.FindString("status"), "success");
  EXPECT_EQ(*response.FindInt("total_todos"), 2);
}

TEST_F(TodoToolTest, HandleTodoItemNotDict) {
  std::string input = R"({
    "merge": false,
    "todos": [
      {"id": "task1", "content": "Valid task", "status": "pending"},
      "not a dict"
    ]
  })";

  auto result = RunTool(input);
  std::string error_text = GetResultText(result);
  EXPECT_TRUE(error_text.find("Error: Invalid todo item format") !=
              std::string::npos);
}

TEST_F(TodoToolTest, EmptyTodosList) {
  // Create initial list first
  std::string initial_input = R"({
    "merge": false,
    "todos": [
      {"id": "task1", "content": "Task to remove", "status": "pending"},
      {"id": "task2", "content": "Another task", "status": "pending"}
    ]
  })";
  RunTool(initial_input);

  // Replace with empty list
  std::string empty_input = R"({
    "merge": false,
    "todos": []
  })";

  auto result = RunTool(empty_input);
  std::string error_text = GetResultText(result);
  EXPECT_TRUE(error_text.find("Error: At least 2 todo items are required") !=
              std::string::npos);
}

TEST_F(TodoToolTest, MergeIgnoresInvalidItems) {
  // Create initial list
  std::string initial_input = R"({
    "merge": false,
    "todos": [
      {"id": "task1", "content": "Original task", "status": "pending"},
      {"id": "task2", "content": "Another task", "status": "pending"}
    ]
  })";
  RunTool(initial_input);

  // The merge validation should catch invalid items before processing
  std::string merge_input = R"({
    "merge": true,
    "todos": [
      {"id": "task1", "content": "Updated task", "status": "completed"},
      {"content": "Missing ID", "status": "pending"}
    ]
  })";

  auto result = RunTool(merge_input);
  std::string error_text = GetResultText(result);
  EXPECT_TRUE(error_text.find("Error: Invalid todo item format") !=
              std::string::npos);
}

TEST_F(TodoToolTest, ReplaceIgnoresInvalidItems) {
  // Create initial list
  std::string initial_input = R"({
    "merge": false,
    "todos": [
      {"id": "task1", "content": "Original task", "status": "pending"},
      {"id": "task2", "content": "Another task", "status": "pending"}
    ]
  })";
  RunTool(initial_input);

  // The replace validation should catch invalid items before processing
  std::string replace_input = R"({
    "merge": false,
    "todos": [
      {"id": "task1", "content": "Valid task", "status": "completed"},
      {"content": "Missing ID", "status": "pending"}
    ]
  })";

  auto result = RunTool(replace_input);
  std::string error_text = GetResultText(result);
  EXPECT_TRUE(error_text.find("Error: Invalid todo item format") !=
              std::string::npos);
}

}  // namespace ai_chat
