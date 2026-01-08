/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_account/brave_account_row_handler.h"

#include <memory>
#include <string>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/no_destructor.h"
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
  MOCK_METHOD(void, UpdateState, (mojom::AccountStatePtr), (override));

  mojo::PendingRemote<mojom::RowClient> BindNewPipeAndPassRemote() {
    return receiver_.BindNewPipeAndPassRemote();
  }

  void FlushForTesting() { receiver_.FlushForTesting(); }

 private:
  mojo::Receiver<mojom::RowClient> receiver_{this};
};

template <typename T>
class BraveAccountRowHandlerTest : public testing::TestWithParam<const T*> {
 public:
  static constexpr auto kNameGenerator = [](const auto& info) {
    return CHECK_DEREF(info.param).test_name;
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
  mojom::AccountStatePtr expected_state;
};

const GetAccountStateTestCase* LoggedOut() {
  static const base::NoDestructor<GetAccountStateTestCase> kLoggedOut(
      {.test_name = "LoggedOut",
       .verification_token = "",
       .authentication_token = "",
       .expected_state =
           mojom::AccountState::NewLoggedOut(mojom::LoggedOutState::New())});
  return kLoggedOut.get();
}

const GetAccountStateTestCase* Verification() {
  static const base::NoDestructor<GetAccountStateTestCase> kVerification(
      {.test_name = "Verification",
       .verification_token = "verification_token",
       .authentication_token = "",
       .expected_state = mojom::AccountState::NewVerification(
           mojom::VerificationState::New())});
  return kVerification.get();
}

const GetAccountStateTestCase* LoggedIn() {
  static const base::NoDestructor<GetAccountStateTestCase> kLoggedIn(
      {.test_name = "LoggedIn",
       .verification_token = "",
       .authentication_token = "authentication_token",
       .expected_state = mojom::AccountState::NewLoggedIn(
           mojom::LoggedInState::New("email"))});
  return kLoggedIn.get();
}

const GetAccountStateTestCase* LoggedInTakesPrecedenceOverVerification() {
  static const base::NoDestructor<GetAccountStateTestCase>
      kLoggedInTakesPrecedenceOverVerification(
          {.test_name = "LoggedInTakesPrecedenceOverVerification",
           .verification_token = "verification_token",
           .authentication_token = "authentication_token",
           .expected_state = mojom::AccountState::NewLoggedIn(
               mojom::LoggedInState::New("email"))});
  return kLoggedInTakesPrecedenceOverVerification.get();
}

using BraveAccountRowHandlerGetAccountStateTest =
    BraveAccountRowHandlerTest<GetAccountStateTestCase>;

}  // namespace

TEST_P(BraveAccountRowHandlerGetAccountStateTest, GetAccountState) {
  const auto& test_case = CHECK_DEREF(GetParam());

  if (!test_case.verification_token.empty()) {
    prefs().SetString(prefs::kBraveAccountVerificationToken,
                      test_case.verification_token);
  }

  if (!test_case.authentication_token.empty()) {
    prefs().SetString(prefs::kBraveAccountEmailAddress, "email");
    prefs().SetString(prefs::kBraveAccountAuthenticationToken,
                      test_case.authentication_token);
  }

  CreateHandler();

  base::test::TestFuture<mojom::AccountStatePtr> future;
  row_handler()->GetAccountState(future.GetCallback());
  EXPECT_EQ(future.Get(), test_case.expected_state);
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountRowHandlerTest,
    BraveAccountRowHandlerGetAccountStateTest,
    testing::Values(LoggedOut(),
                    Verification(),
                    LoggedIn(),
                    LoggedInTakesPrecedenceOverVerification()),
    BraveAccountRowHandlerGetAccountStateTest::kNameGenerator);

namespace {

enum class StateAction {
  kSwitchToVerification,
  kSwitchToLoggedIn,
  kSwitchToLoggedOut,
  kUpdateEmailAddress,
};

MATCHER_P(AccountStateEq, expected, "") {
  // calls mojo::StructPtr<>::Equals()
  return arg == expected.get();
}

struct OnPrefChangedTestCase {
  std::string test_name;
  mojom::AccountStatePtr from;
  StateAction action;
  mojom::AccountStatePtr to;
};

const OnPrefChangedTestCase* LoggedOutToVerification() {
  static const base::NoDestructor<OnPrefChangedTestCase>
      kLoggedOutToVerification({.test_name = "LoggedOutToVerification",
                                .from = mojom::AccountState::NewLoggedOut(
                                    mojom::LoggedOutState::New()),
                                .action = StateAction::kSwitchToVerification,
                                .to = mojom::AccountState::NewVerification(
                                    mojom::VerificationState::New())});
  return kLoggedOutToVerification.get();
}

const OnPrefChangedTestCase* VerificationToLoggedIn() {
  static const base::NoDestructor<OnPrefChangedTestCase>
      kVerificationToLoggedIn({.test_name = "VerificationToLoggedIn",
                               .from = mojom::AccountState::NewVerification(
                                   mojom::VerificationState::New()),
                               .action = StateAction::kSwitchToLoggedIn,
                               .to = mojom::AccountState::NewLoggedIn(
                                   mojom::LoggedInState::New("email"))});
  return kVerificationToLoggedIn.get();
}

const OnPrefChangedTestCase* LoggedInToLoggedOut() {
  static const base::NoDestructor<OnPrefChangedTestCase> kLoggedInToLoggedOut(
      {.test_name = "LoggedInToLoggedOut",
       .from =
           mojom::AccountState::NewLoggedIn(mojom::LoggedInState::New("email")),
       .action = StateAction::kSwitchToLoggedOut,
       .to = mojom::AccountState::NewLoggedOut(mojom::LoggedOutState::New())});
  return kLoggedInToLoggedOut.get();
}

const OnPrefChangedTestCase* LoggedOutToLoggedIn() {
  static const base::NoDestructor<OnPrefChangedTestCase> kLoggedOutToLoggedIn(
      {.test_name = "LoggedOutToLoggedIn",
       .from = mojom::AccountState::NewLoggedOut(mojom::LoggedOutState::New()),
       .action = StateAction::kSwitchToLoggedIn,
       .to = mojom::AccountState::NewLoggedIn(
           mojom::LoggedInState::New("email"))});
  return kLoggedOutToLoggedIn.get();
}

const OnPrefChangedTestCase* LoggedInToLoggedInEmailChange() {
  static const base::NoDestructor<OnPrefChangedTestCase>
      kLoggedInToLoggedInEmailChange(
          {.test_name = "LoggedInToLoggedInEmailChange",
           .from = mojom::AccountState::NewLoggedIn(
               mojom::LoggedInState::New("email")),
           .action = StateAction::kUpdateEmailAddress,
           .to = mojom::AccountState::NewLoggedIn(
               mojom::LoggedInState::New("new_email"))});
  return kLoggedInToLoggedInEmailChange.get();
}

using BraveAccountRowHandlerOnPrefChangedTest =
    BraveAccountRowHandlerTest<OnPrefChangedTestCase>;

}  // namespace

TEST_P(BraveAccountRowHandlerOnPrefChangedTest, OnPrefChanged) {
  const auto& test_case = CHECK_DEREF(GetParam());

  switch (test_case.from->which()) {
    case mojom::AccountState::Tag::kLoggedOut:
      break;
    case mojom::AccountState::Tag::kVerification:
      prefs().SetString(prefs::kBraveAccountVerificationToken,
                        "verification_token");
      break;
    case mojom::AccountState::Tag::kLoggedIn:
      prefs().SetString(prefs::kBraveAccountEmailAddress, "email");
      prefs().SetString(prefs::kBraveAccountAuthenticationToken,
                        "authentication_token");
      break;
  }

  CreateHandler();

  // When switching to LoggedIn, the email is set first,
  // which triggers OnPrefChanged() but does not change the state
  // — the auth token update is what actually does.
  if (test_case.action == StateAction::kSwitchToLoggedIn) {
    EXPECT_CALL(row_client(),
                UpdateState(AccountStateEq(testing::ByRef(test_case.from))))
        .Times(1);
  }

  EXPECT_CALL(row_client(),
              UpdateState(AccountStateEq(testing::ByRef(test_case.to))))
      .Times(1);

  switch (test_case.action) {
    case StateAction::kSwitchToVerification:
      prefs().SetString(prefs::kBraveAccountVerificationToken,
                        "verification_token");
      break;
    case StateAction::kSwitchToLoggedIn:
      prefs().SetString(prefs::kBraveAccountEmailAddress, "email");
      prefs().SetString(prefs::kBraveAccountAuthenticationToken,
                        "authentication_token");
      break;
    case StateAction::kSwitchToLoggedOut:
      prefs().ClearPref(prefs::kBraveAccountAuthenticationToken);
      break;
    case StateAction::kUpdateEmailAddress:
      prefs().SetString(prefs::kBraveAccountEmailAddress, "new_email");
      break;
  }

  row_client().FlushForTesting();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountRowHandlerTest,
    BraveAccountRowHandlerOnPrefChangedTest,
    testing::Values(LoggedOutToVerification(),
                    VerificationToLoggedIn(),
                    LoggedInToLoggedOut(),
                    LoggedOutToLoggedIn(),
                    LoggedInToLoggedInEmailChange()),
    BraveAccountRowHandlerOnPrefChangedTest::kNameGenerator);

}  // namespace brave_account
