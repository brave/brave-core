/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/browser/skus_service_impl.h"

#include <string>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

const char kTestVpnOrders[] = R"(
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
                                  "expires_at": "2022-05-13T00:00:00",
                                  "issued_at": "2022-05-11T00:00:00",
                                  "item_id": "424bc657-633f-4fcc-bd8e-92a51c8e4971",
                                  "token": "q7gunpfaAVvnoP6uvnLaZHLivyky1VmF4NqryK3Hx+dq67LNtA3KLx8251Pc5tLH"
                              },
                              {
                                  "expires_at": "2022-05-13T00:00:00",
                                  "issued_at": "2022-05-11T00:00:00",
                                  "item_id": "424bc657-633f-4fcc-bd8e-92a51c8e4971",
                                  "token": "8N35H12HrUElmWl9owqKOxg8/rHrtCPSuOHpjU4gv4xNcZUMrjnStqWo2NgwIhpZ"
                              }
                          ],
                          "item_id": "424bc657-633f-4fcc-bd8e-92a51c8e4971",
                          "type": "time-limited"
                      },
                      "463b9cca-8609-4255-a29e-0f2ac475af3b":
                      {
                          "creds":
                          [
                              {
                                  "expires_at": "2021-07-19T00:00:00",
                                  "issued_at": "2021-07-18T00:00:00",
                                  "item_id": "463b9cca-8609-4255-a29e-0f2ac475af3b",
                                  "token": "8N35H12HrUElmWl9owqKOxg8/rHrtCPSuOHpjU4gv4xNcZUMrjnStqWo2NgwIhpZ"
                              },
                              {
                                  "expires_at": "2022-06-14T00:00:00",
                                  "issued_at": "2022-06-13T00:00:00",
                                  "item_id": "463b9cca-8609-4255-a29e-0f2ac475af3b",
                                  "token": "q7gunpfaAVvnoP6uvnLaZHLivyky1VmF4NqryK3Hx+dq67LNtA3KLx8251Pc5tLH"
                              },
                              {
                                  "expires_at": "2021-07-19T00:00:00",
                                  "issued_at": "2021-07-18T00:00:00",
                                  "item_id": "463b9cca-8609-4255-a29e-0f2ac475af3b",
                                  "token": "8N35H12HrUElmWl9owqKOxg8/rHrtCPSuOHpjU4gv4xNcZUMrjnStqWo2NgwIhpZ"
                              }
                          ],
                          "item_id": "463b9cca-8609-4255-a29e-0f2ac475af3b",
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
                      "expires_at": "2022-06-14T14:36:02.579641",
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
                          },
                          {
                              "created_at": "2022-06-13T13:05:17.144570",
                              "credential_type": "time-limited",
                              "currency": "USD",
                              "description": "Brave VPN",
                              "id": "55555555-633f-4fcc-bd8e-92a51c8e4971",
                              "location": "{domain}",
                              "order_id": "33a8231a-7c69-47bd-a061-2045b9b1b890",
                              "price": 9.99,
                              "quantity": 1,
                              "sku": "brave-vpn-premium",
                              "subtotal": 9.99,
                              "updated_at": "2022-06-13T13:05:17.144570"
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
                  },
                  "7df66bcb-921e-424b-ad53-37885948fb34":
                  {
                      "created_at": "2022-06-13T14:35:28.313786",
                      "currency": "USD",
                      "expires_at": "{year}-06-14T14:36:02.579641",
                      "id": "7df66bcb-921e-424b-ad53-37885948fb34",
                      "items":
                      [
                          {
                              "created_at": "2022-06-13T14:35:28.313786",
                              "credential_type": "time-limited",
                              "currency": "USD",
                              "description": "Brave VPN",
                              "id": "463b9cca-8609-4255-a29e-0f2ac475af3",
                              "location": "{domain}",
                              "order_id": "7df66bcb-921e-424b-ad53-37885948fb34",
                              "price": 9.99,
                              "quantity": 1,
                              "sku": "brave-vpn-premium",
                              "subtotal": 9.99,
                              "updated_at": "2022-06-13T14:35:28.313786"
                          },
                          {
                              "created_at": "2022-06-13T14:35:28.313786",
                              "credential_type": "time-limited",
                              "currency": "USD",
                              "description": "Brave VPN",
                              "id": "00000000-633f-4fcc-bd8e-92a51c8e4971",
                              "location": "{domain}",
                              "order_id": "7df66bcb-921e-424b-ad53-37885948fb34",
                              "price": 9.99,
                              "quantity": 1,
                              "sku": "brave-vpn-premium",
                              "subtotal": 9.99,
                              "updated_at": "2022-06-13T14:35:28.313786"
                          }
                      ],
                      "last_paid_at": "2022-06-13T14:36:02.579656",
                      "location": "{domain}",
                      "merchant_id": "brave.com",
                      "metadata":
                      {
                          "stripe_checkout_session_id": null
                      },
                      "status": "paid",
                      "total_price": 9.99,
                      "updated_at": "2022-06-13T14:36:02.577786"
                  },
                  "2ba8231a-7c69-47bd-a061-2045b9b1b890":
                  {
                      "created_at": "2022-06-13T13:05:17.144570",
                      "currency": "USD",
                      "expires_at": "{year}-06-13T13:06:49.466071",
                      "id": "2ba8231a-7c69-47bd-a061-2045b9b1b890",
                      "items":
                      [
                          {
                              "created_at": "2022-06-13T13:05:17.144570",
                              "credential_type": "time-limited",
                              "currency": "USD",
                              "description": "Brave VPN",
                              "id": "424bc657-633f-4fcc-bd8e-92a51c8e4971",
                              "location": "{domain}",
                              "order_id": "2ba8231a-7c69-47bd-a061-2045b9b1b890",
                              "price": 9.99,
                              "quantity": 1,
                              "sku": "brave-vpn-premium",
                              "subtotal": 9.99,
                              "updated_at": "2022-06-13T13:05:17.144570"
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
              },
              "promotions": null,
              "wallet": null
          }
  )";

std::string GenerateTestingCreds(const std::string& domain) {
  auto value = base::JSONReader::Read(kTestVpnOrders);
  std::string json;
  base::JSONWriter::WriteWithOptions(
      value.value(), base::JSONWriter::OPTIONS_PRETTY_PRINT, &json);

  auto now = base::Time::Now();
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);
  base::ReplaceSubstringsAfterOffset(&json, 0, "{year}",
                                     std::to_string(exploded.year + 1));
  base::ReplaceSubstringsAfterOffset(&json, 0, "{domain}", domain);
  return json;
}

base::Value GetExpectedCreds(const std::string& json,
                             const std::string& order_id) {
  auto value = base::JSONReader::Read(json);
  EXPECT_TRUE(value);
  const auto* order_value = value->FindPath("orders." + order_id);
  EXPECT_TRUE(order_value);
  return order_value->Clone();
}
}  // namespace

class SkusServiceTestUnitTest : public testing::Test {
 public:
  SkusServiceTestUnitTest() = default;
  ~SkusServiceTestUnitTest() override = default;

 protected:
  void SetUp() override {
    skus::RegisterLocalStatePrefs(prefs_.registry());
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    url_loader_factory_.SetInterceptor(base::BindRepeating(
        &SkusServiceTestUnitTest::Interceptor, base::Unretained(this)));

    skus_service_ = std::make_unique<skus::SkusServiceImpl>(
        prefs(), url_loader_factory_.GetSafeWeakWrapper());
  }

  std::string GetCredentialsSummary(const std::string& domain) {
    std::string result;
    bool callback_called = false;
    skus_service_->CredentialSummary(
        domain, base::BindLambdaForTesting([&](const std::string& summary) {
          callback_called = true;
          result = summary;
        }));
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(callback_called);
    return result;
  }

  void Interceptor(const network::ResourceRequest& request) {
    url_loader_factory_.ClearResponses();
    url_loader_factory_.AddResponse(request.url.spec(), "{}");
  }
  PrefService* prefs() { return &prefs_; }

 private:
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<skus::SkusServiceImpl> skus_service_;
  TestingPrefServiceSimple prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(SkusServiceTestUnitTest, CredentialSummarySuccess) {
  base::Value state(base::Value::Type::DICT);
  auto env = skus::GetDefaultEnvironment();
  auto domain = skus::GetDomain("vpn", env);
  auto testing_payload = GenerateTestingCreds(domain);
  state.SetStringKey("skus:" + env, testing_payload);
  prefs()->Set(skus::prefs::kSkusState, std::move(state));
  auto credentials = GetCredentialsSummary(domain);
  EXPECT_FALSE(credentials.empty());
  auto credentials_json = base::JSONReader::Read(credentials);
  auto* order = credentials_json->FindPath("order");
  EXPECT_TRUE(order);
  EXPECT_EQ(*order, GetExpectedCreds(testing_payload,
                                     "7df66bcb-921e-424b-ad53-37885948fb34"));
}

TEST_F(SkusServiceTestUnitTest, CredentialSummaryFailed) {
  base::Value state(base::Value::Type::DICT);
  auto env = skus::GetDefaultEnvironment();
  auto domain = skus::GetDomain("vpn", env);
  auto testing_payload = GenerateTestingCreds(domain);
  auto payload_value = base::JSONReader::Read(testing_payload);
  auto* orders = payload_value->FindPath("orders");
  EXPECT_TRUE(orders);
  // Remove unexpired creds
  orders->RemoveKey("7df66bcb-921e-424b-ad53-37885948fb34");
  std::string json;
  base::JSONWriter::WriteWithOptions(
      payload_value.value(), base::JSONWriter::OPTIONS_PRETTY_PRINT, &json);
  // Save prefs with expired prefs only
  state.SetStringKey("skus:" + env, json);

  prefs()->Set(skus::prefs::kSkusState, std::move(state));
  auto credentials = GetCredentialsSummary(domain);
  EXPECT_TRUE(credentials.empty());
}

TEST_F(SkusServiceTestUnitTest, CredentialSummaryWrongEnv) {
  base::Value state(base::Value::Type::DICT);
  auto testing_payload = GenerateTestingCreds("vpn.brave.software");
  state.SetStringKey("skus:staging", testing_payload);
  prefs()->Set(skus::prefs::kSkusState, std::move(state));
  auto credentials = GetCredentialsSummary("vpn.brave.software");
  EXPECT_TRUE(credentials.empty());
}
