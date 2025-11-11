/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_account/brave_account_row_handler.h"

#include <memory>
#include <string>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_future.h"
#include "brave/components/brave_account/features.h"
#include "brave/components/brave_account/mojom/brave_account_row.mojom.h"
#include "brave/components/brave_account/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_web_ui.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

namespace {

class MockRowClient : public mojom::RowClient {
 public:
  MOCK_METHOD(void, UpdateState, (mojom::AccountState), (override));

  mojo::PendingRemote<mojom::RowClient> BindNewPipeAndPassRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  void FlushForTesting() { receiver_.FlushForTesting(); }

 private:
  mojo::Receiver<mojom::RowClient> receiver_{this};
};

template <typename T>
class BraveAccountRowHandlerTest : public testing::TestWithParam<T> {
 public:
  static constexpr auto kNameGenerator = [](const auto& info) {
    return info.param.test_name;
  };

  BraveAccountRowHandlerTest()
      : profile_(TestingProfile::Builder().Build()),
        web_contents_(content::WebContents::Create(
            content::WebContents::CreateParams(profile_.get()))) {
    test_web_ui_.set_web_contents(web_contents_.get());
  }

  void CreateHandler() {
    row_client_ = std::make_unique<MockRowClient>();
    handler_ = std::make_unique<BraveAccountRowHandler>(
        row_handler_.BindNewPipeAndPassReceiver(),
        row_client_->BindNewPipeAndPassRemote(), &test_web_ui_);
  }

  PrefService& prefs() { return CHECK_DEREF(profile_->GetPrefs()); }

  MockRowClient& row_client() { return CHECK_DEREF(row_client_.get()); }

  mojo::Remote<mojom::RowHandler>& row_handler() {
    CHECK(row_handler_);
    return row_handler_;
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_{features::kBraveAccount};
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<content::WebContents> web_contents_;
  content::TestWebUI test_web_ui_;
  std::unique_ptr<MockRowClient> row_client_;
  mojo::Remote<mojom::RowHandler> row_handler_;
  std::unique_ptr<BraveAccountRowHandler> handler_;
};

struct GetAccountStateTestCase {
  std::string test_name;
  std::string verification_token;
  std::string authentication_token;
  mojom::AccountState expected_state;
};

}  // namespace

using BraveAccountRowHandlerGetAccountStateTest =
    BraveAccountRowHandlerTest<GetAccountStateTestCase>;

TEST_P(BraveAccountRowHandlerGetAccountStateTest, GetAccountState) {
  const auto& test_case = GetParam();

  if (!test_case.verification_token.empty()) {
    prefs().SetString(prefs::kVerificationToken, test_case.verification_token);
  }

  if (!test_case.authentication_token.empty()) {
    prefs().SetString(prefs::kAuthenticationToken,
                      test_case.authentication_token);
  }

  CreateHandler();

  base::test::TestFuture<mojom::AccountState> future;
  row_handler()->GetAccountState(future.GetCallback());
  EXPECT_EQ(future.Get(), test_case.expected_state);
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountRowHandlerTest,
    BraveAccountRowHandlerGetAccountStateTest,
    testing::Values(
        GetAccountStateTestCase{"LoggedOut", "", "",
                                mojom::AccountState::kLoggedOut},
        GetAccountStateTestCase{"Verification", "verification_token", "",
                                mojom::AccountState::kVerification},
        GetAccountStateTestCase{"LoggedIn", "", "authentication_token",
                                mojom::AccountState::kLoggedIn},
        GetAccountStateTestCase{"LoggedInTakesPrecedenceOverVerification",
                                "verification_token", "authentication_token",
                                mojom::AccountState::kLoggedIn}),
    BraveAccountRowHandlerGetAccountStateTest::kNameGenerator);

namespace {

enum class StateAction {
  kSetVerificationToken,
  kSetAuthenticationToken,
  kClearAuthenticationToken,
};

struct OnPrefChangedTestCase {
  std::string test_name;
  mojom::AccountState from;
  StateAction action;
  mojom::AccountState to;
};

}  // namespace

using BraveAccountRowHandlerOnPrefChangedTest =
    BraveAccountRowHandlerTest<OnPrefChangedTestCase>;

TEST_P(BraveAccountRowHandlerOnPrefChangedTest, OnPrefChanged) {
  const auto& test_case = GetParam();

  switch (test_case.from) {
    case mojom::AccountState::kLoggedOut:
      break;
    case mojom::AccountState::kVerification:
      prefs().SetString(prefs::kVerificationToken, "verification_token");
      break;
    case mojom::AccountState::kLoggedIn:
      prefs().SetString(prefs::kAuthenticationToken, "authentication_token");
      break;
  }

  CreateHandler();

  EXPECT_CALL(row_client(), UpdateState(test_case.to)).Times(1);

  switch (test_case.action) {
    case StateAction::kSetVerificationToken:
      prefs().SetString(prefs::kVerificationToken, "verification_token");
      break;
    case StateAction::kSetAuthenticationToken:
      prefs().SetString(prefs::kAuthenticationToken, "authentication_token");
      break;
    case StateAction::kClearAuthenticationToken:
      prefs().ClearPref(prefs::kAuthenticationToken);
      break;
  }

  row_client().FlushForTesting();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountRowHandlerTest,
    BraveAccountRowHandlerOnPrefChangedTest,
    testing::Values(OnPrefChangedTestCase{"LoggedOutToVerification",
                                          mojom::AccountState::kLoggedOut,
                                          StateAction::kSetVerificationToken,
                                          mojom::AccountState::kVerification},
                    OnPrefChangedTestCase{"VerificationToLoggedIn",
                                          mojom::AccountState::kVerification,
                                          StateAction::kSetAuthenticationToken,
                                          mojom::AccountState::kLoggedIn},
                    OnPrefChangedTestCase{
                        "LoggedInToLoggedOut", mojom::AccountState::kLoggedIn,
                        StateAction::kClearAuthenticationToken,
                        mojom::AccountState::kLoggedOut},
                    OnPrefChangedTestCase{"LoggedOutToLoggedIn",
                                          mojom::AccountState::kLoggedOut,
                                          StateAction::kSetAuthenticationToken,
                                          mojom::AccountState::kLoggedIn}),
    BraveAccountRowHandlerOnPrefChangedTest::kNameGenerator);

}  // namespace brave_account
