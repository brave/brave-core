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
      ADD_FAILURE() << "Request should have been blocked";
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
  ExecuteCode("return 'hello world'", &output);
  EXPECT_EQ(output, "hello world");
}

IN_PROC_BROWSER_TEST_F(AIChatCodeExecutionToolBrowserTest, SimpleFibonacci) {
  std::string script = R"(
    function fibonacci(n) {
      if (n <= 1) return n;
      return fibonacci(n - 1) + fibonacci(n - 2);
    }
    return 'Fibonacci(10) = ' + fibonacci(10);
  )";

  std::string output;
  ExecuteCode(script, &output);
  EXPECT_EQ(output, "Fibonacci(10) = 55");
}

IN_PROC_BROWSER_TEST_F(AIChatCodeExecutionToolBrowserTest,
                       AccessLocalStateBlocked) {
  std::string script = "return localStorage.getItem('sensitive_data')";

  std::string output;
  ExecuteCode(script, &output);
  EXPECT_THAT(
      output,
      HasSubstr("SecurityError: Failed to read the 'localStorage' property"));
}

IN_PROC_BROWSER_TEST_F(AIChatCodeExecutionToolBrowserTest,
                       BlocksNetworkRequest) {
  std::string script = base::StrCat({
      R"(
        const response = await fetch(')",
      test_server_url(),
      R"(');
        const text = await response.text();
        return 'Request succeeded: ' + text;
      )"});

  std::string output;
  ExecuteCode(script, &output);
  EXPECT_THAT(output, HasSubstr("Failed to fetch"));
}

IN_PROC_BROWSER_TEST_F(AIChatCodeExecutionToolBrowserTest, ExecutionTimeout) {
  tool_->SetExecutionTimeLimitForTesting(base::Seconds(1));

  std::string script = R"(
    function fibonacci(n) {
      if (n <= 1) return n;
      return fibonacci(n - 1) + fibonacci(n - 2);
    }
    let result = 'Starting computation...';
    result += ' Result: ' + fibonacci(45);
    return result;
  )";

  std::string output;
  ExecuteCode(script, &output);
  EXPECT_EQ(output, "Error: Time limit exceeded");
}

IN_PROC_BROWSER_TEST_F(AIChatCodeExecutionToolBrowserTest, InvalidReturnType) {
  std::string script = "return 42";

  std::string output;
  ExecuteCode(script, &output);
  EXPECT_EQ(output, "Error: Invalid return type or syntax error");
}

IN_PROC_BROWSER_TEST_F(AIChatCodeExecutionToolBrowserTest, SyntaxError) {
  std::string script = R"(
    let x = 'unclosed string;
    return x;
  )";

  std::string output;
  ExecuteCode(script, &output);
  EXPECT_EQ(output, "Error: Invalid return type or syntax error");
}

}  // namespace ai_chat
