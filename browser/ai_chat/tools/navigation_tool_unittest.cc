// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_chat/tools/navigation_tool.h"

#include <memory>
#include <string>

#include "base/json/json_writer.h"
#include "brave/browser/ai_chat/tools/content_agent_tool_base_test.h"
#include "brave/browser/ai_chat/tools/target_test_util.h"
#include "chrome/browser/actor/browser_action_util.h"
#include "chrome/browser/actor/tools/navigate_tool_request.h"
#include "chrome/browser/actor/tools/page_tool_request.h"
#include "components/optimization_guide/proto/features/actions_data.pb.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class NavigationToolTest : public ContentAgentToolBaseTest {
 protected:
  std::unique_ptr<Tool> CreateTool() override {
    return std::make_unique<NavigationTool>(mock_task_provider_.get());
  }

  std::string CreateToolInputJson(const std::string& url) {
    base::Value::Dict dict;
    dict.Set("website_url", url);

    std::string json;
    base::JSONWriter::Write(dict, &json);
    return json;
  }

  void VerifySuccess(const std::string& input_json,
                     const std::string& expected_url) {
    GURL expected_gurl(expected_url);
    auto [action, tool_request] =
        RunWithExpectedSuccess(FROM_HERE, input_json, "Navigate");
    EXPECT_TRUE(action.has_navigate());

    const auto& navigate_action = action.navigate();
    EXPECT_EQ(navigate_action.tab_id(), test_tab_handle_.raw_value());
    EXPECT_EQ(navigate_action.url(), expected_gurl.spec());

    auto* navigate_request =
        static_cast<actor::NavigateToolRequest*>(tool_request.get());
    EXPECT_EQ(navigate_request->AssociatedOriginGrant(),
              url::Origin::Create(expected_gurl));
  }
};

TEST_F(NavigationToolTest, ValidInputHttpsUrl) {
  std::string test_url = "https://www.example.com";
  std::string input_json = CreateToolInputJson(test_url);

  VerifySuccess(input_json, test_url);
}

TEST_F(NavigationToolTest, ValidInputComplexUrl) {
  std::string test_url = "https://search.brave.com/search?q=test&source=web";
  std::string input_json = CreateToolInputJson(test_url);

  VerifySuccess(input_json, test_url);
}

TEST_F(NavigationToolTest, InvalidJson) {
  RunWithExpectedError(FROM_HERE, "{ invalid json }");
}

TEST_F(NavigationToolTest, MissingWebsiteUrl) {
  RunWithExpectedError(FROM_HERE, R"({})");
}

TEST_F(NavigationToolTest, InvalidUrlFormat) {
  RunWithExpectedError(FROM_HERE, CreateToolInputJson("not_a_valid_url"));
}

TEST_F(NavigationToolTest, NonHttpsUrl) {
  RunWithExpectedError(FROM_HERE,
                       CreateToolInputJson("http://www.example.com"));
}

TEST_F(NavigationToolTest, FtpUrl) {
  RunWithExpectedError(FROM_HERE,
                       CreateToolInputJson("ftp://files.example.com"));
}

TEST_F(NavigationToolTest, FileUrl) {
  RunWithExpectedError(FROM_HERE, CreateToolInputJson("file:///local/path"));
}

TEST_F(NavigationToolTest, InvalidUrlType) {
  RunWithExpectedError(FROM_HERE, R"({
    "website_url": 123
  })");
}

}  // namespace ai_chat
