/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/purchased_state_manager.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/location.h"
#include "base/run_loop.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/brave_vpn/browser/v2/credential_store.h"
#include "brave/components/brave_vpn/browser/v2/skus_service_client.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {

using PurchasedState = mojom::PurchasedState;

// Cookie expiry strings, evaluated against the fixed mock clock.
constexpr char kFutureCookieExpiry[] = "Sat, 01 Jan 2050 00:00:00 GMT";
constexpr char kPastCookieExpiry[] = "Fri, 01 Jan 2010 00:00:00 GMT";

std::string MakeCredentialCookie(const std::string& value,
                                 const std::string& expires) {
  return "credential=" + value + "; Expires=" + expires;
}

class FakeSkusServiceClient : public SkusServiceClient {
 public:
  FakeSkusServiceClient()
      : SkusServiceClient(base::BindRepeating(
            [] { return mojo::PendingRemote<skus::mojom::SkusService>(); })) {}
  ~FakeSkusServiceClient() override = default;

  void GetCredentialSummary(
      const std::string& domain,
      skus::mojom::SkusService::CredentialSummaryCallback /*callback*/)
      override {
    ++credential_summary_calls_;
    last_domain_ = domain;
  }

  void PrepareCredentialsPresentation(
      const std::string& domain,
      const std::string& /*path*/,
      skus::mojom::SkusService::PrepareCredentialsPresentationCallback
      /*callback*/) override {
    ++prepare_presentation_calls_;
    last_domain_ = domain;
  }

  int credential_summary_calls() const { return credential_summary_calls_; }
  int prepare_presentation_calls() const { return prepare_presentation_calls_; }
  const std::string& last_domain() const { return last_domain_; }

 private:
  int credential_summary_calls_ = 0;
  int prepare_presentation_calls_ = 0;
  std::string last_domain_;
};

}  // namespace

class PurchasedStateManagerTest : public testing::Test {
 public:
  PurchasedStateManagerTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void SetUp() override {
    // Pin the clock so cookie/credential expirations are deterministic.
    base::Time fixed_time;
    ASSERT_TRUE(base::Time::FromString("2023-01-04", &fixed_time));
    task_environment_.AdvanceClock(fixed_time - base::Time::Now());

    brave_vpn::RegisterLocalStatePrefs(local_pref_service_.registry());
  }

  void CreateManager() {
    manager_ = std::make_unique<PurchasedStateManager>(
        &local_pref_service_, &fake_skus_client_,
        base::BindRepeating(&PurchasedStateManagerTest::OnPurchasedStateChanged,
                            base::Unretained(this)));
    base::RunLoop run_loop;
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, run_loop.QuitClosure());
    run_loop.Run();
  }

  void CreateAndLoadManager() {
    CreateManager();
    ASSERT_TRUE(manager_);
    manager_->Load(domain());
  }

  void SeedEnvironment() {
    local_pref_service_.SetString(prefs::kBraveVPNEnvironment, env_);
  }

  void SeedValidSubscriberCredential() {
    CredentialStore(&local_pref_service_)
        .SetSubscriberCredential("subscriber-credential", Future());
  }

  void SeedExpiredSubscriberCredential() {
    CredentialStore(&local_pref_service_)
        .SetSubscriberCredential("subscriber-credential", Past());
  }

  void SeedValidSkusCredential() {
    CredentialStore(&local_pref_service_)
        .SetSkusCredential("skus-credential", Future());
  }

  void OnCredentialSummary(const std::string& domain,
                           const std::string& message) {
    manager_->OnCredentialSummary(domain, MakeResult(message));
  }

  void OnPrepareCredentialsPresentation(const std::string& domain,
                                        const std::string& cookie) {
    manager_->OnPrepareCredentialsPresentation(domain, MakeResult(cookie));
  }

  void OnGetSubscriberCredentialV12(const std::string& credential,
                                    bool success) {
    manager_->OnGetSubscriberCredentialV12(Future(), credential, success);
  }

  CredentialStore* credential_store() {
    return manager_->credential_store_.get();
  }

  PurchasedState GetState() const { return manager_->GetInfo().state; }

  void OnPurchasedStateChanged(PurchasedState state,
                               const std::optional<std::string>& description) {
    ++state_change_count_;
    last_state_ = state;
    last_description_ = description;
  }

  std::string DomainFor(const std::string& env) const {
    return skus::GetDomain(skus::GetVpnProductPrefix(), env);
  }

  std::string domain() const { return DomainFor(env_); }

  // An environment guaranteed to differ from env_.
  std::string OtherEnvironment() const {
    return env_ == skus::kEnvProduction ? skus::kEnvStaging
                                        : skus::kEnvProduction;
  }

  skus::mojom::SkusResultPtr MakeResult(const std::string& message) {
    return skus::mojom::SkusResult::New(skus::mojom::SkusResultCode::Ok,
                                        message);
  }

  base::Time Future() const { return base::Time::Now() + base::Days(30); }
  base::Time Past() const { return base::Time::Now() - base::Days(1); }

 protected:
  const std::string env_ = skus::GetDefaultEnvironment();

  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple local_pref_service_;
  FakeSkusServiceClient fake_skus_client_;
  std::unique_ptr<PurchasedStateManager> manager_;

  int state_change_count_ = 0;
  std::optional<PurchasedState> last_state_;
  std::optional<std::string> last_description_;
};

TEST_F(PurchasedStateManagerTest, FreshUserStaysNotPurchased) {
  CreateManager();

  EXPECT_EQ(GetState(), PurchasedState::NOT_PURCHASED);
  EXPECT_EQ(state_change_count_, 0);
  EXPECT_EQ(fake_skus_client_.credential_summary_calls(), 0);
}

TEST_F(PurchasedStateManagerTest,
       ValidSubscriberCredentialIsPurchasedAtStartup) {
  SeedEnvironment();
  SeedValidSubscriberCredential();
  CreateManager();

  EXPECT_EQ(GetState(), PurchasedState::PURCHASED);
  ASSERT_TRUE(last_state_.has_value());
  EXPECT_EQ(*last_state_, PurchasedState::PURCHASED);
}

TEST_F(PurchasedStateManagerTest,
       ValidSubscriberCredentialAtStartupArmsRefreshTimer) {
  SeedEnvironment();
  SeedValidSubscriberCredential();
  CreateManager();

  ASSERT_EQ(GetState(), PurchasedState::PURCHASED);
  task_environment_.FastForwardBy(base::Days(31));
  EXPECT_FALSE(credential_store()->HasValidSubscriberCredential());
  EXPECT_EQ(fake_skus_client_.credential_summary_calls(), 1);
}

TEST_F(PurchasedStateManagerTest, CachedSkusCredentialAtStartupParksInLoading) {
  // A cached SKUS credential at startup reaches Load's SKUS branch, which is
  // NOTIMPLEMENTED. It should park at LOADING without requesting a summary or a
  // presentation, and leave the SKUS credential cached. When the exchange is
  // wired, this test is expected to change.
  SeedEnvironment();
  SeedValidSkusCredential();
  CreateManager();

  EXPECT_EQ(GetState(), PurchasedState::LOADING);
  EXPECT_EQ(fake_skus_client_.credential_summary_calls(), 0);
  EXPECT_EQ(fake_skus_client_.prepare_presentation_calls(), 0);
  EXPECT_TRUE(credential_store()->HasValidSkusCredential());
}

TEST_F(PurchasedStateManagerTest,
       StaleSubscriberCredentialIsClearedAndReloaded) {
  SeedEnvironment();
  SeedExpiredSubscriberCredential();
  CreateManager();

  EXPECT_FALSE(credential_store()->HasAnyCredential());
  EXPECT_GE(fake_skus_client_.credential_summary_calls(), 1);
  EXPECT_EQ(GetState(), PurchasedState::LOADING);
}

TEST_F(PurchasedStateManagerTest, NotifiesOnlyWhenStateChanges) {
  SeedEnvironment();
  CreateManager();

  manager_->SetPurchasedState(env_, PurchasedState::PURCHASED);
  EXPECT_EQ(state_change_count_, 1);
  EXPECT_EQ(GetState(), PurchasedState::PURCHASED);

  manager_->SetPurchasedState(env_, PurchasedState::PURCHASED);
  EXPECT_EQ(state_change_count_, 1);
}

TEST_F(PurchasedStateManagerTest, GettersTrackPurchasedState) {
  SeedEnvironment();
  CreateManager();

  EXPECT_EQ(GetState(), PurchasedState::NOT_PURCHASED);
  EXPECT_FALSE(manager_->IsPurchased());

  manager_->SetPurchasedState(env_, PurchasedState::PURCHASED);
  EXPECT_EQ(GetState(), PurchasedState::PURCHASED);
  EXPECT_TRUE(manager_->IsPurchased());

  manager_->SetPurchasedState(env_, PurchasedState::FAILED);
  EXPECT_EQ(GetState(), PurchasedState::FAILED);
  EXPECT_FALSE(manager_->IsPurchased());
}

TEST_F(PurchasedStateManagerTest, LoadWithoutCredentialsRequestsSummary) {
  CreateAndLoadManager();

  EXPECT_EQ(GetState(), PurchasedState::LOADING);
  EXPECT_EQ(fake_skus_client_.credential_summary_calls(), 1);
  EXPECT_EQ(fake_skus_client_.last_domain(), domain());
}

TEST_F(PurchasedStateManagerTest, EmptySummaryIsNotPurchased) {
  CreateAndLoadManager();
  OnCredentialSummary(domain(), "");
  EXPECT_EQ(GetState(), PurchasedState::NOT_PURCHASED);
}

TEST_F(PurchasedStateManagerTest, MalformedSummaryIsFailed) {
  CreateAndLoadManager();
  OnCredentialSummary(domain(), "not-json");
  EXPECT_EQ(GetState(), PurchasedState::FAILED);
}

TEST_F(PurchasedStateManagerTest, NeedsActivationSummaryIsNotPurchased) {
  CreateAndLoadManager();
  OnCredentialSummary(domain(),
                      R"({"active": false, "remaining_credential_count": 3})");
  EXPECT_EQ(GetState(), PurchasedState::NOT_PURCHASED);
}

TEST_F(PurchasedStateManagerTest, ValidSummaryPresentsAndCachesSkusCredential) {
  CreateAndLoadManager();

  OnCredentialSummary(domain(),
                      R"({"active": true, "remaining_credential_count": 3})");
  // Active summary triggers a presentation request.
  EXPECT_EQ(fake_skus_client_.prepare_presentation_calls(), 1);

  OnPrepareCredentialsPresentation(
      domain(), MakeCredentialCookie("skus-credential", kFutureCookieExpiry));

  // The SKUS credential is cached. The exchange is not wired yet, so the state
  // remains LOADING.
  EXPECT_TRUE(credential_store()->HasValidSkusCredential());
  EXPECT_EQ(credential_store()->GetSkusCredential(), "skus-credential");
  EXPECT_EQ(GetState(), PurchasedState::LOADING);
}

TEST_F(PurchasedStateManagerTest, InvalidPresentationCookieIsFailed) {
  CreateAndLoadManager();
  OnCredentialSummary(domain(),
                      R"({"active": true, "remaining_credential_count": 3})");

  OnPrepareCredentialsPresentation(domain(), "this-is-not-a-cookie");
  EXPECT_EQ(GetState(), PurchasedState::FAILED);
  EXPECT_FALSE(credential_store()->HasValidSkusCredential());
}

TEST_F(PurchasedStateManagerTest, ExpiredPresentationCookieIsFailed) {
  CreateAndLoadManager();
  OnCredentialSummary(domain(),
                      R"({"active": true, "remaining_credential_count": 3})");

  OnPrepareCredentialsPresentation(
      domain(), MakeCredentialCookie("skus-credential", kPastCookieExpiry));
  EXPECT_EQ(GetState(), PurchasedState::FAILED);
  EXPECT_FALSE(credential_store()->HasValidSkusCredential());
}

TEST_F(PurchasedStateManagerTest, EmptyPresentationCredentialIsNotPurchased) {
  CreateAndLoadManager();
  OnCredentialSummary(domain(),
                      R"({"active": true, "remaining_credential_count": 3})");

  OnPrepareCredentialsPresentation(
      domain(), MakeCredentialCookie("", kFutureCookieExpiry));
  EXPECT_EQ(GetState(), PurchasedState::NOT_PURCHASED);
  EXPECT_FALSE(credential_store()->HasValidSkusCredential());
}

TEST_F(PurchasedStateManagerTest, SuccessfulExchangeIsPurchasedAndFillsStore) {
  SeedEnvironment();
  CreateManager();

  OnGetSubscriberCredentialV12("valid-subscriber-credential", /*success=*/true);

  EXPECT_EQ(GetState(), PurchasedState::PURCHASED);
  EXPECT_TRUE(credential_store()->HasValidSubscriberCredential());
  EXPECT_EQ(credential_store()->GetSubscriberCredential(),
            "valid-subscriber-credential");
  // Filling the subscriber credential drops any SKUS credential, and the retry
  // guard is reset on success.
  EXPECT_FALSE(credential_store()->HasValidSkusCredential());
  EXPECT_FALSE(credential_store()->IsExchangeRetried());
}

#if !BUILDFLAG(IS_ANDROID)

TEST_F(PurchasedStateManagerTest, TokenNoLongerValidRetriesExactlyOnce) {
  SeedEnvironment();
  CreateManager();
  manager_->SetPurchasedState(env_, PurchasedState::LOADING);

  // First "token no longer valid" arms the guard and re-requests a summary;
  // state is unchanged while the retry is in flight.
  OnGetSubscriberCredentialV12(::brave_vpn::kTokenNoLongerValid,
                               /*success=*/false);
  EXPECT_EQ(GetState(), PurchasedState::LOADING);
  EXPECT_TRUE(credential_store()->IsExchangeRetried());
  const int calls_after_first = fake_skus_client_.credential_summary_calls();
  EXPECT_GE(calls_after_first, 1);

  // Second failure: the guard is set, so we give up rather than loop.
  OnGetSubscriberCredentialV12(::brave_vpn::kTokenNoLongerValid,
                               /*success=*/false);
  EXPECT_EQ(GetState(), PurchasedState::FAILED);
  // No additional summary request was made on the second failure.
  EXPECT_EQ(fake_skus_client_.credential_summary_calls(), calls_after_first);

  // A later successful exchange clears the guard.
  OnGetSubscriberCredentialV12("valid-subscriber-credential", /*success=*/true);
  EXPECT_FALSE(credential_store()->IsExchangeRetried());
  EXPECT_EQ(GetState(), PurchasedState::PURCHASED);
}

TEST_F(PurchasedStateManagerTest, OutOfCredentialsSummaryIsSessionExpired) {
  CreateAndLoadManager();
  // Subscription on record but nothing left to redeem.
  OnCredentialSummary(domain(),
                      R"({"active": false, "remaining_credential_count": 0})");
  EXPECT_EQ(GetState(), PurchasedState::SESSION_EXPIRED);
}

TEST_F(PurchasedStateManagerTest,
       OutOfCredentialsWithFutureExpiryIsOutOfCredentials) {
  // A future last-credential expiry means the user redeemed but we lost the
  // vendor: out of credentials rather than session expired.
  local_pref_service_.SetTime(prefs::kBraveVPNLastCredentialExpiry, Future());
  CreateAndLoadManager();
  OnCredentialSummary(domain(),
                      R"({"active": false, "remaining_credential_count": 0})");
  EXPECT_EQ(GetState(), PurchasedState::OUT_OF_CREDENTIALS);
  EXPECT_TRUE(last_description_.has_value());
}

TEST_F(PurchasedStateManagerTest,
       SessionExpiredWithinWindowStaysSessionExpired) {
  // A stamp in the past but within the checking window keeps SESSION_EXPIRED.
  local_pref_service_.SetTime(prefs::kBraveVPNSessionExpiredDate,
                              base::Time::Now() - base::Days(5));
  CreateAndLoadManager();
  OnCredentialSummary(domain(),
                      R"({"active": false, "remaining_credential_count": 0})");
  EXPECT_EQ(GetState(), PurchasedState::SESSION_EXPIRED);
}

TEST_F(PurchasedStateManagerTest,
       SessionExpiredBeyondWindowBecomesNotPurchased) {
  // First out-of-credentials stamps "now" and reports SESSION_EXPIRED.
  CreateAndLoadManager();
  OnCredentialSummary(domain(),
                      R"({"active": false, "remaining_credential_count": 0})");
  ASSERT_EQ(GetState(), PurchasedState::SESSION_EXPIRED);

  // After more than the checking duration elapses, a fresh out-of-credentials
  // result is treated as not purchased.
  task_environment_.FastForwardBy(base::Days(31));
  manager_->Load(domain());
  OnCredentialSummary(domain(),
                      R"({"active": false, "remaining_credential_count": 0})");
  EXPECT_EQ(GetState(), PurchasedState::NOT_PURCHASED);
}

TEST_F(PurchasedStateManagerTest,
       SessionExpiredStampInFutureBecomesNotPurchased) {
  // Defensive clock-skew branch: a stamp in the future is treated as not
  // purchased rather than session expired.
  local_pref_service_.SetTime(prefs::kBraveVPNSessionExpiredDate, Future());
  CreateAndLoadManager();
  OnCredentialSummary(domain(),
                      R"({"active": false, "remaining_credential_count": 0})");
  EXPECT_EQ(GetState(), PurchasedState::NOT_PURCHASED);
}

TEST_F(PurchasedStateManagerTest, ValidSummaryClearsSessionExpiredDate) {
  // A stale session-expired stamp is cleared once an active summary arrives.
  local_pref_service_.SetTime(prefs::kBraveVPNSessionExpiredDate, Past());
  CreateAndLoadManager();
  OnCredentialSummary(domain(),
                      R"({"active": true, "remaining_credential_count": 3})");
  EXPECT_TRUE(local_pref_service_.GetTime(prefs::kBraveVPNSessionExpiredDate)
                  .is_null());
}

#endif  // !BUILDFLAG(IS_ANDROID)

TEST_F(PurchasedStateManagerTest, RefreshTimerClearsAndReloadsAfterExpiry) {
  SeedEnvironment();
  CreateManager();

  OnGetSubscriberCredentialV12("valid-subscriber-credential", /*success=*/true);
  ASSERT_EQ(GetState(), PurchasedState::PURCHASED);
  const int calls_before = fake_skus_client_.credential_summary_calls();

  // Advance past the credential's expiration to fire the refresh timer.
  task_environment_.FastForwardBy(base::Days(31));

  // Refresh clears the cached credential and reloads, which requests a fresh
  // summary.
  EXPECT_FALSE(credential_store()->HasValidSubscriberCredential());
  EXPECT_EQ(fake_skus_client_.credential_summary_calls(), calls_before + 1);
}

TEST_F(PurchasedStateManagerTest,
       LoadingAnotherEnvironmentDoesNotDisturbCurrentState) {
  SeedEnvironment();
  CreateManager();

  OnGetSubscriberCredentialV12("subscriber-credential", /*success=*/true);
  ASSERT_EQ(GetState(), PurchasedState::PURCHASED);
  const int changes_before = state_change_count_;

  // A load for a different environment is ignored for the current env's state:
  // SetPurchasedState only acts when the environment matches.
  manager_->Load(DomainFor(OtherEnvironment()));
  EXPECT_EQ(GetState(), PurchasedState::PURCHASED);
  EXPECT_EQ(state_change_count_, changes_before);
}

TEST_F(PurchasedStateManagerTest,
       AuthorizingInNewEnvironmentSwitchesCurrentEnvironment) {
  local_pref_service_.SetString(prefs::kBraveVPNEnvironment, "");
  CreateManager();  // current environment is unset.

  // First load adopts that environment (the current one was empty).
  manager_->Load(DomainFor(skus::kEnvStaging));
  ASSERT_EQ(manager_->GetCurrentEnvironment(), skus::kEnvStaging);

  // A successful presentation for a different environment switches current.
  OnCredentialSummary(DomainFor(skus::kEnvProduction),
                      R"({"active": true, "remaining_credential_count": 3})");
  OnPrepareCredentialsPresentation(
      DomainFor(skus::kEnvProduction),
      MakeCredentialCookie("skus-credential", kFutureCookieExpiry));
  EXPECT_EQ(manager_->GetCurrentEnvironment(), skus::kEnvProduction);
}

}  // namespace brave_vpn::v2
