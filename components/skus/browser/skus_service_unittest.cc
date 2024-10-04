/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/skus/browser/skus_service_impl.h"

#include <string>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
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

constexpr char kTestVpnOrders[] = R"(
          {
              "credentials":
              {
                  "items":
                  {
                      "93e2e06b-bb69-47e9-8dff-270b53938157":
                      {
                          "creds":
                          [
                              "pvGUZncSMLz5v4eW5oJVe9NEYpE09HDryFTBDjDE3XrLpwZh2Xwi/l3tU6UBdeGHNZw0KEFluZQPRZt0PRiTPIH0A8NnXy52Al1gQF9D5fmZ4Do1awJM2NWrdCh6GlUP",
                              "FXg607b9IPPe6k65y1A76g7qV6+wMNZAlwvMarZ+fAbIGv4wJb/sneWcHmN9rvDJ7wyeupGPj08po6ymIln9ESX8ODU/9Ng0zhBiG0o4mGYj5uwT6QVDSocBekwmOdIG",
                              "PvtiOx8Nt300OFozXLZqcJAxTzuq+wxOiAKi44r0x33aoCZcOmJ7qZrWUGfECB4Ueq4XP9F8tarNaZqd8MbxUWQxVIiwR1FoffZqVV03uoXfCIgD+DOAtNg52oW1pusF"
                          ],
                          "item_id": "93e2e06b-bb69-47e9-8dff-270b53938157",
                          "state": "ActiveCredentials",
                          "type": "time-limited-v2",
                          "unblinded_creds":
                          [
                              {
                                  "issuer_id": "8dc09cb0-a69b-4695-8288-7bf716615385",
                                  "unblinded_creds":
                                  [
                                      {
                                          "spent": false,
                                          "unblinded_cred": "nfjj7YWTOJ3Ct7kAB2eyBzdhxOg1RKItWOgJdMCkj33ksrTE2FMvGjI12gu2UdgGJP1d2QTzHKh7k/aIIaUV+F4vjG/FpaWS0VShkPk2I8ODI2aNgcEUDHC0zeRIpjkg"
                                      },
                                      {
                                          "spent": false,
                                          "unblinded_cred": "c6UZLQYdkgUK0pqvyvawr1sDqGrJ/d67hWl3J69+f0qlTTreNNjrYZyw+IzXSHimTgYrzF7gRc5sG2nv+d8Nrqxaf7P4f/yPL5f7XnaM6C2BVxS+P7Maz1+Ibkk2mbwA"
                                      }
                                  ],
                                  "valid_from": "2022-06-19T19:31:40",
                                  "valid_to": "{year}-06-20T19:31:40"
                              },
                              {
                                  "issuer_id": "8dc09cb0-a69b-4695-8288-7bf716615385",
                                  "unblinded_creds":
                                  [
                                      {
                                          "spent": false,
                                          "unblinded_cred": "Lw06d7LyqIAhjKr7HynJ2+8vGJNBknzYkhsAHBV592q650mzswoUa1ob6s6ALH3mFCgAhELFPrDM2BAue003okp0aMSgDtZOVSYm1i1HYyEBeCms8dqsEJ0PZom+Kd8W"
                                      },
                                      {
                                          "spent": false,
                                          "unblinded_cred": "UuDemArRSL/WW1tLIePymNWGVPDYrj0dDzKqBrHkIvWuGImfJW/7mpQa9VwU4Ac1J/bnIPsuS8lnXogfLfcfxIwtYK6KSj42zCu7s4E4xQuwKi1wlprliKRVv6SnwZlX"
                                      }
                                  ],
                                  "valid_from": "2022-06-20T19:31:40",
                                  "valid_to": "{year}-06-21T19:31:40"
                              },
                              {
                                  "issuer_id": "8dc09cb0-a69b-4695-8288-7bf716615385",
                                  "unblinded_creds":
                                  [
                                      {
                                          "spent": false,
                                          "unblinded_cred": "48XvG94GrosHjnU38gsfF7maMwFOkPmUjxRBo/VU3tzUkUdue0LEZQIgeKlO2MKujKToDAn5GWP9RAl5sKiLpX62pTXLpS7fQ41CdyREBU9Jdc2hf0eRTdJsbdm6mL9Z"
                                      },
                                      {
                                          "spent": false,
                                          "unblinded_cred": "hSHfygeq++tl3SgkhygbjyvNTWdnMjJlt51To6KOb19SlgJ5kUMZEvsw6H1/e0MV+KyFBeJTd24ED6kzbac2ugDi8aXnBoucVyeJa3XBtSvqcAJhRu8VABv6IfxW81AT"
                                      }
                                  ],
                                  "valid_from": "2022-06-21T19:31:40",
                                  "valid_to": "{year}-06-22T19:31:40"
                              }
                          ]
                      },
                      "424bc657-633f-4fcc-bd8e-92a51c8e4971":
                      {
                          "creds":
                          [
                              "pvGUZncSMLz5v4eW5oJVe9NEYpE09HDryFTBDjDE3XrLpwZh2Xwi/l3tU6UBdeGHNZw0KEFluZQPRZt0PRiTPIH0A8NnXy52Al1gQF9D5fmZ4Do1awJM2NWrdCh6GlUP",
                              "FXg607b9IPPe6k65y1A76g7qV6+wMNZAlwvMarZ+fAbIGv4wJb/sneWcHmN9rvDJ7wyeupGPj08po6ymIln9ESX8ODU/9Ng0zhBiG0o4mGYj5uwT6QVDSocBekwmOdIG",
                              "u3aByl/KnY/yuVPhWWDodB0w7uhmS0RW0V3n8WHkn0JlBTcCgBp0HIzLxqgdzKWOEFrIL7nLYul/qjLbf2HBKj08n1JkDpzLLy2NmizBrP13pzMeZ8PBED3ArU9jfvoB"
                          ],
                          "item_id": "424bc657-633f-4fcc-bd8e-92a51c8e4971",
                          "state": "ActiveCredentials",
                          "type": "time-limited-v2",
                          "unblinded_creds":
                          [
                              {
                                  "issuer_id": "8dc09cb0-a69b-4695-8288-7bf716615385",
                                  "unblinded_creds":
                                  [
                                      {
                                          "spent": false,
                                          "unblinded_cred": "nfjj7YWTOJ3Ct7kAB2eyBzdhxOg1RKItWOgJdMCkj33ksrTE2FMvGjI12gu2UdgGJP1d2QTzHKh7k/aIIaUV+F4vjG/FpaWS0VShkPk2I8ODI2aNgcEUDHC0zeRIpjkg"
                                      },
                                      {
                                          "spent": false,
                                          "unblinded_cred": "c6UZLQYdkgUK0pqvyvawr1sDqGrJ/d67hWl3J69+f0qlTTreNNjrYZyw+IzXSHimTgYrzF7gRc5sG2nv+d8Nrqxaf7P4f/yPL5f7XnaM6C2BVxS+P7Maz1+Ibkk2mbwA"
                                      }
                                  ],
                                  "valid_from": "2022-06-19T19:31:40",
                                  "valid_to": "2022-06-20T19:31:40"
                              },
                              {
                                  "issuer_id": "8dc09cb0-a69b-4695-8288-7bf716615385",
                                  "unblinded_creds":
                                  [
                                      {
                                          "spent": false,
                                          "unblinded_cred": "Lw06d7LyqIAhjKr7HynJ2+8vGJNBknzYkhsAHBV592q650mzswoUa1ob6s6ALH3mFCgAhELFPrDM2BAue003okp0aMSgDtZOVSYm1i1HYyEBeCms8dqsEJ0PZom+Kd8W"
                                      },
                                      {
                                          "spent": false,
                                          "unblinded_cred": "UuDemArRSL/WW1tLIePymNWGVPDYrj0dDzKqBrHkIvWuGImfJW/7mpQa9VwU4Ac1J/bnIPsuS8lnXogfLfcfxIwtYK6KSj42zCu7s4E4xQuwKi1wlprliKRVv6SnwZlX"
                                      }
                                  ],
                                  "valid_from": "2022-06-20T19:31:40",
                                  "valid_to": "2022-06-21T19:31:40"
                              },
                              {
                                  "issuer_id": "8dc09cb0-a69b-4695-8288-7bf716615385",
                                  "unblinded_creds":
                                  [
                                      {
                                          "spent": false,
                                          "unblinded_cred": "48XvG94GrosHjnU38gsfF7maMwFOkPmUjxRBo/VU3tzUkUdue0LEZQIgeKlO2MKujKToDAn5GWP9RAl5sKiLpX62pTXLpS7fQ41CdyREBU9Jdc2hf0eRTdJsbdm6mL9Z"
                                      },
                                      {
                                          "spent": false,
                                          "unblinded_cred": "hSHfygeq++tl3SgkhygbjyvNTWdnMjJlt51To6KOb19SlgJ5kUMZEvsw6H1/e0MV+KyFBeJTd24ED6kzbac2ugDi8aXnBoucVyeJa3XBtSvqcAJhRu8VABv6IfxW81AT"
                                      }
                                  ],
                                  "valid_from": "2022-06-21T19:31:40",
                                  "valid_to": "2022-06-22T19:31:40"
                              }
                          ]
                      }
                  }
              },
              "orders":
              {
                  "ed5a53c1-9555-4b9c-81df-485521ab8161":
                  {
                      "created_at": "2023-02-16T22:48:02.804478",
                      "currency": "USD",
                      "expires_at": "{year}-09-16T23:56:52.839338",
                      "id": "ed5a53c1-9555-4b9c-81df-485521ab8161",
                      "items":
                      [
                          {
                              "created_at": "2023-02-16T22:48:02.804478",
                              "credential_type": "time-limited-v2",
                              "currency": "USD",
                              "description": "brave-vpn-premium",
                              "id": "93e2e06b-bb69-47e9-8dff-270b53938157",
                              "location": "{domain}",
                              "order_id": "ed5a53c1-9555-4b9c-81df-485521ab8161",
                              "price": 9.99,
                              "quantity": 1,
                              "sku": "brave-vpn-premium",
                              "subtotal": 9.99,
                              "updated_at": "2023-02-16T22:48:02.804478"
                          }
                      ],
                      "last_paid_at": "2023-08-16T23:56:52.839338",
                      "location": "{domain}",
                      "merchant_id": "brave.com",
                      "metadata":
                      {
                          "num_intervals": 33,
                          "num_per_interval": 2,
                          "payment_processor": "stripe",
                          "stripe_checkout_session_id": "cs_live_b1l4e7azojxIWro3UYp3Bx8dO1CMb9IRbAC1x6qaKAtsMH9KPO77quKGoM"
                      },
                      "status": "paid",
                      "total_price": 9.99,
                      "updated_at": "2023-08-16T23:56:52.837510"
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
  const auto* order_value =
      value->GetDict().FindByDottedPath("orders." + order_id);
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
    skus::mojom::SkusResultPtr result;
    bool callback_called = false;
    skus_service_->CredentialSummary(
        domain,
        base::BindLambdaForTesting([&](skus::mojom::SkusResultPtr summary) {
          callback_called = true;
          result = std::move(summary);
        }));
    task_environment_.RunUntilIdle();
    EXPECT_TRUE(callback_called);
    return result->message;
  }

  void Interceptor(const network::ResourceRequest& request) {
    url_loader_factory_.ClearResponses();
    url_loader_factory_.AddResponse(request.url.spec(), "{}");
  }
  PrefService* prefs() { return &prefs_; }

  base::test::TaskEnvironment task_environment_;

 private:
  std::unique_ptr<skus::SkusServiceImpl> skus_service_;
  TestingPrefServiceSimple prefs_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(SkusServiceTestUnitTest, CredentialSummarySuccess) {
  base::Value::Dict state;
  auto env = skus::GetDefaultEnvironment();
  auto domain = skus::GetDomain("vpn", env);
  auto testing_payload = GenerateTestingCreds(domain);
  state.Set("skus:" + env, testing_payload);
  prefs()->SetDict(skus::prefs::kSkusState, std::move(state));
  auto credentials = GetCredentialsSummary(domain);
  EXPECT_FALSE(credentials.empty());
  auto credentials_json = base::JSONReader::Read(credentials);
  auto* order = credentials_json->GetDict().Find("order");
  EXPECT_TRUE(order);
  EXPECT_EQ(*order, GetExpectedCreds(testing_payload,
                                     "ed5a53c1-9555-4b9c-81df-485521ab8161"));
}

TEST_F(SkusServiceTestUnitTest, CredentialSummaryFailed) {
  base::Value::Dict state;
  auto env = skus::GetDefaultEnvironment();
  auto domain = skus::GetDomain("vpn", env);
  auto testing_payload = GenerateTestingCreds(domain);
  auto payload_value = base::JSONReader::Read(testing_payload);
  auto* orders = payload_value->GetDict().FindDict("orders");
  EXPECT_TRUE(orders);
  // Remove unexpired creds
  orders->Remove("ed5a53c1-9555-4b9c-81df-485521ab8161");
  std::string json;
  base::JSONWriter::WriteWithOptions(
      payload_value.value(), base::JSONWriter::OPTIONS_PRETTY_PRINT, &json);
  // Save prefs with expired prefs only
  state.Set("skus:" + env, json);

  prefs()->SetDict(skus::prefs::kSkusState, std::move(state));
  auto credentials = GetCredentialsSummary(domain);
  EXPECT_EQ(credentials, "{}");
}

TEST_F(SkusServiceTestUnitTest, CredentialSummaryWrongEnv) {
  base::Value::Dict state;
  auto testing_payload = GenerateTestingCreds("vpn.brave.software");
  state.Set("skus:staging", testing_payload);
  prefs()->SetDict(skus::prefs::kSkusState, std::move(state));
  auto credentials = GetCredentialsSummary("vpn.brave.software");
  EXPECT_EQ(credentials, "{}");
}
