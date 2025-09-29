// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/content_agent_tool_base_test.h"

#include <memory>
#include <string>

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/tools/content_agent_tool_base_test.h"
#include "brave/browser/ai_chat/tools/mock_content_agent_task_provider.h"
#include "brave/components/ai_chat/core/common/test_utils.h"
#include "chrome/browser/actor/browser_action_util.h"
#include "chrome/common/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "components/tabs/public/tab_interface.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

ContentAgentToolBaseTest::ContentAgentToolBaseTest() = default;
ContentAgentToolBaseTest::~ContentAgentToolBaseTest() = default;

void ContentAgentToolBaseTest::SetUp() {
  mock_task_provider_ = std::make_unique<MockContentAgentTaskProvider>();
  test_tab_handle_ = tabs::TabHandle(123);
  test_task_id_ = actor::TaskId(456);

  mock_task_provider_->SetTaskId(test_task_id_);

  ON_CALL(*mock_task_provider_, GetOrCreateTabHandleForTask)
      .WillByDefault(base::test::RunOnceCallback<0>(test_tab_handle_));

  tool_ = CreateTool();
}

std::tuple<optimization_guide::proto::Action,
           std::unique_ptr<actor::ToolRequest>>
ContentAgentToolBaseTest::RunWithExpectedSuccess(
    const base::Location& location,
    const std::string& input_json,
    const std::string& expected_tool_name,
    bool uses_tab) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  base::RunLoop run_loop;
  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions)
      .WillOnce(testing::Invoke([&captured_actions, &run_loop](
                                    optimization_guide::proto::Actions actions,
                                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        run_loop.Quit();
      }));

  tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();
  // Verify proto action
  EXPECT_EQ(captured_actions.task_id(), test_task_id_.value());
  EXPECT_EQ(captured_actions.actions_size(), 1);

  const auto& action = captured_actions.actions(0);

  // Verify CreateToolRequest works and produces correct ClickToolRequest
  auto tool_request = actor::CreateToolRequest(action, nullptr);
  EXPECT_NE(tool_request, nullptr);
  EXPECT_EQ(tool_request->JournalEvent(), expected_tool_name);

  if (uses_tab) {
    EXPECT_EQ(tool_request->GetTabHandle(), test_tab_handle_);
  } else {
    EXPECT_EQ(tool_request->GetTabHandle(), tabs::TabHandle::Null());
  }
  return std::make_tuple(action, std::move(tool_request));
}

void ContentAgentToolBaseTest::RunWithExpectedError(
    const base::Location& location,
    const std::string& input_json,
    const std::string& expected_error) {
  SCOPED_TRACE(testing::Message() << location.ToString());
  // For error cases, the tool should not call the interesting task provider
  // methods Note: GetTaskId() may still be called as it's infrastructure, but
  // we don't care
  EXPECT_CALL(*mock_task_provider_, GetOrCreateTabHandleForTask).Times(0);
  EXPECT_CALL(*mock_task_provider_, ExecuteActions).Times(0);

  base::test::TestFuture<std::vector<mojom::ContentBlockPtr>> future;
  tool_->UseTool(input_json, future.GetCallback());

  auto result = future.Take();
  EXPECT_THAT(result, ContentBlockText(testing::HasSubstr(expected_error)));
}

}  // namespace ai_chat
