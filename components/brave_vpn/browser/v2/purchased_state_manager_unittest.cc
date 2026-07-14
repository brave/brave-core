/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/purchased_state_manager.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_writer.h"
#include "base/location.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/values_test_util.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_vpn/browser/v2/credential_store.h"
#include "brave/components/brave_vpn/browser/v2/skus_service_client.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/skus_service_impl.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/features.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "build/build_config.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_vpn::v2 {
namespace {
constexpr char kTestCredential[] = "test-credential";
constexpr char kNonVpnDomain[] = "talk.brave.com";

// Credential-summary payloads for each terminal branch.
constexpr char kValidSummary[] =
    R"({"active": true, "remaining_credential_count": 3})";
constexpr char kNeedsActivationSummary[] =
    R"({"active": false, "remaining_credential_count": 3})";
constexpr char kExpiredSummary[] =
    R"({"active": true, "remaining_credential_count": 0})";

// Credential cookies for tests.
// We can't use absolute dates in the tests because they use MOCK_TIME, which
// starts near the Unix epoch. So all the times should be relative to
// base::Time::Now() in tests.
std::string BuildTestCookie(base::Time expiry,
                            const std::string& value = "testvalue%3D") {
  return "credential=" + value + "; Expires=" + base::TimeFormatHTTP(expiry) +
         "; Path=/";
}

// Test VPN orders payload for the real SKUS service used in some tests.
// The {year} and {domain} placeholders are replaced with the mock year of
// expiry and the environment domain, respectively.
constexpr char kTestVpnOrders[] = R"(
{
    "credentials":
    {
        "items":
        {
            "424bc657-633f-4fcc-bd8e-92a51c8e4971":
            {
                "creds":
                [
                    {
                        "expires_at": "{year}-05-13T00:00:00",
                        "issued_at": "2022-05-11T00:00:00",
                        "item_id": "424bc657-633f-4fcc-bd8e-92a51c8e4971",
                        "token":
            "q7gunpfaAVvnoP6uvnLaZHLivyky1VmF4NqryK3Hx+dq67LNtA3KLx8251Pc5tLH"
                    }
                ],
                "item_id": "424bc657-633f-4fcc-bd8e-92a51c8e4971",
                "type": "time-limited"
            }
        }
    },
    "orders":
    {
        "33a8231a-7c69-47bd-a061-2045b9b1b890":
        {
            "created_at": "2022-06-13T13:05:17.144570",
            "currency": "USD",
            "expires_at": "{year}-06-14T14:36:02.579641",
            "id": "33a8231a-7c69-47bd-a061-2045b9b1b890",
            "items":
            [
                {
                    "created_at": "2022-06-13T14:35:28.313786",
                    "credential_type": "time-limited",
                    "currency": "USD",
                    "description": "Brave VPN",
                    "id": "424bc657-633f-4fcc-bd8e-92a51c8e4971",
                    "location": "{domain}",
                    "order_id": "33a8231a-7c69-47bd-a061-2045b9b1b890",
                    "price": 9.99,
                    "quantity": 1,
                    "sku": "brave-vpn-premium",
                    "subtotal": 9.99,
                    "updated_at": "2022-06-13T14:35:28.313786"
                }
            ],
            "last_paid_at": "2022-06-13T13:06:49.466083",
            "location": "{domain}",
            "merchant_id": "brave.com",
            "metadata":
            {
                "stripe_checkout_session_id": null
            },
            "status": "paid",
            "total_price": 9.99,
            "updated_at": "2022-06-13T13:06:49.465232"
        }
    }
})";

std::string GenerateTestingSkusCredential(const std::string& domain,
                                          bool active_subscription = true) {
  auto value = base::test::ParseJsonDict(kTestVpnOrders);
  std::string json;
  base::JSONWriter::WriteWithOptions(
      value, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json);

  base::Time now = base::Time::Now();
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);

  // Give sufficient additional years to prevent expiration.
  std::string year =
      base::NumberToString(active_subscription ? exploded.year + 10 : 0);
  base::ReplaceSubstringsAfterOffset(&json, 0, "{year}", year);
  base::ReplaceSubstringsAfterOffset(&json, 0, "{domain}", domain);
  return json;
}

// Extracts remaining_credential_count from a raw SKUS credential-summary
// result. Returns nullopt for errors or an empty summary body.
std::optional<int> RemainingCredentialCount(
    const skus::mojom::SkusResultPtr& result) {
  if (!result || result->code != skus::mojom::SkusResultCode::Ok ||
      result->message.empty()) {
    return std::nullopt;
  }
  return base::test::ParseJsonDict(result->message)
      .FindInt("remaining_credential_count");
}

// Records every purchased-state notification. WaitForChangeCount() drives a
// RunLoop until the expected number of notifications has arrived.
class StateChangeCollector {
 public:
  using Change = std::pair<mojom::PurchasedState, std::optional<std::string>>;

  PurchasedStateManager::PurchasedStateChangedCallback GetCallback() {
    return base::BindRepeating(&StateChangeCollector::OnChanged,
                               base::Unretained(this));
  }

  void WaitForChangeCount(size_t count) {
    if (changes_.size() >= count) {
      return;
    }
    expected_count_ = count;
    base::RunLoop run_loop;
    quit_closure_ = run_loop.QuitClosure();
    run_loop.Run();
  }

  void QuitAtChangeCount(size_t count, base::RepeatingClosure quit_closure) {
    if (changes_.size() >= count) {
      quit_closure.Run();
      return;
    }
    expected_count_ = count;
    quit_closure_ = std::move(quit_closure);
  }

  const std::vector<Change>& changes() const { return changes_; }

 private:
  void OnChanged(mojom::PurchasedState state,
                 std::optional<std::string> description) {
    changes_.emplace_back(state, std::move(description));
    if (quit_closure_ && changes_.size() >= expected_count_) {
      std::move(quit_closure_).Run();
    }
  }

  std::vector<Change> changes_;
  size_t expected_count_ = 0;
  base::OnceClosure quit_closure_;
};

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

skus::mojom::SkusResultPtr SkusOkResult(const std::string& message) {
  return skus::mojom::SkusResult::New(skus::mojom::SkusResultCode::Ok, message);
}

skus::mojom::SkusResultPtr SkusTransportError() {
  return skus::mojom::SkusResult::New(skus::mojom::SkusResultCode::UnknownError,
                                      "SkusService disconnected");
}

}  // namespace

class PurchasedStateManagerTest : public testing::Test {
 public:
  PurchasedStateManagerTest() {
    brave_vpn::RegisterLocalStatePrefs(local_pref_service_.registry());
  }

  void CreateManager() {
    manager_ = std::make_unique<PurchasedStateManager>(
        &local_pref_service_, &skus_client_, collector_.GetCallback());
  }

  // Environment/domain helpers. The "current" environment is whatever the
  // registered pref default is; "other" is any valid VPN environment that is
  // not the current one.
  std::string CurrentEnvironment() const {
    return local_pref_service_.GetString(prefs::kBraveVPNEnvironment);
  }
  std::string OtherEnvironment() const {
    return CurrentEnvironment() == skus::kEnvDevelopment
               ? skus::kEnvStaging
               : skus::kEnvDevelopment;
  }
  std::string DomainFor(const std::string& env) const {
    return skus::GetDomain(skus::GetVpnProductPrefix(), env);
  }
  std::string CurrentDomain() const { return DomainFor(CurrentEnvironment()); }
  std::string OtherDomain() const { return DomainFor(OtherEnvironment()); }

  const std::string& loading_environment() const {
    return manager_->loading_environment_;
  }
  uint64_t loading_sequence() const { return manager_->loading_sequence_; }

  void CallFinishLoad(const std::string& env,
                      mojom::PurchasedState state,
                      std::optional<std::string> description = std::nullopt) {
    manager_->FinishLoad(env, state, std::move(description));
  }
  void CallOnCredentialSummary(uint64_t sequence,
                               const std::string& domain,
                               skus::mojom::SkusResultPtr result = nullptr) {
    manager_->OnCredentialSummary(sequence, domain, std::move(result));
  }
  void CallOnPrepareCredentialsPresentation(
      uint64_t sequence,
      const std::string& domain,
      skus::mojom::SkusResultPtr result = nullptr) {
    manager_->OnPrepareCredentialsPresentation(sequence, domain,
                                               std::move(result));
  }

  void SeedSubscriberCredential(base::Time expiration) {
    credential_store_.SetSubscriberCredential(kTestCredential, expiration);
  }
  bool HasAnyStoredCredential() const {
    return credential_store_.HasAnyCredential();
  }

 protected:
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  TestingPrefServiceSimple local_pref_service_;
  CredentialStore credential_store_{&local_pref_service_};
  StateChangeCollector collector_;
  FakeSkusServiceClient skus_client_;
  std::unique_ptr<PurchasedStateManager> manager_;
};

TEST_F(PurchasedStateManagerTest, UnresolvedStateReadsAsNotPurchased) {
  CreateManager();

  const mojom::PurchasedInfo info = manager_->GetInfo();
  EXPECT_EQ(info.state, mojom::PurchasedState::NOT_PURCHASED);
  EXPECT_EQ(info.description, std::nullopt);
  EXPECT_FALSE(manager_->IsPurchased());
  EXPECT_EQ(skus_client_.credential_summary_calls(), 0);
}

TEST_F(PurchasedStateManagerTest, EnvironmentIsReadFromPrefs) {
  CreateManager();

  EXPECT_EQ(manager_->GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  const std::string other_env = OtherEnvironment();
  local_pref_service_.SetString(prefs::kBraveVPNEnvironment, other_env);
  EXPECT_EQ(manager_->GetCurrentEnvironment(), other_env);
}

TEST_F(PurchasedStateManagerTest, InitialStateWithValidCredentialIsPurchased) {
  SeedSubscriberCredential(base::Time::Now() + base::Days(30));
  CreateManager();

  // State is settled synchronously at construction, and the notification is
  // delivered asynchronously.
  EXPECT_TRUE(manager_->IsPurchased());
  EXPECT_EQ(skus_client_.credential_summary_calls(), 0);
  collector_.WaitForChangeCount(1);
  EXPECT_EQ(collector_.changes()[0].first, mojom::PurchasedState::PURCHASED);
}

TEST_F(PurchasedStateManagerTest, InitialStateClearsStaleCredential) {
  SeedSubscriberCredential(base::Time::Now() - base::Days(1));
  CreateManager();

  // The stale slot is cleared and a reload of the current environment starts:
  // no valid credentials, so the state becomes (visible) LOADING.
  EXPECT_FALSE(HasAnyStoredCredential());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::LOADING);
  EXPECT_EQ(loading_environment(), CurrentEnvironment());
  EXPECT_EQ(skus_client_.credential_summary_calls(), 1);
  EXPECT_EQ(skus_client_.last_domain(), CurrentDomain());
  collector_.WaitForChangeCount(1);
  EXPECT_EQ(collector_.changes()[0].first, mojom::PurchasedState::LOADING);
}

TEST_F(PurchasedStateManagerTest, LoadForCurrentEnvironmentNotifiesLoading) {
  CreateManager();
  manager_->Load(CurrentDomain());
  EXPECT_EQ(loading_environment(), CurrentEnvironment());
  EXPECT_EQ(skus_client_.credential_summary_calls(), 1);
  EXPECT_EQ(skus_client_.last_domain(), CurrentDomain());
  collector_.WaitForChangeCount(1);
  EXPECT_EQ(collector_.changes()[0].first, mojom::PurchasedState::LOADING);
}

TEST_F(PurchasedStateManagerTest, DuplicateLoadIsIgnored) {
  CreateManager();
  manager_->Load(CurrentDomain());
  const uint64_t original_sequence = loading_sequence();

  // Same environment while in flight: deduped, no new cycle.
  manager_->Load(CurrentDomain());
  EXPECT_EQ(loading_sequence(), original_sequence);
  EXPECT_EQ(skus_client_.credential_summary_calls(), 1);

  // Sentinel notification: if the duplicate had produced a second LOADING,
  // it would be observed before the sentinel.
  manager_->SetPurchasedState(CurrentEnvironment(),
                              mojom::PurchasedState::PURCHASED);
  collector_.WaitForChangeCount(2);
  ASSERT_EQ(collector_.changes().size(), 2u);
  EXPECT_EQ(collector_.changes()[0].first, mojom::PurchasedState::LOADING);
  EXPECT_EQ(collector_.changes()[1].first, mojom::PurchasedState::PURCHASED);
}

TEST_F(PurchasedStateManagerTest, NonVpnDomainIsIgnored) {
  CreateManager();
  manager_->Load(kNonVpnDomain);
  EXPECT_TRUE(loading_environment().empty());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::NOT_PURCHASED);
  EXPECT_EQ(skus_client_.credential_summary_calls(), 0);
}

TEST_F(PurchasedStateManagerTest, CrossEnvironmentLoadIsSilent) {
  CreateManager();
  manager_->Load(OtherDomain());

  // The load is in flight but nothing visible changed: no state update, no
  // environment switch before the commit point.
  EXPECT_EQ(loading_environment(), OtherEnvironment());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::NOT_PURCHASED);
  EXPECT_EQ(manager_->GetCurrentEnvironment(), CurrentEnvironment());
  EXPECT_EQ(skus_client_.credential_summary_calls(), 1);
  EXPECT_EQ(skus_client_.last_domain(), OtherDomain());

  // Sentinel: a visible load afterwards must produce the FIRST notification;
  // the silent load must not have posted anything.
  manager_->Load(CurrentDomain());
  collector_.WaitForChangeCount(1);
  ASSERT_EQ(collector_.changes().size(), 1u);
  EXPECT_EQ(collector_.changes()[0].first, mojom::PurchasedState::LOADING);
}

TEST_F(PurchasedStateManagerTest, CachedCredentialFastPathCancelsSilentLoad) {
  SeedSubscriberCredential(base::Time::Now() + base::Days(30));
  CreateManager();
  collector_.WaitForChangeCount(1);  // Initial PURCHASED.
  EXPECT_EQ(skus_client_.credential_summary_calls(), 0);

  manager_->Load(OtherDomain());
  ASSERT_EQ(loading_environment(), OtherEnvironment());
  const uint64_t silent_sequence = loading_sequence();
  EXPECT_EQ(skus_client_.credential_summary_calls(), 1);

  // A load for the current environment is served from cache and cancels the
  // in-flight silent load without another trip to SKUS.
  manager_->Load(CurrentDomain());
  EXPECT_TRUE(loading_environment().empty());
  EXPECT_GT(loading_sequence(), silent_sequence);
  EXPECT_TRUE(manager_->IsPurchased());
  EXPECT_EQ(skus_client_.credential_summary_calls(), 1);
}

TEST_F(PurchasedStateManagerTest, FinishLoadForCurrentEnvironmentNotifies) {
  CreateManager();
  manager_->Load(CurrentDomain());
  collector_.WaitForChangeCount(1);  // LOADING.

  CallFinishLoad(CurrentEnvironment(), mojom::PurchasedState::FAILED, "error");
  EXPECT_TRUE(loading_environment().empty());
  collector_.WaitForChangeCount(2);
  EXPECT_EQ(collector_.changes()[1].first, mojom::PurchasedState::FAILED);
  EXPECT_EQ(collector_.changes()[1].second, "error");
}

TEST_F(PurchasedStateManagerTest, FinishedSilentLoadReloadsStrandedState) {
  CreateManager();
  manager_->Load(CurrentDomain());  // Visible LOADING.
  collector_.WaitForChangeCount(1);
  manager_->Load(OtherDomain());  // Cancels the visible load, runs silently.
  ASSERT_EQ(loading_environment(), OtherEnvironment());
  EXPECT_EQ(skus_client_.credential_summary_calls(), 2);

  // Finish the silent load with a terminal outcome for the other environment.
  // The visible state was stranded at LOADING, so a reload of the current
  // environment is kicked.
  CallFinishLoad(OtherEnvironment(), mojom::PurchasedState::FAILED);
  EXPECT_EQ(loading_environment(), CurrentEnvironment());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::LOADING);
  EXPECT_EQ(skus_client_.credential_summary_calls(), 3);

  // Sentinel: the re-entered LOADING is value-deduped, so the next notification
  // is the sentinel itself.
  manager_->SetPurchasedState(CurrentEnvironment(),
                              mojom::PurchasedState::PURCHASED);
  collector_.WaitForChangeCount(2);
  ASSERT_EQ(collector_.changes().size(), 2u);
  EXPECT_EQ(collector_.changes()[1].first, mojom::PurchasedState::PURCHASED);
}

TEST_F(PurchasedStateManagerTest, StaleSummaryResponseIsDropped) {
  CreateManager();
  manager_->Load(CurrentDomain());
  const uint64_t sequence = loading_sequence();

  CallOnCredentialSummary(sequence - 1, CurrentDomain());
  EXPECT_EQ(loading_environment(), CurrentEnvironment());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::LOADING);
}

TEST_F(PurchasedStateManagerTest, VisibleLoadTimesOutAndUnblocksRetry) {
  CreateManager();
  EXPECT_EQ(skus_client_.credential_summary_calls(), 0);
  manager_->Load(CurrentDomain());
  EXPECT_EQ(skus_client_.credential_summary_calls(), 1);

  task_environment_.FastForwardBy(PurchasedStateManager::kLoadTimeout);

  EXPECT_TRUE(loading_environment().empty());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::FAILED);
  collector_.WaitForChangeCount(2);
  ASSERT_EQ(collector_.changes().size(), 2u);
  EXPECT_EQ(collector_.changes()[0].first, mojom::PurchasedState::LOADING);
  EXPECT_EQ(collector_.changes()[1].first, mojom::PurchasedState::FAILED);
  EXPECT_TRUE(collector_.changes()[1].second.has_value());

  // Before the timeout this Load would be swallowed by the "already loading"
  // dedupe; now it starts a fresh cycle.
  manager_->Load(CurrentDomain());
  EXPECT_EQ(loading_environment(), CurrentEnvironment());
  EXPECT_EQ(skus_client_.credential_summary_calls(), 2);
}

// A terminal outcome before the deadline must disarm the timer: no spurious
// FAILED later.
TEST_F(PurchasedStateManagerTest, TerminalOutcomeBeforeDeadlineStopsTimer) {
  CreateManager();
  manager_->Load(CurrentDomain());
  CallFinishLoad(CurrentEnvironment(), mojom::PurchasedState::PURCHASED);

  task_environment_.FastForwardBy(PurchasedStateManager::kLoadTimeout);
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::PURCHASED);
}

// A silent load that times out stays invisible even when there is a real
// visible state to protect: no notification, no state change, no reload (the
// visible state was never LOADING), the cached credential survives, but the
// dedupe is released.
TEST_F(PurchasedStateManagerTest, SilentLoadTimeoutStaysInvisible) {
  SeedSubscriberCredential(base::Time::Now() + base::Days(30));
  CreateManager();
  collector_.WaitForChangeCount(1);  // Initial PURCHASED.
  EXPECT_EQ(skus_client_.credential_summary_calls(), 0);
  manager_->Load(OtherDomain());
  EXPECT_EQ(skus_client_.credential_summary_calls(), 1);

  task_environment_.FastForwardBy(PurchasedStateManager::kLoadTimeout);

  EXPECT_TRUE(loading_environment().empty());
  EXPECT_TRUE(manager_->IsPurchased());
  EXPECT_TRUE(HasAnyStoredCredential());
  EXPECT_EQ(skus_client_.credential_summary_calls(), 1);
  ASSERT_EQ(collector_.changes().size(), 1u);

  // Sentinel: the next notification observed must be the sentinel itself;
  // the silent timeout must not have posted anything before it.
  manager_->SetPurchasedState(CurrentEnvironment(),
                              mojom::PurchasedState::SESSION_EXPIRED);
  collector_.WaitForChangeCount(2);
  ASSERT_EQ(collector_.changes().size(), 2u);
  EXPECT_EQ(collector_.changes()[1].first,
            mojom::PurchasedState::SESSION_EXPIRED);
}

// A silent load that cancelled a visible load and then timed out must reload
// the current environment.
TEST_F(PurchasedStateManagerTest, SilentLoadTimeoutReloadsStrandedState) {
  CreateManager();
  manager_->Load(CurrentDomain());  // Visible LOADING.
  manager_->Load(OtherDomain());    // Cancels it, runs silently.
  EXPECT_EQ(skus_client_.credential_summary_calls(), 2);

  task_environment_.FastForwardBy(PurchasedStateManager::kLoadTimeout);

  EXPECT_EQ(loading_environment(), CurrentEnvironment());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::LOADING);
  EXPECT_EQ(skus_client_.credential_summary_calls(), 3);
}

// FinishLoad bumps the loading sequence, so a response that raced the timeout
// and lost is dropped by the sequence check.
TEST_F(PurchasedStateManagerTest, LateResponseAfterTimeoutIsDropped) {
  CreateManager();
  manager_->Load(CurrentDomain());
  const uint64_t in_flight_sequence = loading_sequence();

  task_environment_.FastForwardBy(PurchasedStateManager::kLoadTimeout);
  ASSERT_EQ(manager_->GetInfo().state, mojom::PurchasedState::FAILED);
  EXPECT_EQ(loading_sequence(), in_flight_sequence + 1);

  CallOnCredentialSummary(in_flight_sequence, CurrentDomain());
  EXPECT_TRUE(loading_environment().empty());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::FAILED);
}

// An authoritative non-purchased verdict for the current environment drops
// the cached credential.
TEST_F(PurchasedStateManagerTest, FinishLoadClearsCredentialsOnNotPurchased) {
  CreateManager();
  SeedSubscriberCredential(base::Time::Now() - base::Days(1));
  ASSERT_TRUE(HasAnyStoredCredential());
  manager_->Load(CurrentDomain());

  CallFinishLoad(CurrentEnvironment(), mojom::PurchasedState::NOT_PURCHASED);
  EXPECT_FALSE(HasAnyStoredCredential());
}

// FAILED is transient, not a verdict: the cached credential survives so the
// next load can retry with it.
TEST_F(PurchasedStateManagerTest, FinishLoadPreservesCredentialsOnFailed) {
  CreateManager();
  SeedSubscriberCredential(base::Time::Now() - base::Days(1));
  ASSERT_TRUE(HasAnyStoredCredential());
  manager_->Load(CurrentDomain());

  CallFinishLoad(CurrentEnvironment(), mojom::PurchasedState::FAILED);
  EXPECT_TRUE(HasAnyStoredCredential());
}

// A silent load's verdict must not clear the current environment's credential
// (silent loads touch no persistent state).
TEST_F(PurchasedStateManagerTest, SilentFinishLoadPreservesCredentials) {
  CreateManager();
  SeedSubscriberCredential(base::Time::Now() - base::Days(1));
  ASSERT_TRUE(HasAnyStoredCredential());
  manager_->Load(OtherDomain());

  CallFinishLoad(OtherEnvironment(), mojom::PurchasedState::NOT_PURCHASED);
  EXPECT_TRUE(HasAnyStoredCredential());
}

TEST_F(PurchasedStateManagerTest,
       SummaryTransportErrorFailsLoadAndPreservesCredentials) {
  CreateManager();
  SeedSubscriberCredential(base::Time::Now() - base::Days(1));
  ASSERT_TRUE(HasAnyStoredCredential());
  manager_->Load(CurrentDomain());

  CallOnCredentialSummary(loading_sequence(), CurrentDomain(),
                          SkusTransportError());

  EXPECT_TRUE(loading_environment().empty());
  EXPECT_TRUE(HasAnyStoredCredential());
  collector_.WaitForChangeCount(2);
  EXPECT_EQ(collector_.changes()[1].first, mojom::PurchasedState::FAILED);
  EXPECT_TRUE(collector_.changes()[1].second.has_value());
}

TEST_F(PurchasedStateManagerTest, UnparseableSummaryFailsLoad) {
  CreateManager();
  manager_->Load(CurrentDomain());

  CallOnCredentialSummary(loading_sequence(), CurrentDomain(),
                          SkusOkResult("not json"));

  EXPECT_TRUE(loading_environment().empty());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::FAILED);
}

TEST_F(PurchasedStateManagerTest,
       EmptySummaryMeansNotPurchasedAndClearsCredentials) {
  CreateManager();
  SeedSubscriberCredential(base::Time::Now() - base::Days(1));
  ASSERT_TRUE(HasAnyStoredCredential());
  manager_->Load(CurrentDomain());

  CallOnCredentialSummary(loading_sequence(), CurrentDomain(),
                          SkusOkResult(""));

  EXPECT_TRUE(loading_environment().empty());
  EXPECT_FALSE(HasAnyStoredCredential());
  collector_.WaitForChangeCount(2);
  EXPECT_EQ(collector_.changes()[1].first,
            mojom::PurchasedState::NOT_PURCHASED);
}

TEST_F(PurchasedStateManagerTest, NeedsActivationSummaryMeansNotPurchased) {
  CreateManager();
  manager_->Load(CurrentDomain());

  CallOnCredentialSummary(loading_sequence(), CurrentDomain(),
                          SkusOkResult(kNeedsActivationSummary));

  EXPECT_TRUE(loading_environment().empty());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::NOT_PURCHASED);
}

// A valid summary is not a terminal outcome: the load continues into the
// credentials-presentation hop, still under the same cycle and timeout.
TEST_F(PurchasedStateManagerTest, ValidSummaryStartsCredentialsPresentation) {
  CreateManager();
  manager_->Load(CurrentDomain());
  const uint64_t sequence = loading_sequence();

  CallOnCredentialSummary(loading_sequence(), CurrentDomain(),
                          SkusOkResult(kValidSummary));

  EXPECT_EQ(skus_client_.prepare_presentation_calls(), 1);
  EXPECT_EQ(skus_client_.last_domain(), CurrentDomain());
  // Same load, still in flight.
  EXPECT_EQ(loading_sequence(), sequence);
  EXPECT_EQ(loading_environment(), CurrentEnvironment());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::LOADING);
}

TEST_F(PurchasedStateManagerTest, PresentationTransportErrorFailsLoad) {
  CreateManager();
  manager_->Load(CurrentDomain());

  CallOnCredentialSummary(loading_sequence(), CurrentDomain(),
                          SkusOkResult(kValidSummary));
  CallOnPrepareCredentialsPresentation(loading_sequence(), CurrentDomain(),
                                       SkusTransportError());

  EXPECT_TRUE(loading_environment().empty());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::FAILED);
}

TEST_F(PurchasedStateManagerTest, InvalidCredentialCookieFailsLoad) {
  CreateManager();
  manager_->Load(CurrentDomain());

  CallOnCredentialSummary(loading_sequence(), CurrentDomain(),
                          SkusOkResult(kValidSummary));
  CallOnPrepareCredentialsPresentation(loading_sequence(), CurrentDomain(),
                                       SkusOkResult("not a cookie at all"));

  EXPECT_TRUE(loading_environment().empty());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::FAILED);
}

TEST_F(PurchasedStateManagerTest, ExpiredCredentialCookieFailsLoad) {
  CreateManager();
  manager_->Load(CurrentDomain());

  CallOnCredentialSummary(loading_sequence(), CurrentDomain(),
                          SkusOkResult(kValidSummary));
  CallOnPrepareCredentialsPresentation(
      loading_sequence(), CurrentDomain(),
      SkusOkResult(BuildTestCookie(base::Time::Now() - base::Days(30))));

  EXPECT_TRUE(loading_environment().empty());
  collector_.WaitForChangeCount(2);
  EXPECT_EQ(collector_.changes()[1].first, mojom::PurchasedState::FAILED);
  EXPECT_EQ(
      collector_.changes()[1].second,
      l10n_util::GetStringUTF8(IDS_BRAVE_VPN_PURCHASE_CREDENTIALS_EXPIRED));
  EXPECT_FALSE(credential_store_.HasValidSkusCredential());
}

TEST_F(PurchasedStateManagerTest, EmptyCredentialCookieValueMeansNotPurchased) {
  CreateManager();
  manager_->Load(CurrentDomain());

  CallOnCredentialSummary(loading_sequence(), CurrentDomain(),
                          SkusOkResult(kValidSummary));
  CallOnPrepareCredentialsPresentation(
      loading_sequence(), CurrentDomain(),
      SkusOkResult(BuildTestCookie(base::Time::Now() + base::Days(30), "")));

  EXPECT_TRUE(loading_environment().empty());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::NOT_PURCHASED);
}

// The commit point for a current-environment load: the SKUS credential is
// cached, the environment is unchanged, and the load is NOT finished - the
// exchange hop is still pending, so the cycle and its timeout stay live.
TEST_F(PurchasedStateManagerTest, ValidCookieCachesSkusCredential) {
  CreateManager();
  manager_->Load(CurrentDomain());
  const uint64_t sequence = loading_sequence();

  CallOnCredentialSummary(loading_sequence(), CurrentDomain(),
                          SkusOkResult(kValidSummary));
  CallOnPrepareCredentialsPresentation(
      loading_sequence(), CurrentDomain(),
      SkusOkResult(
          BuildTestCookie(base::Time::Now() + base::Days(30), "test%3D")));

  EXPECT_TRUE(credential_store_.HasValidSkusCredential());
  EXPECT_EQ(credential_store_.GetSkusCredential(), "test=");
  EXPECT_EQ(manager_->GetCurrentEnvironment(), CurrentEnvironment());
  EXPECT_EQ(loading_sequence(), sequence);
  EXPECT_EQ(loading_environment(), CurrentEnvironment());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::LOADING);
}

// The commit point for a silent cross-environment load: authorization
// succeeded, so the environment switches, the SKUS credential is cached, and
// the load becomes visible (LOADING for the new current environment).
TEST_F(PurchasedStateManagerTest, ValidCookieCommitsEnvironmentSwitch) {
  CreateManager();
  const std::string other_env = OtherEnvironment();
  manager_->Load(OtherDomain());

  CallOnCredentialSummary(loading_sequence(), OtherDomain(),
                          SkusOkResult(kValidSummary));
  CallOnPrepareCredentialsPresentation(
      loading_sequence(), OtherDomain(),
      SkusOkResult(BuildTestCookie(base::Time::Now() + base::Days(30))));

  EXPECT_EQ(manager_->GetCurrentEnvironment(), other_env);
  EXPECT_TRUE(credential_store_.HasValidSkusCredential());
  // The load is now visible and still in flight for the new environment.
  EXPECT_EQ(loading_environment(), other_env);
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::LOADING);
  collector_.WaitForChangeCount(1);
  EXPECT_EQ(collector_.changes()[0].first, mojom::PurchasedState::LOADING);
}

#if !BUILDFLAG(IS_ANDROID)

// Expired summary while the last redeemed credential is still unexpired: the
// user ran out of credentials.
TEST_F(PurchasedStateManagerTest, ExpiredSummaryMeansOutOfCredentials) {
  CreateManager();
  local_pref_service_.SetTime(prefs::kBraveVPNLastCredentialExpiry,
                              base::Time::Now() + base::Hours(2));
  manager_->Load(CurrentDomain());

  CallOnCredentialSummary(loading_sequence(), CurrentDomain(),
                          SkusOkResult(kExpiredSummary));

  EXPECT_TRUE(loading_environment().empty());
  collector_.WaitForChangeCount(2);
  EXPECT_EQ(collector_.changes()[1].first,
            mojom::PurchasedState::OUT_OF_CREDENTIALS);
  EXPECT_TRUE(collector_.changes()[1].second.has_value());
}

// First expired summary since the last purchase: the session-expired date is
// stamped and SESSION_EXPIRED is reported.
TEST_F(PurchasedStateManagerTest, FirstExpiredSummaryStampsSessionExpired) {
  CreateManager();
  ASSERT_TRUE(local_pref_service_.GetTime(prefs::kBraveVPNSessionExpiredDate)
                  .is_null());
  manager_->Load(CurrentDomain());

  CallOnCredentialSummary(loading_sequence(), CurrentDomain(),
                          SkusOkResult(kExpiredSummary));

  EXPECT_EQ(local_pref_service_.GetTime(prefs::kBraveVPNSessionExpiredDate),
            base::Time::Now());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::SESSION_EXPIRED);
}

// SESSION_EXPIRED held longer than the grace period demotes to NOT_PURCHASED.
TEST_F(PurchasedStateManagerTest, SessionExpiredDemotedAfterGracePeriod) {
  CreateManager();
  local_pref_service_.SetTime(prefs::kBraveVPNSessionExpiredDate,
                              base::Time::Now() - base::Days(31));
  manager_->Load(CurrentDomain());

  CallOnCredentialSummary(loading_sequence(), CurrentDomain(),
                          SkusOkResult(kExpiredSummary));

  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::NOT_PURCHASED);
}

// Silent load's expired verdict must not touch the current environment's
// session bookkeeping or credentials, and must not notify.
TEST_F(PurchasedStateManagerTest, SilentExpiredSummaryTouchesNothing) {
  CreateManager();
  SeedSubscriberCredential(base::Time::Now() - base::Days(1));
  ASSERT_TRUE(HasAnyStoredCredential());
  manager_->Load(OtherDomain());

  CallOnCredentialSummary(loading_sequence(), OtherDomain(),
                          SkusOkResult(kExpiredSummary));

  EXPECT_TRUE(loading_environment().empty());
  EXPECT_TRUE(local_pref_service_.GetTime(prefs::kBraveVPNSessionExpiredDate)
                  .is_null());
  EXPECT_TRUE(HasAnyStoredCredential());
  EXPECT_EQ(manager_->GetCurrentEnvironment(), CurrentEnvironment());

  // Sentinel: the first observed notification must be the sentinel itself.
  manager_->SetPurchasedState(CurrentEnvironment(),
                              mojom::PurchasedState::PURCHASED);
  collector_.WaitForChangeCount(1);
  ASSERT_EQ(collector_.changes().size(), 1u);
  EXPECT_EQ(collector_.changes()[0].first, mojom::PurchasedState::PURCHASED);
}

// A committed load clears the session-expired stamp: active credentials were
// obtained, so the expired narrative is over.
TEST_F(PurchasedStateManagerTest, CommitClearsSessionExpiredDate) {
  CreateManager();
  local_pref_service_.SetTime(prefs::kBraveVPNSessionExpiredDate,
                              base::Time::Now() - base::Days(1));
  manager_->Load(CurrentDomain());

  CallOnCredentialSummary(loading_sequence(), CurrentDomain(),
                          SkusOkResult(kValidSummary));
  CallOnPrepareCredentialsPresentation(
      loading_sequence(), CurrentDomain(),
      SkusOkResult(BuildTestCookie(base::Time::Now() + base::Days(30))));

  EXPECT_TRUE(local_pref_service_.GetTime(prefs::kBraveVPNSessionExpiredDate)
                  .is_null());
}

#else  // !BUILDFLAG(IS_ANDROID)

// Android has no session-expired routing: an expired summary is an
// authoritative non-purchased verdict, so the credential store is cleared.
TEST_F(PurchasedStateManagerTest, ExpiredSummaryMeansNotPurchased) {
  CreateManager();
  SeedSubscriberCredential(base::Time::Now() - base::Days(1));
  manager_->Load(CurrentDomain());

  CallOnCredentialSummary(loading_sequence(), CurrentDomain(),
                          SkusOkResult(kExpiredSummary));

  EXPECT_TRUE(loading_environment().empty());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::NOT_PURCHASED);
  EXPECT_FALSE(HasAnyStoredCredential());
}

#endif  // !BUILDFLAG(IS_ANDROID)

class PurchasedStateManagerWithRealSkusServiceTest
    : public PurchasedStateManagerTest {
 public:
  PurchasedStateManagerWithRealSkusServiceTest() : PurchasedStateManagerTest() {
    scoped_feature_list_.InitWithFeatures({skus::features::kSkusFeature}, {});
    skus::RegisterLocalStatePrefs(local_pref_service_.registry());
  }

  void SetUp() override {
    // The real SKUS service shares the mocked clock. The clock must sit inside
    // the seeded credential's validity window: after kTestVpnOrders' issued_at
    // (2022-05-11); the {year}+10 substitution handles the far end.
    base::Time future_mock_time;
    ASSERT_TRUE(base::Time::FromString("2023-01-04", &future_mock_time));
    task_environment_.AdvanceClock(future_mock_time - base::Time::Now());

    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    skus_service_ = std::make_unique<skus::SkusServiceImpl>(
        &local_pref_service_, url_loader_factory_.GetSafeWeakWrapper());
  }

  void TearDown() override {
    manager_.reset();
    real_skus_client_.reset();
    skus_service_.reset();
  }

  void CreateManager(const std::string& env, bool active_subscription = true) {
    // Seed the SKUS's per-environment state under skus::prefs::kSkusState.
    // SkusServiceImpl reads it live, keyed as "skus:{environment}" - the same
    // keys it resolves from the request domain.
    base::DictValue state;
    state.Set("skus:" + env, GenerateTestingSkusCredential(
                                 DomainFor(env), active_subscription));
    local_pref_service_.SetDict(skus::prefs::kSkusState, std::move(state));
    CreateRealChain();
  }

  void CreateManagerWithBothEnvironments() {
    base::DictValue state;
    for (const std::string& env : {CurrentEnvironment(), OtherEnvironment()}) {
      state.Set("skus:" + env, GenerateTestingSkusCredential(DomainFor(env)));
    }
    local_pref_service_.SetDict(skus::prefs::kSkusState, std::move(state));
    CreateRealChain();
  }

  // No SKUS state at all: "no subscription on record" for every environment.
  void CreateManagerWithEmptySkusState() { CreateRealChain(); }

  void CreateRealChain() {
    real_skus_client_ = std::make_unique<SkusServiceClient>(base::BindRepeating(
        &PurchasedStateManagerWithRealSkusServiceTest::GetSkusService,
        base::Unretained(this)));
    manager_ = std::make_unique<PurchasedStateManager>(
        &local_pref_service_, real_skus_client_.get(),
        collector_.GetCallback());
  }

  mojo::PendingRemote<skus::mojom::SkusService> GetSkusService() {
    return skus_service_->MakeRemote();
  }

  void WaitForCommitOrTerminalOutcome(size_t terminal_change_count) {
    base::RunLoop run_loop;
    PrefChangeRegistrar registrar;
    registrar.Init(&local_pref_service_);
    registrar.Add(prefs::kBraveVPNSubscriberCredential, run_loop.QuitClosure());
    collector_.QuitAtChangeCount(terminal_change_count, run_loop.QuitClosure());

    // Deadline guard: a silent load that fails produces neither a pref write
    // nor a visible notification, which would hang this loop forever. Quit just
    // past the load timeout instead (instant under MOCK_TIME auto-advance) and
    // let the assertions report the failure state. RunLoop::QuitClosure() is
    // safe to run after the loop has exited.
    base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE, run_loop.QuitClosure(),
        PurchasedStateManager::kLoadTimeout + base::Seconds(1));

    run_loop.Run();
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  std::unique_ptr<skus::SkusServiceImpl> skus_service_;
  std::unique_ptr<SkusServiceClient> real_skus_client_;
};

// The full chain against the real SKUS: summary JSON parses, the presentation
// cookie survives ParsedCookie + URL-decode, and the SKUS credential is cached
// at the commit point - all without touching the network.
TEST_F(PurchasedStateManagerWithRealSkusServiceTest,
       SummaryAndPresentationCommit) {
  CreateManager(CurrentEnvironment());
  manager_->Load(CurrentDomain());

  WaitForCommitOrTerminalOutcome(2);  // LOADING + terminal.

  ASSERT_EQ(url_loader_factory_.NumPending(), 0);
  EXPECT_TRUE(credential_store_.HasValidSkusCredential());
  EXPECT_FALSE(credential_store_.GetSkusCredential().empty());
  EXPECT_EQ(manager_->GetCurrentEnvironment(), CurrentEnvironment());
  EXPECT_EQ(loading_environment(), CurrentEnvironment());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::LOADING);
}

// PrepareCredentialsPresentation is idempotent within a credential's validity
// period: a second full chain yields the same presented credential and the
// summary's remaining count is unchanged.
TEST_F(PurchasedStateManagerWithRealSkusServiceTest,
       RepeatedPresentationIsIdempotent) {
  CreateManager(CurrentEnvironment());

  base::test::TestFuture<skus::mojom::SkusResultPtr> before;
  skus_service_->CredentialSummary(CurrentDomain(), before.GetCallback());
  const std::optional<int> remaining_before =
      RemainingCredentialCount(before.Get());
  ASSERT_TRUE(remaining_before.has_value());

  manager_->Load(CurrentDomain());
  WaitForCommitOrTerminalOutcome(2);  // LOADING + terminal.
  ASSERT_TRUE(credential_store_.HasValidSkusCredential());
  const std::string first_credential = credential_store_.GetSkusCredential();

  // Release the in-flight load (FAILED preserves the cache), then clear the
  // store so the next Load() runs the full chain instead of the fast paths.
  CallFinishLoad(CurrentEnvironment(), mojom::PurchasedState::FAILED);
  credential_store_.Clear();
  manager_->Load(CurrentDomain());
  // Changes so far: LOADING, FAILED, LOADING(new load). The commit's LOADING
  // is value-deduped, so the terminal fallback is change #4.
  WaitForCommitOrTerminalOutcome(4);
  ASSERT_TRUE(credential_store_.HasValidSkusCredential());
  EXPECT_EQ(credential_store_.GetSkusCredential(), first_credential);

  base::test::TestFuture<skus::mojom::SkusResultPtr> after;
  skus_service_->CredentialSummary(CurrentDomain(), after.GetCallback());
  EXPECT_EQ(RemainingCredentialCount(after.Get()), remaining_before);
  EXPECT_EQ(url_loader_factory_.NumPending(), 0);
}

// No SKUS state on record: the real SKUS's "no subscription" summary must land
// in the "empty" handler (not a "parse failure", not a network fetch), and the
// manager settles as NOT_PURCHASED.
TEST_F(PurchasedStateManagerWithRealSkusServiceTest,
       EmptySkusStateMeansNotPurchased) {
  CreateManagerWithEmptySkusState();
  manager_->Load(CurrentDomain());

  WaitForCommitOrTerminalOutcome(2);  // LOADING + terminal.

  EXPECT_EQ(url_loader_factory_.NumPending(), 0);
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::NOT_PURCHASED);
  EXPECT_TRUE(loading_environment().empty());
  EXPECT_FALSE(HasAnyStoredCredential());
}

// A silent cross-environment load against the real SKUS: pins the three-way
// naming agreement between GetEnvironmentForDomain(), the per-environment
// domains, and the SKUS's "skus:{env}" state keys, through to a real commit.
TEST_F(PurchasedStateManagerWithRealSkusServiceTest,
       SilentLoadCommitsEnvironmentSwitch) {
  CreateManagerWithBothEnvironments();
  const std::string other_env = OtherEnvironment();
  manager_->Load(OtherDomain());

  // The load is silent, so the first (and only) visible change is the
  // post-commit LOADING for the new current environment.
  WaitForCommitOrTerminalOutcome(1);

  ASSERT_EQ(url_loader_factory_.NumPending(), 0);
  EXPECT_EQ(manager_->GetCurrentEnvironment(), other_env);
  EXPECT_TRUE(credential_store_.HasValidSkusCredential());
  EXPECT_EQ(loading_environment(), other_env);
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::LOADING);
}

}  // namespace brave_vpn::v2
