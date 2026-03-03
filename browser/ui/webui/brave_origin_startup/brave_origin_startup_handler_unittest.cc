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
#include "chrome/test/base/testing_profile.h"
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
      base::RepeatingClosure open_buy_window_callback =
          base::RepeatingClosure(),
      base::OnceClosure close_dialog_callback = base::OnceClosure()) {
    return std::make_unique<BraveOriginStartupHandler>(
        &profile_, &local_state_, std::move(open_buy_window_callback),
        std::move(close_dialog_callback));
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  TestingPrefServiceSimple local_state_;
  skus::FakeSkusService fake_skus_service_;
};

TEST_F(BraveOriginStartupHandlerTest, GetBuyUrl) {
  auto handler = CreateHandler();

  base::test::TestFuture<const std::string&> future;
  handler->GetBuyUrl(future.GetCallback());

  const std::string& url = future.Get();
  EXPECT_FALSE(url.empty());
  EXPECT_TRUE(url.find("intent=checkout") != std::string::npos);
  EXPECT_TRUE(url.find("product=origin") != std::string::npos);
}

TEST_F(BraveOriginStartupHandlerTest, OpenBuyWindowCallsCallback) {
  bool buy_window_opened = false;
  auto handler = CreateHandler(base::BindRepeating(
      [](bool* opened) { *opened = true; }, &buy_window_opened));

  handler->OpenBuyWindow();
  EXPECT_TRUE(buy_window_opened);
}

TEST_F(BraveOriginStartupHandlerTest, CloseDialogSetsPrefAndCallsCallback) {
  EXPECT_FALSE(local_state_.GetBoolean(brave_origin::kOriginPurchaseValidated));

  bool dialog_closed = false;
  auto handler = CreateHandler(
      base::RepeatingClosure(),
      base::BindOnce([](bool* closed) { *closed = true; }, &dialog_closed));

  handler->CloseDialog();

  EXPECT_TRUE(local_state_.GetBoolean(brave_origin::kOriginPurchaseValidated));
  EXPECT_TRUE(dialog_closed);
}

TEST_F(BraveOriginStartupHandlerTest,
       CheckPurchaseStateReturnsFalseWhenSkusUnavailable) {
  // Without setting up the SkusServiceFactory, the handler can't connect
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
