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
                      "share_id": "abc123",
                      "deletion_id": "del456"
                    })")),
                    {}, net::OK, GURL()));
            return Ticket();
          });

  base::test::TestFuture<const std::optional<ConversationShareResult>&> future;
  share_manager_->ShareConversation(kEncryptedContents, future.GetCallback());

  const std::optional<ConversationShareResult>& result = future.Get();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ("https://leo-ai.brave.app/shared/abc123",
            result->viewer_url.spec());
  EXPECT_EQ("abc123", result->share_id);
  // The deletion id is the capability token needed to delete the share later.
  EXPECT_EQ("del456", result->deletion_id);
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

  base::test::TestFuture<const std::optional<ConversationShareResult>&> future;
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

  base::test::TestFuture<const std::optional<ConversationShareResult>&> future;
  share_manager_->ShareConversation(kEncryptedContents, future.GetCallback());

  EXPECT_FALSE(future.Get().has_value());
}

TEST_F(ConversationShareManagerUnitTest, ShareConversation_MissingDeletionId) {
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
                  "share_id": "abc123"
                })")),
                    {}, net::OK, GURL()));
            return Ticket();
          });

  base::test::TestFuture<const std::optional<ConversationShareResult>&> future;
  share_manager_->ShareConversation(kEncryptedContents, future.GetCallback());

  // The link works even though the share can't be deleted from this client, so
  // the share is still reported as successful.
  const std::optional<ConversationShareResult>& result = future.Get();
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ("abc123", result->share_id);
  EXPECT_TRUE(result->deletion_id.empty());
}

TEST_F(ConversationShareManagerUnitTest, DeleteShare_Success) {
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
            EXPECT_EQ(net::HttpRequestHeaders::kPostMethod, method);
            EXPECT_EQ("/v1/share/delete", url.path());
            EXPECT_EQ("application/json", content_type);

            EXPECT_TRUE(headers.contains("x-brave-key"));
            EXPECT_TRUE(headers.contains("digest"));
            EXPECT_TRUE(
                headers.contains(net::HttpRequestHeaders::kAuthorization));

            auto body_dict = base::test::ParseJsonDict(body);
            const std::string* deletion_id =
                body_dict.FindString("deletion_id");
            EXPECT_TRUE(deletion_id);
            if (deletion_id) {
              EXPECT_EQ("del456", *deletion_id);
            }

            std::move(result_callback)
                .Run(api_request_helper::APIRequestResult(
                    net::HTTP_OK, base::Value(), {}, net::OK, GURL()));
            return Ticket();
          });

  base::test::TestFuture<bool> future;
  share_manager_->DeleteShare("del456", future.GetCallback());

  EXPECT_TRUE(future.Get());
}

TEST_F(ConversationShareManagerUnitTest, DeleteShare_AlreadyGone) {
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
                    net::HTTP_NOT_FOUND, base::Value(), {}, net::OK, GURL()));
            return Ticket();
          });

  base::test::TestFuture<bool> future;
  share_manager_->DeleteShare("del456", future.GetCallback());

  // A share the server has already forgotten is in the state the user asked
  // for, so the local record can be dropped too.
  EXPECT_TRUE(future.Get());
}

TEST_F(ConversationShareManagerUnitTest, DeleteShare_ServerError) {
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
                    net::HTTP_INTERNAL_SERVER_ERROR, base::Value(), {}, net::OK,
                    GURL()));
            return Ticket();
          });

  base::test::TestFuture<bool> future;
  share_manager_->DeleteShare("del456", future.GetCallback());

  EXPECT_FALSE(future.Get());
}

TEST_F(ConversationShareManagerUnitTest, DeleteShare_NoDeletionId) {
  MockAPIRequestHelper* mock_request_helper =
      share_manager_->GetMockAPIRequestHelper();

  // Nothing authorizes the deletion, so no request is made.
  EXPECT_CALL(*mock_request_helper, Request).Times(0);

  base::test::TestFuture<bool> future;
  share_manager_->DeleteShare("", future.GetCallback());

  EXPECT_FALSE(future.Get());
}

}  // namespace ai_chat
