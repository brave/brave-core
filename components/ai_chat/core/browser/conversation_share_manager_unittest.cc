// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_chat/core/browser/conversation_share_manager.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/api_request_helper/mock_api_request_helper.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using ResultCallback = api_request_helper::APIRequestHelper::ResultCallback;
using Ticket = api_request_helper::APIRequestHelper::Ticket;
using api_request_helper::MockAPIRequestHelper;

namespace ai_chat {

namespace {

constexpr char kEncryptedContents[] = "base64-encoded-iv-and-ciphertext";

// Exposes a MockAPIRequestHelper so tests can intercept the outgoing request
// and supply a fabricated response.
class TestConversationShareManager : public ConversationShareManager {
 public:
  TestConversationShareManager() : ConversationShareManager(nullptr) {
    auto mock_helper =
        std::make_unique<testing::NiceMock<MockAPIRequestHelper>>(
            TRAFFIC_ANNOTATION_FOR_TESTS, nullptr);
    SetAPIRequestHelperForTesting(std::move(mock_helper));
  }

  MockAPIRequestHelper* GetMockAPIRequestHelper() {
    return static_cast<MockAPIRequestHelper*>(GetAPIRequestHelperForTesting());
  }
};

}  // namespace

class ConversationShareManagerUnitTest : public testing::Test {
 public:
  ConversationShareManagerUnitTest() = default;
  ~ConversationShareManagerUnitTest() override = default;

  void SetUp() override {
    share_manager_ = std::make_unique<TestConversationShareManager>();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<TestConversationShareManager> share_manager_;
};

TEST_F(ConversationShareManagerUnitTest, ShareConversation_Success) {
  MockAPIRequestHelper* mock_request_helper =
      share_manager_->GetMockAPIRequestHelper();

  EXPECT_CALL(*mock_request_helper, Request)
      .WillOnce(
          [&](const std::string& method, const GURL& url,
              const std::string& body, const std::string& content_type,
              ResultCallback result_callback,
              const base::flat_map<std::string, std::string>& headers,
              const api_request_helper::APIRequestOptions& options,
              api_request_helper::APIRequestHelper::ResponseConversionCallback
                  conversion_callback) {
            // Verify the request targets the sharing endpoint on the relay
            // host.
            EXPECT_EQ(net::HttpRequestHeaders::kPostMethod, method);
            EXPECT_TRUE(url.is_valid());
            EXPECT_TRUE(url.SchemeIs(url::kHttpsScheme));
            EXPECT_EQ("/v1/share", url.path());
            EXPECT_EQ("application/json", content_type);

            // Verify the same relay auth headers as the conversation API.
            EXPECT_TRUE(headers.contains("x-brave-key"));
            EXPECT_TRUE(headers.contains("digest"));
            EXPECT_TRUE(
                headers.contains(net::HttpRequestHeaders::kAuthorization));

            // Verify the ciphertext is sent under the expected key.
            auto body_dict = base::test::ParseJsonDict(body);
            const std::string* ciphertext = body_dict.FindString("ciphertext");
            EXPECT_TRUE(ciphertext);
            if (ciphertext) {
              EXPECT_EQ(kEncryptedContents, *ciphertext);
            }

            std::move(result_callback)
                .Run(api_request_helper::APIRequestResult(
                    net::HTTP_OK, base::Value(base::test::ParseJsonDict(R"({
                      "share_id": "abc123"
                    })")),
                    {}, net::OK, GURL()));
            return Ticket();
          });

  base::test::TestFuture<const std::optional<GURL>&> future;
  share_manager_->ShareConversation(kEncryptedContents, future.GetCallback());

  const std::optional<GURL>& result = future.Get();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ("https://leo-ai.brave.app/shared/abc123", result->spec());
}

TEST_F(ConversationShareManagerUnitTest, ShareConversation_ServerError) {
  MockAPIRequestHelper* mock_request_helper =
      share_manager_->GetMockAPIRequestHelper();

  // The mocked Request both returns a Ticket and hands us the ResultCallback to
  // invoke, so a lambda is used rather than RunOnceCallback<> (which cannot
  // move a OnceCallback out of DoAll's const-ref args while still returning the
  // Ticket).
  EXPECT_CALL(*mock_request_helper, Request)
      .WillOnce(
          [](const std::string&, const GURL&, const std::string&,
             const std::string&, ResultCallback result_callback,
             const base::flat_map<std::string, std::string>&,
             const api_request_helper::APIRequestOptions&,
             api_request_helper::APIRequestHelper::ResponseConversionCallback) {
            std::move(result_callback)
                .Run(api_request_helper::APIRequestResult(
                    net::HTTP_INTERNAL_SERVER_ERROR, base::Value(), {}, net::OK,
                    GURL()));
            return Ticket();
          });

  base::test::TestFuture<const std::optional<GURL>&> future;
  share_manager_->ShareConversation(kEncryptedContents, future.GetCallback());

  EXPECT_FALSE(future.Get().has_value());
}

TEST_F(ConversationShareManagerUnitTest, ShareConversation_MissingShareId) {
  MockAPIRequestHelper* mock_request_helper =
      share_manager_->GetMockAPIRequestHelper();

  EXPECT_CALL(*mock_request_helper, Request)
      .WillOnce(
          [](const std::string&, const GURL&, const std::string&,
             const std::string&, ResultCallback result_callback,
             const base::flat_map<std::string, std::string>&,
             const api_request_helper::APIRequestOptions&,
             api_request_helper::APIRequestHelper::ResponseConversionCallback) {
            std::move(result_callback)
                .Run(api_request_helper::APIRequestResult(
                    net::HTTP_OK, base::Value(base::test::ParseJsonDict(R"({
                  "unexpected": "field"
                })")),
                    {}, net::OK, GURL()));
            return Ticket();
          });

  base::test::TestFuture<const std::optional<GURL>&> future;
  share_manager_->ShareConversation(kEncryptedContents, future.GetCallback());

  EXPECT_FALSE(future.Get().has_value());
}

}  // namespace ai_chat
