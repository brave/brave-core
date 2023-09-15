/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/json/values_util.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/ai_chat/browser/ai_chat_credential_manager.h"
#include "brave/components/ai_chat/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/common/pref_names.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/skus_context_impl.h"
#include "brave/components/skus/browser/skus_service_impl.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/features.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ai_chat {

class AIChatCredentialManagerUnitTest : public testing::Test {
 public:
  AIChatCredentialManagerUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {
    scoped_feature_list_.InitWithFeatures({skus::features::kSkusFeature}, {});
  }

  void SetUp() override {
    auto* registry = prefs_service_.registry();
    ai_chat::prefs::RegisterLocalStatePrefs(registry);
    skus::RegisterLocalStatePrefs(registry);

    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    skus_service_ = std::make_unique<skus::SkusServiceImpl>(
        &prefs_service_, url_loader_factory_.GetSafeWeakWrapper());

    ai_chat_credential_manager_ = std::make_unique<AIChatCredentialManager>(
        base::BindRepeating(&AIChatCredentialManagerUnitTest::GetSkusService,
                            base::Unretained(this)),
        &prefs_service_);
  }

  mojo::PendingRemote<skus::mojom::SkusService> GetSkusService() {
    if (!skus_service_) {
      return mojo::PendingRemote<skus::mojom::SkusService>();
    }
    return static_cast<skus::SkusServiceImpl*>(skus_service_.get())
        ->MakeRemote();
  }

  void TestGetPremiumStatus(ai_chat::mojom::PremiumStatus expected_status) {
    base::RunLoop run_loop;
    ai_chat_credential_manager_->GetPremiumStatus(
        base::BindLambdaForTesting([&](ai_chat::mojom::PremiumStatus status) {
          EXPECT_EQ(status, expected_status);
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestFetchPremiumCredential(
      absl::optional<CredentialCacheEntry> expected_credential) {
    base::RunLoop run_loop;
    ai_chat_credential_manager_->FetchPremiumCredential(
        base::BindLambdaForTesting(
            [&](absl::optional<CredentialCacheEntry> credential) {
              ASSERT_EQ(credential.has_value(),
                        expected_credential.has_value());
              if (credential && expected_credential) {
                EXPECT_EQ(credential->credential,
                          expected_credential->credential);
                EXPECT_EQ(credential->expires_at,
                          expected_credential->expires_at);
              }
              run_loop.Quit();
            }));
    run_loop.Run();
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple prefs_service_;
  base::test::ScopedFeatureList scoped_feature_list_;
  std::unique_ptr<skus::SkusServiceImpl> skus_service_;
  std::unique_ptr<AIChatCredentialManager> ai_chat_credential_manager_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
};

TEST_F(AIChatCredentialManagerUnitTest, CacheDefault) {
  // Should be an empty dictionary by default
  EXPECT_EQ(
      prefs_service_.GetDict(ai_chat::prefs::kBraveChatPremiumCredentialCache),
      base::Value::Dict());
}

TEST_F(AIChatCredentialManagerUnitTest, PutCredentialInCache) {
  CredentialCacheEntry entry;
  entry.credential = "credential";
  entry.expires_at = base::Time::Now();
  ai_chat_credential_manager_->PutCredentialInCache(entry);
  const auto& cached_creds_dict =
      prefs_service_.GetDict(ai_chat::prefs::kBraveChatPremiumCredentialCache);
  EXPECT_EQ(cached_creds_dict.size(), 1u);
  EXPECT_EQ(base::ValueToTime(*(cached_creds_dict.Find("credential"))),
            entry.expires_at);
}

TEST_F(AIChatCredentialManagerUnitTest, GetPremiumStatus) {
  // By default there should be no credentials.
  TestGetPremiumStatus(ai_chat::mojom::PremiumStatus::Disconnected);

  // Add an expired credential to the cache, should return false.
  CredentialCacheEntry entry;
  entry.credential = "credential";
  entry.expires_at = base::Time::Now() - base::Hours(1);  // Expired
  ai_chat_credential_manager_->PutCredentialInCache(entry);
  TestGetPremiumStatus(ai_chat::mojom::PremiumStatus::Disconnected);

  // Add valid credential to the cache, GetPremiumStatus should
  // return ai_chat::mojom::PremiumStatus::Active.
  entry.expires_at = base::Time::Now() + base::Hours(1);  // Valid
  ai_chat_credential_manager_->PutCredentialInCache(entry);
  TestGetPremiumStatus(ai_chat::mojom::PremiumStatus::Active);
}

TEST_F(AIChatCredentialManagerUnitTest, FetchPremiumCredential) {
  // If cache is empty, it should return nullopt.
  TestFetchPremiumCredential(absl::nullopt);

  // Add an invalid credential to the cache. FetchPremiumCredential should
  // not return it and should remove the invalid value from the cache.
  CredentialCacheEntry entry;
  entry.credential = "credential";
  entry.expires_at = base::Time::Now() - base::Hours(1);
  ai_chat_credential_manager_->PutCredentialInCache(entry);
  const auto& cached_creds_dict =
      prefs_service_.GetDict(ai_chat::prefs::kBraveChatPremiumCredentialCache);
  EXPECT_EQ(cached_creds_dict.size(), 1u);
  TestFetchPremiumCredential(absl::nullopt);
  const auto& cached_creds_list2 =
      prefs_service_.GetDict(ai_chat::prefs::kBraveChatPremiumCredentialCache);
  EXPECT_EQ(cached_creds_list2.size(), 0u);

  // Add a valid credential to the cache. FetchPremiumCredential should
  // return it an remove it from the cache.
  entry.expires_at = base::Time::Now() + base::Hours(2);
  ai_chat_credential_manager_->PutCredentialInCache(entry);
  TestFetchPremiumCredential(entry);
  const auto& cached_creds_list3 =
      prefs_service_.GetDict(ai_chat::prefs::kBraveChatPremiumCredentialCache);
  EXPECT_EQ(cached_creds_list3.size(), 0u);

  // Add two valid credentials to the cache. FetchPremiumCredential should
  // return the one that's expiring soonest, leaving the other in the cache.
  ai_chat_credential_manager_->PutCredentialInCache(entry);
  CredentialCacheEntry entry2;
  entry2.credential = "credential2";
  entry2.expires_at = base::Time::Now() + base::Hours(1);
  ai_chat_credential_manager_->PutCredentialInCache(entry2);
  TestFetchPremiumCredential(entry2);
  const auto& cached_creds_list4 =
      prefs_service_.GetDict(ai_chat::prefs::kBraveChatPremiumCredentialCache);
  EXPECT_EQ(cached_creds_list4.size(), 1u);
  TestFetchPremiumCredential(entry);
}

}  // namespace ai_chat
