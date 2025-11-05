// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_TOOLS_CONTENT_AGENT_TOOL_BASE_TEST_H_
#define BRAVE_BROWSER_AI_CHAT_TOOLS_CONTENT_AGENT_TOOL_BASE_TEST_H_

#include <memory>
#include <string>

#include "base/location.h"
#include "brave/browser/ai_chat/tools/mock_content_agent_task_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool.h"
#include "chrome/browser/actor/tools/tool_request.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

// This base class should be used by any Tool that uses the
// ContentAgentTaskProvider. It will create a mock ContentAgentTaskProvider,
// setup an actor Task and perform common verifications.
class ContentAgentToolBaseTest : public testing::Test {
 public:
  void SetUp() override;
  ~ContentAgentToolBaseTest() override;

 protected:
  ContentAgentToolBaseTest();

  // Test should implement this to create the tool to test
  virtual std::unique_ptr<Tool> CreateTool() = 0;

  // Run the UseTool method and verify ExecuteActions is called, capturing
  // the actions, verifying they can be converted to a ToolRequest, and
  // returning the action and tool request for further verification.
  std::tuple<optimization_guide::proto::Action,
             std::unique_ptr<actor::ToolRequest>>
  RunWithExpectedSuccess(const base::Location& location,
                         const std::string& input_json,
                         const std::string& expected_tool_name,
                         bool uses_tab = true);

  // Run the UseTool method and verify ExecuteActions is not called, and the
  // result is an error with the expected substring.
  void RunWithExpectedError(const base::Location& location,
                            const std::string& input_json,
                            const std::string& expected_error = "Error: ");

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<MockContentAgentTaskProvider> mock_task_provider_;
  std::unique_ptr<Tool> tool_;
  tabs::TabHandle test_tab_handle_;
  actor::TaskId test_task_id_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_TOOLS_CONTENT_AGENT_TOOL_BASE_TEST_H_
