/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ai_chat/code_execution_tool.h"

#include <memory>
#include <string>
#include <vector>

#include "base/json/json_writer.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/test/bind.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::HasSubstr;

namespace ai_chat {

class AIChatCodeExecutionToolBrowserTest : public InProcessBrowserTest {
 public:
  AIChatCodeExecutionToolBrowserTest() = default;
  ~AIChatCodeExecutionToolBrowserTest() override = default;

  void SetUpOnMainThread() override {
    InProcessBrowserTest::SetUpOnMainThread();
    http_server_.RegisterRequestHandler(base::BindRepeating(
        &AIChatCodeExecutionToolBrowserTest::HandleTestRequest,
        base::Unretained(this)));
    ASSERT_TRUE(http_server_.Start());
    tool_ = std::make_unique<CodeExecutionTool>(browser()->profile());
  }

  void TearDownOnMainThread() override {
    tool_.reset();
    InProcessBrowserTest::TearDownOnMainThread();
  }

  std::string test_server_url() const {
    return http_server_.GetURL("/test").spec();
  }

  void ExecuteCode(const std::string& script, std::string* output) {
    base::Value::Dict input;
    input.Set("script", script);
    std::string input_json;
    base::JSONWriter::Write(input, &input_json);

    base::RunLoop run_loop;
    tool_->UseTool(
        input_json,
        base::BindLambdaForTesting(
            [&run_loop, output](std::vector<mojom::ContentBlockPtr> result) {
              ASSERT_FALSE(result.empty());
              ASSERT_TRUE(result[0]->is_text_content_block());
              *output = result[0]->get_text_content_block()->text;
              run_loop.Quit();
            }));
    run_loop.Run();
  }

 protected:
  std::unique_ptr<CodeExecutionTool> tool_;

 private:
  std::unique_ptr<net::test_server::HttpResponse> HandleTestRequest(
      const net::test_server::HttpRequest& request) {
    if (request.relative_url == "/test") {
      auto response = std::make_unique<net::test_server::BasicHttpResponse>();
      response->set_code(net::HTTP_OK);
      response->set_content("test response");
      return response;
    }
    return nullptr;
  }

  net::EmbeddedTestServer http_server_;
};

IN_PROC_BROWSER_TEST_F(AIChatCodeExecutionToolBrowserTest, HelloWorld) {
  std::string output;
  ExecuteCode("console.log('hello world')", &output);
  EXPECT_EQ(output, "hello world");
}

IN_PROC_BROWSER_TEST_F(AIChatCodeExecutionToolBrowserTest,
                       BlocksNetworkRequest) {
  std::string script = base::StrCat({
      R"(
        const xhr = new XMLHttpRequest();
        xhr.open('GET', ')",
      test_server_url(),
      R"(', false);
        try {
          xhr.send();
          console.log('Request succeeded: ' + xhr.responseText);
        } catch (e) {
          console.log('Error: ' + e.message);
        }
      )"});

  std::string output;
  ExecuteCode(script, &output);
  EXPECT_THAT(output, HasSubstr("action has been blocked"));
}

IN_PROC_BROWSER_TEST_F(AIChatCodeExecutionToolBrowserTest, ExecutionTimeout) {
  tool_->SetExecutionTimeLimitForTesting(base::Seconds(1));

  std::string script = R"(
    function fibonacci(n) {
      if (n <= 1) return n;
      return fibonacci(n - 1) + fibonacci(n - 2);
    }
    console.log('Starting computation...');
    const result = fibonacci(45);
    console.log('Result: ' + result);
  )";

  std::string output;
  ExecuteCode(script, &output);
  EXPECT_EQ(output, "Error: Time limit exceeded");
}

IN_PROC_BROWSER_TEST_F(AIChatCodeExecutionToolBrowserTest,
                       WindowAndLocationAreUndefined) {
  std::string script = R"(
    console.log('window: ' + typeof window);
    console.log('location: ' + typeof location);
  )";

  std::string output;
  ExecuteCode(script, &output);
  EXPECT_THAT(output, HasSubstr("window: undefined"));
  EXPECT_THAT(output, HasSubstr("location: undefined"));
}

}  // namespace ai_chat
