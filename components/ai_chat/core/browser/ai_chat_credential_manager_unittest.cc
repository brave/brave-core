/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/browser/ai_chat_credential_manager.h"

#include <memory>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/json/values_util.h"
#include "base/memory/scoped_refptr.h"
#include "base/numerics/clamped_math.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom.h"
#include "brave/components/ai_chat/core/common/pref_names.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/skus_service_impl.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/features.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/struct_ptr.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace {

constexpr char kSkusStateValueTemplate[] = R"({
  "credentials": {
    "items": {
      "b7114ccc-b3a5-4951-9a5d-8b7a28731111": {
        "creds": [
          "fbdoW1uxsxUuQrPBrk20jS7BdyfOEh8NJHLcVhutzkgaJ8UcKyMw/nkOaMXTsme5XSW/uxJ0H+H14996nBEpYXgJJBYJrBlpL8gmQGgwQA2hbKZDBIwS+5fpU5Yo5RkG",
          "v2X4t5GuttVpz/oEx+jAxI66e+qFBSAD5VvoD8W7l+/vUX4mOODwizoilNDQ8zW3UKjHgevuInJNA+fmNQTbFQELvrhfSlkDBW/NBbMXW9JlPZcv4RIBL+SFuz3TTLAH"
        ],
        "item_id": "b7114ccc-b3a5-4951-9a5d-8b7a28731111",
        "state": "ActiveCredentials",
        "type": "time-limited-v2",
        "unblinded_creds": [
          {
            "issuer_id": "bfd1ad9e-e9cf-4c46-96d7-7350c52de34f",
            "unblinded_creds": [
              {
                "spent": false,
                "unblinded_cred": "e0ysOdVNd7ZMBhW8m+aZLAROZjRQosITScYvJ1StwOsjyENfroQZ/6BOdGSKSTlSc/Yhz0L+p/pemStozGDHsoIsDfYpGXxTX/aODh6TCZWOORXPSy6t5+h5iJajRakb"
              },
              {
                "spent": false,
                "unblinded_cred": "iUgmZwALkPgGmKOyjkf13edV1rpMuuPcePhFIPTCuzR9Vqt3kxmhfAXXUrvF+zFaCb8n5AbeUeeXD2lzNCN6tDKf1OYAWQjDA173p+pVZ7kF8tGa5lSR1pd4YccWQn4O"
              }
            ],
            "valid_from": "$1",
            "valid_to": "$2"
          },
          {
            "issuer_id": "bfd1ad9e-e9cf-4c46-96d7-7350c52de34f",
            "unblinded_creds": [
              {
                "spent": false,
                "unblinded_cred": "T3WTO/6P7TeHk7BWPuWDpjc9mSZSWGwvdtYxhSNvWRSg4u+P4UkHDYakdOJxelW338UqconXonwDhp9CIkM0iXRVUN6QzBa55HkB+HKJlEbKcrKbrilYP+PcSxVChHYl"
              },
              {
                "spent": false,
                "unblinded_cred": "UTvHRpo0ySNO4wv59E/h64AvArHkJ7sWvNAL4KadBre/kBKNCNTTB183/DWWgWgwmDG1YkqfTZb2T32wpnOe+OjXjBxx1/z68b5BMWMcXfKsfpAlGPGG2cZ4FrPQTiB2"
              }
            ],
            "valid_from": "$3",
            "valid_to": "$4"
          }
        ]
      }
    }
  },
  "orders": {
    "e24787ab-7bc3-46b9-bc05-65befb361111": {
      "created_at": "$1",
      "currency": "USD",
      "expires_at": "$5",
      "id": "e24787ab-7bc3-46b9-bc05-65befb361111",
      "items": [
        {
          "created_at": "$1",
          "credential_type": "time-limited-v2",
          "currency": "USD",
          "description": "brave-leo-premium",
          "id": "b7114ccc-b3a5-4951-9a5d-8b7a28731111",
          "location": "$6",
          "order_id": "e24787ab-7bc3-46b9-bc05-65befb361111",
          "price": 15,
          "quantity": 1,
          "sku": "brave-leo-premium",
          "subtotal": 15,
          "updated_at": "$1"
        }
      ],
      "last_paid_at": "$1",
      "location": "$6",
      "merchant_id": "brave.com",
      "metadata": {
        "num_intervals": 3,
        "num_per_interval": 192,
        "payment_processor": "stripe",
        "stripe_checkout_session_id": "cs_live_b1lZu8rs8O0CvxymIK5W0zeEVhaYqq6H5SvXMwAkkv5PDxiN4g2cSGlCNH"
      },
      "status": "paid",
      "total_price": 15,
      "updated_at": "$1"
    }
  },
  "promotions": null,
  "wallet": null
})";

std::string formatSkusStateValue(const base::Time start_time,
                                 int shift_days = 0) {
  std::vector<std::string> replacements;
  base::Time shifted_start_time = start_time + base::Days(shift_days);
  for (int i = 0; i < 5; ++i) {
    base::Time incremented_time = shifted_start_time + base::Days(i);
    base::Time::Exploded exploded;
    incremented_time.UTCExplode(&exploded);
    std::string formatted_time = base::StringPrintf(
        "%04d-%02d-%02dT%02d:%02d:%02d", exploded.year, exploded.month,
        exploded.day_of_month, exploded.hour, exploded.minute, exploded.second);

    replacements.push_back(formatted_time);
  }

#if defined(OFFICIAL_BUILD)
  const std::string skus_domain = "leo.brave.com";
#else
  const std::string skus_domain = "leo.bravesoftware.com";
#endif

  replacements.push_back(skus_domain);
  const std::string result = base::ReplaceStringPlaceholders(
      kSkusStateValueTemplate, replacements, nullptr);
  return result;
}

}  // namespace

namespace ai_chat {

class AIChatCredentialManagerUnitTest : public testing::Test {
 public:
  AIChatCredentialManagerUnitTest() {
    scoped_feature_list_.InitWithFeatures({skus::features::kSkusFeature}, {});
  }

  void SetUp() override {
    auto* registry = prefs_service_.registry();
    prefs::RegisterLocalStatePrefs(registry);
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

  void SetSkusState(const std::string& skusStateValue) {
    base::Value::Dict state;
    // Unofficial builds will use the unofficial default for
    // Leo SKUs, which is staging. Official builds will use
    // production by default.
#if defined(OFFICIAL_BUILD)
    state.Set("skus:production", skusStateValue);
#else
    state.Set("skus:staging", skusStateValue);
#endif
    prefs()->SetDict(skus::prefs::kSkusState, std::move(state));
  }

  void TestGetPremiumStatus(mojom::PremiumStatus expected_status,
                            mojom::PremiumInfoPtr expected_info) {
    base::RunLoop run_loop;
    ai_chat_credential_manager_->GetPremiumStatus(base::BindLambdaForTesting(
        [&](mojom::PremiumStatus status, mojom::PremiumInfoPtr info) {
          EXPECT_EQ(status, expected_status);
          if (expected_info) {
            ASSERT_TRUE(info);
            EXPECT_EQ(info->remaining_credential_count,
                      expected_info->remaining_credential_count);

            if (expected_info->next_active_at) {
              ASSERT_TRUE(info->next_active_at);
              EXPECT_EQ(info->next_active_at, expected_info->next_active_at);
            } else {
              EXPECT_FALSE(info->next_active_at);
            }
          } else {
            ASSERT_FALSE(info);
          }
          run_loop.Quit();
        }));
    run_loop.Run();
  }

  void TestFetchPremiumCredential(
      std::optional<CredentialCacheEntry> expected_credential) {
    base::RunLoop run_loop;
    ai_chat_credential_manager_->FetchPremiumCredential(
        base::BindLambdaForTesting(
            [&](std::optional<CredentialCacheEntry> credential) {
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

  PrefService* prefs() { return &prefs_service_; }

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
  EXPECT_EQ(prefs_service_.GetDict(prefs::kBraveChatPremiumCredentialCache),
            base::Value::Dict());
}

TEST_F(AIChatCredentialManagerUnitTest, PutCredentialInCache) {
  CredentialCacheEntry entry;
  entry.credential = "credential";
  entry.expires_at = base::Time::Now();
  ai_chat_credential_manager_->PutCredentialInCache(entry);
  const auto& cached_creds_dict =
      prefs_service_.GetDict(prefs::kBraveChatPremiumCredentialCache);
  EXPECT_EQ(cached_creds_dict.size(), 1u);
  EXPECT_EQ(base::ValueToTime(*(cached_creds_dict.Find("credential"))),
            entry.expires_at);
}
TEST_F(AIChatCredentialManagerUnitTest, GetPremiumStatusInactive) {
  // By default there should be no credentials.
  TestGetPremiumStatus(mojom::PremiumStatus::Inactive, nullptr);

  // Add an expired credential to the cache, should return false.
  CredentialCacheEntry entry;
  entry.credential = "credential";
  entry.expires_at = base::Time::Now() - base::Hours(1);  // Expired
  ai_chat_credential_manager_->PutCredentialInCache(entry);
  TestGetPremiumStatus(mojom::PremiumStatus::Inactive, nullptr);
}

TEST_F(AIChatCredentialManagerUnitTest, GetPremiumStatusActive) {
  // By default there should be no credentials.
  TestGetPremiumStatus(mojom::PremiumStatus::Inactive, nullptr);

  // Add an expired credential to the cache, status should be Inactive
  CredentialCacheEntry entry;
  entry.credential = "credential";
  entry.expires_at = base::Time::Now() - base::Hours(1);  // Expired
  ai_chat_credential_manager_->PutCredentialInCache(entry);
  TestGetPremiumStatus(mojom::PremiumStatus::Inactive, nullptr);

  // Add valid credential to the cache with an empty SkusState, GetPremiumStatus
  // should return Active status, but next_active_at is null.
  entry.expires_at = base::Time::Now() + base::Hours(1);  // Valid
  ai_chat_credential_manager_->PutCredentialInCache(entry);
  mojom::PremiumInfoPtr expected_premium_info =
      mojom::PremiumInfo::New(1, std::nullopt);
  TestGetPremiumStatus(mojom::PremiumStatus::Active,
                       std::move(expected_premium_info));

  // Add a valid SKUs state, which has 2 valid credentials. Including the 1 in
  // the cache, there should be 3 total. next_active_at should be the
  // second batch valid_from.
  base::Time start_time = base::Time::Now();
  base::Time::Exploded exploded;
  start_time.UTCExplode(&exploded);
  exploded.millisecond = 0;  // Trim off milliseconds because they are not
                             // included in the skusState.
  ASSERT_TRUE(base::Time::FromUTCExploded(exploded, &start_time));
  std::string skusStateValue = formatSkusStateValue(start_time);
  SetSkusState(skusStateValue);
  base::Time expected_next_active_at = start_time + base::Days(2);
  expected_premium_info = mojom::PremiumInfo::New(3, expected_next_active_at);
  TestGetPremiumStatus(mojom::PremiumStatus::Active,
                       std::move(expected_premium_info));

  // Remove the valid credential from the cache, and check status again.
  // next_active_at should be the same, but remaining_credential_count
  // should be one less.
  TestFetchPremiumCredential(entry);
  expected_premium_info = mojom::PremiumInfo::New(2, expected_next_active_at);
  TestGetPremiumStatus(mojom::PremiumStatus::Active,
                       std::move(expected_premium_info));

  // Set the SKUs state to be on the final batch. There will be remaining valid
  // credentials, but next_active_at will be null.
  skusStateValue = formatSkusStateValue(start_time, -2);
  SetSkusState(skusStateValue);
  expected_premium_info = mojom::PremiumInfo::New(2, std::nullopt);
  TestGetPremiumStatus(mojom::PremiumStatus::Active,
                       std::move(expected_premium_info));

  // ActiveDisconnected
  start_time += base::Days(30);
  skusStateValue = formatSkusStateValue(start_time);
  SetSkusState(skusStateValue);
  expected_premium_info = mojom::PremiumInfo::New(0, std::nullopt);
  TestGetPremiumStatus(mojom::PremiumStatus::ActiveDisconnected,
                       std::move(expected_premium_info));

  // Add a credential to the cache - would other wise be ActiveDisconnected
  // state based on the SKUs state, however, since there's a credential in the
  // cache, it's Active.
  ai_chat_credential_manager_->PutCredentialInCache(entry);
  expected_premium_info = mojom::PremiumInfo::New(1, std::nullopt);
  TestGetPremiumStatus(mojom::PremiumStatus::Active,
                       std::move(expected_premium_info));
}

TEST_F(AIChatCredentialManagerUnitTest, GetPremiumStatusActiveDisconnected) {
  // Would other wise be ActiveDisconnected state based on the SKUs state,
  // however, since there's a credential in the cache, it's Active.
  const std::string skusStateValue =
      formatSkusStateValue(base::Time::Now() + base::Days(30));
  SetSkusState(skusStateValue);
  mojom::PremiumInfoPtr expected_premium_info =
      mojom::PremiumInfo::New(0, std::nullopt);
  TestGetPremiumStatus(mojom::PremiumStatus::ActiveDisconnected,
                       std::move(expected_premium_info));
}

TEST_F(AIChatCredentialManagerUnitTest, FetchPremiumCredential) {
  // If cache is empty, it should return nullopt.
  TestFetchPremiumCredential(std::nullopt);

  // Add an invalid credential to the cache. FetchPremiumCredential should
  // not return it and should remove the invalid value from the cache.
  CredentialCacheEntry entry;
  entry.credential = "credential";
  entry.expires_at = base::Time::Now() - base::Hours(1);
  ai_chat_credential_manager_->PutCredentialInCache(entry);
  const auto& cached_creds_dict =
      prefs_service_.GetDict(prefs::kBraveChatPremiumCredentialCache);
  EXPECT_EQ(cached_creds_dict.size(), 1u);
  TestFetchPremiumCredential(std::nullopt);
  const auto& cached_creds_list2 =
      prefs_service_.GetDict(prefs::kBraveChatPremiumCredentialCache);
  EXPECT_EQ(cached_creds_list2.size(), 0u);

  // Add a valid credential to the cache. FetchPremiumCredential should
  // return it an remove it from the cache.
  entry.expires_at = base::Time::Now() + base::Hours(2);
  ai_chat_credential_manager_->PutCredentialInCache(entry);
  TestFetchPremiumCredential(entry);
  const auto& cached_creds_list3 =
      prefs_service_.GetDict(prefs::kBraveChatPremiumCredentialCache);
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
      prefs_service_.GetDict(prefs::kBraveChatPremiumCredentialCache);
  EXPECT_EQ(cached_creds_list4.size(), 1u);
  TestFetchPremiumCredential(entry);

  // Add two invalid credentials to the cache, and one valid
  EXPECT_EQ(cached_creds_list4.size(),
            0u);  // Verify cache is empty before this test case.
  CredentialCacheEntry entry3;
  entry3.credential = "credential3";
  entry3.expires_at = base::Time();
  CredentialCacheEntry entry4;
  entry4.credential = "credential4";
  entry4.expires_at = base::Time();
  CredentialCacheEntry entry5;
  entry5.credential = "credential5";
  entry5.expires_at = base::Time::Now() + base::Hours(2);
  ai_chat_credential_manager_->PutCredentialInCache(entry3);
  ai_chat_credential_manager_->PutCredentialInCache(entry4);
  ai_chat_credential_manager_->PutCredentialInCache(entry5);
  TestFetchPremiumCredential(entry5);
  EXPECT_EQ(cached_creds_list4.size(), 0u);
}

}  // namespace ai_chat
