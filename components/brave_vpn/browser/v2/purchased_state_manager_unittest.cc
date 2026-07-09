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
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "brave/components/brave_vpn/browser/v2/credential_store.h"
#include "brave/components/brave_vpn/browser/v2/skus_service_client.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {
constexpr char kTestCredential[] = "test-credential";
constexpr char kNonVpnDomain[] = "talk.brave.com";

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

  const std::string& loading_environment() const {
    return manager_->loading_environment_;
  }
  uint64_t loading_sequence() const { return manager_->loading_sequence_; }

  void CallFinishLoad(const std::string& env,
                      mojom::PurchasedState state,
                      std::optional<std::string> description = std::nullopt) {
    manager_->FinishLoad(env, state, std::move(description));
  }
  void CallOnCredentialSummary(uint64_t sequence, const std::string& domain) {
    manager_->OnCredentialSummary(sequence, domain, nullptr);
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
  manager_->Load(DomainFor(OtherEnvironment()));

  // The load is in flight but nothing visible changed: no state update, no
  // environment switch before the commit point.
  EXPECT_EQ(loading_environment(), OtherEnvironment());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::NOT_PURCHASED);
  EXPECT_EQ(manager_->GetCurrentEnvironment(), CurrentEnvironment());
  EXPECT_EQ(skus_client_.credential_summary_calls(), 1);
  EXPECT_EQ(skus_client_.last_domain(), DomainFor(OtherEnvironment()));

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

  manager_->Load(DomainFor(OtherEnvironment()));
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
  manager_->Load(DomainFor(
      OtherEnvironment()));  // Cancels the visible load, runs silently.
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

// A silent load that times out stays invisible: no notification, no state
// change, no reload (the visible state was never LOADING), but the dedupe is
// released.
TEST_F(PurchasedStateManagerTest, SilentLoadTimeoutStaysInvisible) {
  CreateManager();
  EXPECT_EQ(skus_client_.credential_summary_calls(), 0);
  manager_->Load(DomainFor(OtherEnvironment()));
  EXPECT_EQ(skus_client_.credential_summary_calls(), 1);

  task_environment_.FastForwardBy(PurchasedStateManager::kLoadTimeout);

  EXPECT_TRUE(loading_environment().empty());
  EXPECT_EQ(manager_->GetInfo().state, mojom::PurchasedState::NOT_PURCHASED);
  EXPECT_EQ(skus_client_.credential_summary_calls(), 1);

  // Sentinel: the first notification observed must be the sentinel itself;
  // the silent timeout must not have posted anything before it.
  manager_->SetPurchasedState(CurrentEnvironment(),
                              mojom::PurchasedState::PURCHASED);
  collector_.WaitForChangeCount(1);
  ASSERT_EQ(collector_.changes().size(), 1u);
  EXPECT_EQ(collector_.changes()[0].first, mojom::PurchasedState::PURCHASED);
}

// A silent load that cancelled a visible load and then timed out must reload
// the current environment.
TEST_F(PurchasedStateManagerTest, SilentLoadTimeoutReloadsStrandedState) {
  CreateManager();
  manager_->Load(CurrentDomain());                // Visible LOADING.
  manager_->Load(DomainFor(OtherEnvironment()));  // Cancels it, runs silently.
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

}  // namespace brave_vpn::v2
