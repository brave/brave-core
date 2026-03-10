/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/brave_origin_startup/brave_origin_startup_handler.h"

#include <memory>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/test/test_future.h"
#include "brave/components/brave_origin/pref_names.h"
#include "brave/components/skus/browser/test/fake_skus_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

class BraveOriginStartupHandlerTest : public testing::Test {
 public:
  void SetUp() override {
    local_state_.registry()->RegisterBooleanPref(
        brave_origin::kOriginPurchaseValidated, false);
  }

  std::unique_ptr<BraveOriginStartupHandler> CreateHandler(
      BraveOriginStartupHandler::SkusServiceGetter skus_getter =
          BraveOriginStartupHandler::SkusServiceGetter(),
      base::RepeatingClosure open_buy_window_callback =
          base::RepeatingClosure(),
      base::OnceClosure close_dialog_callback = base::OnceClosure()) {
    return std::make_unique<BraveOriginStartupHandler>(
        std::move(skus_getter), &local_state_,
        std::move(open_buy_window_callback), std::move(close_dialog_callback));
  }

  std::unique_ptr<BraveOriginStartupHandler> CreateHandlerWithSkus() {
    return CreateHandler(base::BindRepeating(
        &BraveOriginStartupHandlerTest::GetSkusServiceRemote,
        base::Unretained(this)));
  }

  mojo::PendingRemote<skus::mojom::SkusService> GetSkusServiceRemote() {
    return fake_skus_service_.MakeRemote();
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_state_;
  skus::FakeSkusService fake_skus_service_;
};

TEST_F(BraveOriginStartupHandlerTest, OpenBuyWindowCallsCallback) {
  bool buy_window_opened = false;
  auto handler =
      CreateHandler(BraveOriginStartupHandler::SkusServiceGetter(),
                    base::BindRepeating([](bool* opened) { *opened = true; },
                                        &buy_window_opened));

  handler->OpenBuyWindow();
  EXPECT_TRUE(buy_window_opened);
}

TEST_F(BraveOriginStartupHandlerTest, CloseDialogBlockedWithoutValidatedPref) {
  bool dialog_closed = false;
  auto handler = CreateHandler(
      BraveOriginStartupHandler::SkusServiceGetter(), base::RepeatingClosure(),
      base::BindOnce([](bool* closed) { *closed = true; }, &dialog_closed));

  // CloseDialog should not fire the callback unless the pref is set.
  handler->CloseDialog();
  EXPECT_FALSE(dialog_closed);
}

TEST_F(BraveOriginStartupHandlerTest, CloseDialogAllowedWithValidatedPref) {
  bool dialog_closed = false;
  auto handler = CreateHandler(
      BraveOriginStartupHandler::SkusServiceGetter(), base::RepeatingClosure(),
      base::BindOnce([](bool* closed) { *closed = true; }, &dialog_closed));

  // Once the pref is set (by credential verification), CloseDialog works.
  local_state_.SetBoolean(brave_origin::kOriginPurchaseValidated, true);
  handler->CloseDialog();
  EXPECT_TRUE(dialog_closed);
}

TEST_F(BraveOriginStartupHandlerTest,
       CheckPurchaseStateReturnsFalseWhenSkusUnavailable) {
  // Without a SkusServiceGetter, the handler can't connect
  // to the SKU service, so CheckPurchaseState should return false.
  auto handler = CreateHandler();

  base::test::TestFuture<bool> future;
  handler->CheckPurchaseState(future.GetCallback());

  EXPECT_FALSE(future.Get());
}

TEST_F(BraveOriginStartupHandlerTest,
       VerifyPurchaseIdReturnsFalseWhenSkusUnavailable) {
  auto handler = CreateHandler();

  base::test::TestFuture<bool, const std::string&> future;
  handler->VerifyPurchaseId("test-purchase-id", future.GetCallback());

  auto [success, error] = future.Get();
  EXPECT_FALSE(success);
  EXPECT_FALSE(error.empty());
}

TEST_F(BraveOriginStartupHandlerTest, PrefDefaultIsFalse) {
  EXPECT_FALSE(local_state_.GetBoolean(brave_origin::kOriginPurchaseValidated));
}

TEST_F(BraveOriginStartupHandlerTest,
       CheckPurchaseStateReturnsTrueWithRemainingCredentials) {
  fake_skus_service_.SetCredentialSummaryResponse(
      R"({"remaining_credential_count": 1})");

  auto handler = CreateHandlerWithSkus();

  base::test::TestFuture<bool> future;
  handler->CheckPurchaseState(future.GetCallback());

  EXPECT_TRUE(future.Get());
  EXPECT_TRUE(local_state_.GetBoolean(brave_origin::kOriginPurchaseValidated));
}

TEST_F(BraveOriginStartupHandlerTest,
       CheckPurchaseStateReturnsTrueWithExpiresAt) {
  fake_skus_service_.SetCredentialSummaryResponse(
      R"({"remaining_credential_count": 0, "expires_at": "2027-01-01"})");

  auto handler = CreateHandlerWithSkus();

  base::test::TestFuture<bool> future;
  handler->CheckPurchaseState(future.GetCallback());

  EXPECT_TRUE(future.Get());
  EXPECT_TRUE(local_state_.GetBoolean(brave_origin::kOriginPurchaseValidated));
}

TEST_F(BraveOriginStartupHandlerTest,
       CheckPurchaseStateReturnsFalseWithNoCredentials) {
  fake_skus_service_.SetCredentialSummaryResponse(
      R"({"remaining_credential_count": 0})");

  auto handler = CreateHandlerWithSkus();

  base::test::TestFuture<bool> future;
  handler->CheckPurchaseState(future.GetCallback());

  EXPECT_FALSE(future.Get());
  EXPECT_FALSE(local_state_.GetBoolean(brave_origin::kOriginPurchaseValidated));
}

TEST_F(BraveOriginStartupHandlerTest,
       CheckPurchaseStateReturnsFalseWithEmptyResponse) {
  fake_skus_service_.SetCredentialSummaryResponse("");

  auto handler = CreateHandlerWithSkus();

  base::test::TestFuture<bool> future;
  handler->CheckPurchaseState(future.GetCallback());

  EXPECT_FALSE(future.Get());
}

TEST_F(BraveOriginStartupHandlerTest,
       VerifyPurchaseIdSucceedsWithValidCredentials) {
  fake_skus_service_.SetCredentialSummaryResponse(
      R"({"remaining_credential_count": 1})");

  auto handler = CreateHandlerWithSkus();

  base::test::TestFuture<bool, const std::string&> future;
  handler->VerifyPurchaseId("valid-order-id", future.GetCallback());

  auto [success, error] = future.Get();
  EXPECT_TRUE(success);
  EXPECT_TRUE(error.empty());
  EXPECT_TRUE(local_state_.GetBoolean(brave_origin::kOriginPurchaseValidated));
}

TEST_F(BraveOriginStartupHandlerTest, VerifyPurchaseIdFailsWithNoCredentials) {
  fake_skus_service_.SetCredentialSummaryResponse(
      R"({"remaining_credential_count": 0})");

  auto handler = CreateHandlerWithSkus();

  base::test::TestFuture<bool, const std::string&> future;
  handler->VerifyPurchaseId("some-order-id", future.GetCallback());

  auto [success, error] = future.Get();
  EXPECT_FALSE(success);
  EXPECT_FALSE(error.empty());
  EXPECT_FALSE(local_state_.GetBoolean(brave_origin::kOriginPurchaseValidated));
}

TEST_F(BraveOriginStartupHandlerTest,
       VerifyPurchaseIdFailsWithEmptyPurchaseId) {
  auto handler = CreateHandlerWithSkus();

  base::test::TestFuture<bool, const std::string&> future;
  handler->VerifyPurchaseId("", future.GetCallback());

  auto [success, error] = future.Get();
  EXPECT_FALSE(success);
  EXPECT_EQ(error, "Purchase ID is empty");
}

TEST_F(BraveOriginStartupHandlerTest,
       VerifyPurchaseIdFailsWithWhitespaceOnlyPurchaseId) {
  auto handler = CreateHandlerWithSkus();

  base::test::TestFuture<bool, const std::string&> future;
  handler->VerifyPurchaseId("   ", future.GetCallback());

  auto [success, error] = future.Get();
  EXPECT_FALSE(success);
  EXPECT_EQ(error, "Purchase ID is empty");
}
