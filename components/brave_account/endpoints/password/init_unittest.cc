/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/password/init.h"

#include <optional>
#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/data_element.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave_account::endpoints {

class PasswordInitTest : public testing::Test {
 protected:
  std::string email_ = "email";
  std::string blinded_message_ = "blinded_message";
  network::TestURLLoaderFactory test_url_loader_factory_;
  base::test::TaskEnvironment task_environment_;
};

TEST_F(PasswordInitTest, Send) {
  PasswordInit password_init(test_url_loader_factory_.GetSafeWeakWrapper());

  test_url_loader_factory_.SetInterceptor(
      base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
        EXPECT_EQ(request.method, "POST");

        EXPECT_EQ(request.url.scheme(), "https");
        EXPECT_TRUE(base::StartsWith(request.url.host(), "accounts.bsg"));
        EXPECT_EQ(request.url.path(), "/v2/accounts/password/init");

        ASSERT_TRUE(request.request_body);
        const auto* elements = request.request_body->elements();
        ASSERT_TRUE(elements);
        ASSERT_FALSE(elements->empty());
        const auto& element = elements->front();
        ASSERT_EQ(element.type(), network::DataElement::Tag::kBytes);
        const auto body = base::JSONReader::Read(
            element.As<network::DataElementBytes>().AsStringPiece());
        ASSERT_TRUE(body && body->is_dict());
        const auto& dict = body->GetDict();
        const auto* blinded_message = dict.FindString("blindedMessage");
        ASSERT_NE(blinded_message, nullptr);
        EXPECT_EQ(*blinded_message, blinded_message_);
        const auto* new_account_email = dict.FindString("newAccountEmail");
        ASSERT_NE(new_account_email, nullptr);
        EXPECT_EQ(*new_account_email, email_);
        const auto serialize_response = dict.FindBool("serializeResponse");
        ASSERT_TRUE(serialize_response);
        EXPECT_TRUE(*serialize_response);

        EXPECT_EQ(request.headers.GetHeader("Content-Type"),
                  "application/json");

        test_url_loader_factory_.AddResponse(request.url.spec(), "");
      }));

  base::RunLoop run_loop;
  base::MockCallback<api_request_helper::APIRequestHelper::ResultCallback>
      callback;
  EXPECT_CALL(callback, Run(testing::_)).Times(1).WillOnce([&] {
    run_loop.Quit();
  });

  password_init.Send(email_, blinded_message_, callback.Get());

  run_loop.Run();
}

}  // namespace brave_account::endpoints
