/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/zcash/zcash_grpc_utils.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/strings/strcat.h"
#include "base/test/bind.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {

class TestTxStreamHanlder : public GRrpcMessageStreamHandler {
 public:
  TestTxStreamHanlder() {}
  ~TestTxStreamHanlder() override {}

  std::vector<std::string> messages() { return messages_; }
  absl::optional<bool> complete_result() { return complete_result_; }
  void set_should_continue(bool r) { should_continue = r; }

 private:
  bool ProcessMessage(const std::string& message) override {
    messages_.push_back(message);
    return should_continue;
  }

  void OnComplete(bool success) override { complete_result_ = success; }

  std::vector<std::string> messages_;
  absl::optional<bool> complete_result_;
  bool should_continue = true;
};

}  // namespace

TEST(ZCashGrpcUtilsTest, TestTxStreamHanlder) {
  TestTxStreamHanlder handler;
  {
    absl::optional<bool> resume_called;
    auto resume_closure =
        base::BindLambdaForTesting([&]() { resume_called = true; });
    handler.OnDataReceived("", std::move(resume_closure));
    EXPECT_EQ(0u, handler.messages().size());
    EXPECT_TRUE(resume_called.value());
  }

  auto message = GetPrefixedProtobuf("message1");
  // Not enough data to complete message
  {
    absl::optional<bool> resume_called;
    auto resume_closure =
        base::BindLambdaForTesting([&]() { resume_called = true; });
    handler.OnDataReceived(message.substr(0, 3), std::move(resume_closure));
    EXPECT_EQ(0u, handler.messages().size());
    EXPECT_TRUE(resume_called.value());
  }

  // Complete message
  {
    absl::optional<bool> resume_called;
    auto resume_closure =
        base::BindLambdaForTesting([&]() { resume_called = true; });
    handler.OnDataReceived(message.substr(3, message.size()),
                           std::move(resume_closure));
    EXPECT_EQ(1u, handler.messages().size());
    EXPECT_TRUE(resume_called.value());
  }

  auto bundled_message = base::StrCat(
      {GetPrefixedProtobuf("message1"), GetPrefixedProtobuf("message2")});
  // Several messages received at once
  {
    absl::optional<bool> resume_called;
    auto resume_closure =
        base::BindLambdaForTesting([&]() { resume_called = true; });
    handler.OnDataReceived(bundled_message, std::move(resume_closure));
    EXPECT_EQ(3u, handler.messages().size());
    EXPECT_TRUE(resume_called.value());
  }
}

}  // namespace brave_wallet
