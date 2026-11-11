/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/network/http_client.h"

#include <memory>

#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_ads/core/browser/network/oblivious_http_feature.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "services/network/public/cpp/network_context_getter.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

network::mojom::NetworkContext* NoNetworkContext() {
  return nullptr;
}

}  // namespace

class BraveAdsHttpClientTest : public testing::Test {
 public:
  void SetUp() override {
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        kAdsObliviousHttpFeature, {{"should_support", "true"}});

    // Not joined Brave Rewards and no connected wallet by default, so OHTTP
    // is needed by default.
    prefs_.registry()->RegisterBooleanPref(brave_rewards::prefs::kEnabled,
                                           false);
    prefs_.registry()->RegisterStringPref(
        brave_rewards::prefs::kExternalWalletType, "");
    prefs_.registry()->RegisterBooleanPref(prefs::kSponsoredEnabled, true);
    prefs_.registry()->RegisterBooleanPref(prefs::kNotificationsEnabled, false);
    prefs_.registry()->RegisterBooleanPref(
        prefs::kShouldShowOnboardingNotification, false);

    local_state_.registry()->RegisterStringPref(prefs::kObliviousHttpKeyConfig,
                                                "");
    local_state_.registry()->RegisterTimePref(
        prefs::kObliviousHttpKeyConfigExpiresAt, base::Time());
  }

 protected:
  std::unique_ptr<HttpClient> CreateHttpClient() {
    return std::make_unique<HttpClient>(
        prefs_, local_state_, shared_url_loader_factory_,
        base::BindRepeating(&NoNetworkContext), /*use_ohttp_staging=*/false);
  }

  void JoinRewardsAndConnectWallet() {
    prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);
    prefs_.SetString(brave_rewards::prefs::kExternalWalletType, "uphold");
  }

  base::test::ScopedFeatureList scoped_feature_list_;
  base::test::TaskEnvironment task_environment_;

  TestingPrefServiceSimple prefs_;
  TestingPrefServiceSimple local_state_;

  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_{
      base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
          &url_loader_factory_)};
};

TEST_F(BraveAdsHttpClientTest,
       FetchesKeyConfigOnConstructionWhenNotJoinedRewardsOrNoConnectedWallet) {
  // Act
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();

  // Assert
  EXPECT_EQ(1, url_loader_factory_.NumPending());
}

TEST_F(BraveAdsHttpClientTest,
       FetchesKeyConfigOnConstructionWhenJoinedRewardsButNoConnectedWallet) {
  // Arrange
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);

  // Act
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();

  // Assert
  EXPECT_EQ(1, url_loader_factory_.NumPending());
}

TEST_F(BraveAdsHttpClientTest,
       FetchesKeyConfigOnConstructionWhenConnectedWalletButNotJoinedRewards) {
  // Arrange
  prefs_.SetString(brave_rewards::prefs::kExternalWalletType, "uphold");

  // Act
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();

  // Assert
  EXPECT_EQ(1, url_loader_factory_.NumPending());
}

TEST_F(BraveAdsHttpClientTest,
       DoesNotFetchKeyConfigOnConstructionWhenJoinedRewardsAndConnectedWallet) {
  // Arrange
  JoinRewardsAndConnectWallet();

  // Act
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();

  // Assert
  EXPECT_EQ(0, url_loader_factory_.NumPending());
}

TEST_F(BraveAdsHttpClientTest,
       StartsFetchingKeyConfigWhenWalletDisconnectedAfterConstruction) {
  // Arrange
  JoinRewardsAndConnectWallet();
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();
  ASSERT_EQ(0, url_loader_factory_.NumPending());

  // Act
  prefs_.SetString(brave_rewards::prefs::kExternalWalletType, "");

  // Assert
  EXPECT_EQ(1, url_loader_factory_.NumPending());
}

TEST_F(BraveAdsHttpClientTest,
       StartsFetchingKeyConfigWhenRewardsDisabledAfterConstruction) {
  // Arrange
  JoinRewardsAndConnectWallet();
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();
  ASSERT_EQ(0, url_loader_factory_.NumPending());

  // Act
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, false);

  // Assert
  EXPECT_EQ(1, url_loader_factory_.NumPending());
}

TEST_F(
    BraveAdsHttpClientTest,
    StopsFetchingKeyConfigWhenRewardsJoinedAndWalletConnectedAfterConstruction) {
  // Arrange
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();
  ASSERT_EQ(1, url_loader_factory_.NumPending());
  const GURL pending_url =
      url_loader_factory_.GetPendingRequest(0)->request.url;

  // Act
  JoinRewardsAndConnectWallet();
  url_loader_factory_.AddResponse(pending_url.spec(), "key-config");

  // Assert
  base::RunLoop run_loop;
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, run_loop.QuitClosure());
  run_loop.Run();
  EXPECT_TRUE(local_state_.GetString(prefs::kObliviousHttpKeyConfig).empty());
}

TEST_F(BraveAdsHttpClientTest,
       DoesNotFetchKeyConfigOnConstructionWhenOhttpFeatureIsDisabled) {
  // Arrange
  scoped_feature_list_.Reset();
  scoped_feature_list_.InitAndDisableFeature(kAdsObliviousHttpFeature);

  // Act
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();

  // Assert
  EXPECT_EQ(0, url_loader_factory_.NumPending());
}

TEST_F(BraveAdsHttpClientTest,
       DoesNotStartFetchingKeyConfigWhenNeedsOhttpButFeatureIsDisabled) {
  // Arrange
  scoped_feature_list_.Reset();
  scoped_feature_list_.InitAndDisableFeature(kAdsObliviousHttpFeature);
  JoinRewardsAndConnectWallet();
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();
  ASSERT_EQ(0, url_loader_factory_.NumPending());

  // Act
  prefs_.SetString(brave_rewards::prefs::kExternalWalletType, "");

  // Assert
  EXPECT_EQ(0, url_loader_factory_.NumPending());
}

TEST_F(BraveAdsHttpClientTest,
       StartsNewFetchWhenNeedsOhttpAgainWhileAFetchWasAbandoned) {
  // Arrange
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();
  ASSERT_EQ(1, url_loader_factory_.NumPending());
  JoinRewardsAndConnectWallet();

  // Act
  prefs_.SetString(brave_rewards::prefs::kExternalWalletType, "");

  // Assert
  EXPECT_EQ(2, url_loader_factory_.NumPending());
}

TEST_F(BraveAdsHttpClientTest,
       DoesNotFetchKeyConfigWhenAnUnrelatedPrefChanges) {
  // Arrange
  JoinRewardsAndConnectWallet();
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();
  ASSERT_EQ(0, url_loader_factory_.NumPending());

  // Act
  prefs_.SetBoolean(prefs::kShouldShowOnboardingNotification, true);

  // Assert
  EXPECT_EQ(0, url_loader_factory_.NumPending());
}

TEST_F(BraveAdsHttpClientTest,
       FetchesKeyConfigOnConstructionWhenNotificationAdsAreEnabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);
  prefs_.SetBoolean(prefs::kNotificationsEnabled, true);

  // Act
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();

  // Assert
  EXPECT_EQ(1, url_loader_factory_.NumPending());
}

TEST_F(
    BraveAdsHttpClientTest,
    DoesNotFetchKeyConfigOnConstructionWhenNotificationAdsEnabledButNotJoinedRewards) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  prefs_.SetBoolean(prefs::kNotificationsEnabled, true);

  // Act
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();

  // Assert
  EXPECT_EQ(0, url_loader_factory_.NumPending());
}

TEST_F(
    BraveAdsHttpClientTest,
    DoesNotFetchKeyConfigOnConstructionWhenNotificationAdsEnabledAndWalletConnected) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  prefs_.SetBoolean(prefs::kNotificationsEnabled, true);
  JoinRewardsAndConnectWallet();

  // Act
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();

  // Assert
  EXPECT_EQ(0, url_loader_factory_.NumPending());
}

TEST_F(BraveAdsHttpClientTest,
       DoesNotFetchKeyConfigOnConstructionWhenSponsoredAdsAreDisabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);

  // Act
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();

  // Assert
  EXPECT_EQ(0, url_loader_factory_.NumPending());
}

TEST_F(BraveAdsHttpClientTest,
       StartsFetchingKeyConfigWhenSponsoredAdsEnabledAfterConstruction) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();
  ASSERT_EQ(0, url_loader_factory_.NumPending());

  // Act
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);

  // Assert
  EXPECT_EQ(1, url_loader_factory_.NumPending());
}

TEST_F(BraveAdsHttpClientTest,
       StopsFetchingKeyConfigWhenSponsoredAdsDisabledAfterConstruction) {
  // Arrange
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();
  ASSERT_EQ(1, url_loader_factory_.NumPending());
  const GURL pending_url =
      url_loader_factory_.GetPendingRequest(0)->request.url;

  // Act
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  url_loader_factory_.AddResponse(pending_url.spec(), "key-config");

  // Assert
  base::RunLoop run_loop;
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, run_loop.QuitClosure());
  run_loop.Run();
  EXPECT_TRUE(local_state_.GetString(prefs::kObliviousHttpKeyConfig).empty());
}

TEST_F(BraveAdsHttpClientTest,
       DestroyingHttpClientWhileAFetchIsPendingDoesNotCrash) {
  // Arrange
  std::unique_ptr<HttpClient> http_client = CreateHttpClient();
  ASSERT_EQ(1, url_loader_factory_.NumPending());

  // Act & Assert (no crash)
  http_client.reset();
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);
}

TEST_F(BraveAdsHttpClientTest,
       StillStartsFetchingKeyConfigAfterCancelRequestsWhenNeedsOhttpAgain) {
  // Arrange
  JoinRewardsAndConnectWallet();
  const std::unique_ptr<HttpClient> http_client = CreateHttpClient();
  ASSERT_EQ(0, url_loader_factory_.NumPending());

  // `CancelRequests` is called whenever the ads service shuts down, e.g. when
  // the user disconnects their wallet. It must not permanently break the pref
  // observer that resumes fetching when OHTTP is needed again.
  http_client->CancelRequests();

  // Act
  prefs_.SetString(brave_rewards::prefs::kExternalWalletType, "");

  // Assert
  EXPECT_EQ(1, url_loader_factory_.NumPending());
}

}  // namespace brave_ads
