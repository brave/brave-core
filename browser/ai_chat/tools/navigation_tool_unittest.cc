// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/navigation_tool.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/test_future.h"
#include "brave/browser/ai_chat/tools/mock_content_agent_task_provider.h"
#include "brave/components/ai_chat/core/browser/tools/tool_utils.h"
#include "chrome/browser/actor/browser_action_util.h"
#include "chrome/browser/actor/tools/navigate_tool_request.h"
#include "chrome/common/actor/task_id.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ai_chat {

class NavigationToolTest : public testing::Test {
 public:
  void SetUp() override {
    mock_task_provider_ = std::make_unique<MockContentAgentTaskProvider>();
    navigation_tool_ =
        std::make_unique<NavigationTool>(mock_task_provider_.get(),
                                         nullptr);  // Actor service not used

    test_tab_handle_ = tabs::TabHandle(123);
    test_task_id_ = actor::TaskId(456);

    mock_task_provider_->SetTaskId(test_task_id_);

    ON_CALL(*mock_task_provider_, GetOrCreateTabHandleForTask)
        .WillByDefault(base::test::RunOnceCallback<0>(test_tab_handle_));
  }

 protected:
  // Creates a valid navigation JSON with the given URL
  std::string CreateValidNavigationJson(const std::string& url) {
    base::Value::Dict dict;
    dict.Set("website_url", url);

    std::string json;
    base::JSONWriter::Write(dict, &json);
    return json;
  }

  void RunWithExpectedError(const std::string& input_json,
                            const std::string& expected_error) {
    // For error cases, the tool should not call the interesting task provider
    // methods Note: GetTaskId() may still be called as it's infrastructure, but
    // we don't care
    EXPECT_CALL(*mock_task_provider_, GetOrCreateTabHandleForTask).Times(0);
    EXPECT_CALL(*mock_task_provider_, ExecuteActions).Times(0);

    base::test::TestFuture<std::vector<mojom::ContentBlockPtr>> future;
    navigation_tool_->UseTool(input_json, future.GetCallback());

    auto result = future.Take();
    EXPECT_EQ(result.size(), 1u);
    ASSERT_TRUE(result[0]->is_text_content_block());
    EXPECT_EQ(result[0]->get_text_content_block()->text, expected_error);
  }

  // Verify navigation action properties and conversions
  void VerifyNavigationAction(const optimization_guide::proto::Actions& actions,
                              const std::string& expected_url) {
    // Verify proto action
    EXPECT_EQ(actions.task_id(), test_task_id_.value());
    EXPECT_EQ(actions.actions_size(), 1);

    const auto& action = actions.actions(0);
    EXPECT_TRUE(action.has_navigate());

    const auto& navigate_action = action.navigate();
    EXPECT_EQ(navigate_action.tab_id(), test_tab_handle_.raw_value());
    EXPECT_EQ(navigate_action.url(), GURL(expected_url).spec());

    // Verify CreateToolRequest works and produces correct NavigateToolRequest
    auto tool_request = actor::CreateToolRequest(action, nullptr);
    ASSERT_NE(tool_request, nullptr);

    auto* navigate_request =
        static_cast<actor::NavigateToolRequest*>(tool_request.get());
    EXPECT_EQ(navigate_request->GetTabHandle(), test_tab_handle_);
  }

  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<MockContentAgentTaskProvider> mock_task_provider_;
  std::unique_ptr<NavigationTool> navigation_tool_;
  tabs::TabHandle test_tab_handle_;
  actor::TaskId test_task_id_;
};

TEST_F(NavigationToolTest, ValidInputHttpsUrl) {
  std::string test_url = "https://www.example.com";
  std::string input_json = CreateValidNavigationJson(test_url);
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce([&captured_actions, &run_loop](
                    optimization_guide::proto::Actions actions,
                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        run_loop.Quit();
      });

  navigation_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify navigation action properties
  VerifyNavigationAction(captured_actions, test_url);
}

TEST_F(NavigationToolTest, ValidInputComplexUrl) {
  std::string test_url = "https://search.brave.com/search?q=test&source=web";
  std::string input_json = CreateValidNavigationJson(test_url);
  base::RunLoop run_loop;

  optimization_guide::proto::Actions captured_actions;
  EXPECT_CALL(*mock_task_provider_, ExecuteActions(testing::_, testing::_))
      .WillOnce([&captured_actions, &run_loop](
                    optimization_guide::proto::Actions actions,
                    Tool::UseToolCallback callback) {
        captured_actions = std::move(actions);
        std::move(callback).Run(CreateContentBlocksForText("Success"));
        run_loop.Quit();
      });

  navigation_tool_->UseTool(input_json, base::DoNothing());
  run_loop.Run();

  // Verify navigation action properties
  VerifyNavigationAction(captured_actions, test_url);
}

TEST_F(NavigationToolTest, InvalidJson) {
  RunWithExpectedError("{ invalid json }",
                       "Failed to parse input JSON. Please try again.");
}

TEST_F(NavigationToolTest, MissingWebsiteUrl) {
  std::string input_json = R"({})";

  RunWithExpectedError(input_json,
                       "Missing website_url parameter from input JSON.");
}

TEST_F(NavigationToolTest, InvalidUrlFormat) {
  std::string input_json = CreateValidNavigationJson("not_a_valid_url");

  RunWithExpectedError(input_json,
                       "website_url parameter did not contain a valid URL");
}

TEST_F(NavigationToolTest, NonHttpsUrl) {
  std::string input_json = CreateValidNavigationJson("http://www.example.com");

  RunWithExpectedError(input_json,
                       "website_url parameter must start with https://");
}

TEST_F(NavigationToolTest, FtpUrl) {
  std::string input_json = CreateValidNavigationJson("ftp://files.example.com");

  RunWithExpectedError(input_json,
                       "website_url parameter must start with https://");
}

TEST_F(NavigationToolTest, FileUrl) {
  std::string input_json = CreateValidNavigationJson("file:///local/path");

  RunWithExpectedError(input_json,
                       "website_url parameter must start with https://");
}

TEST_F(NavigationToolTest, InvalidUrlType) {
  std::string input_json = R"({
    "website_url": 123
  })";

  RunWithExpectedError(input_json,
                       "Missing website_url parameter from input JSON.");
}

TEST_F(NavigationToolTest, ToolMetadata) {
  // Note: NavigationTool uses mojom::kNavigateToolName which should be
  // "navigate" but we test whatever Name() returns
  EXPECT_FALSE(std::string(navigation_tool_->Name()).empty());
  EXPECT_FALSE(std::string(navigation_tool_->Description()).empty());

  auto properties = navigation_tool_->InputProperties();
  EXPECT_TRUE(properties.has_value());

  auto required = navigation_tool_->RequiredProperties();
  EXPECT_TRUE(required.has_value());
  EXPECT_EQ(required->size(), 1u);  // website_url
}

}  // namespace ai_chat
