/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/base64.h"
#include "base/feature_list.h"
#include "base/functional/callback_forward.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service_helper.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_os_connection_api_sim.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/features.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "brave/components/skus/browser/pref_names.h"
#include "brave/components/skus/browser/skus_context_impl.h"
#include "brave/components/skus/browser/skus_service_impl.h"
#include "brave/components/skus/browser/skus_utils.h"
#include "brave/components/skus/common/features.h"
#include "brave/components/skus/common/skus_sdk.mojom.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/browser/browser_context.h"
#include "content/public/test/browser_task_environment.h"
#include "net/base/mock_network_change_notifier.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_vpn {

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
}
          )";

std::string GenerateTestingCreds(const std::string& domain,
                                 bool active_subscription = true) {
  auto value = base::JSONReader::Read(kTestVpnOrders);
  std::string json;
  base::JSONWriter::WriteWithOptions(
      value.value(), base::JSONWriter::OPTIONS_PRETTY_PRINT, &json);

  auto now = base::Time::Now();
  base::Time::Exploded exploded;
  now.LocalExplode(&exploded);
  std::string year =
      base::NumberToString(active_subscription ? exploded.year + 1 : 0);
  base::ReplaceSubstringsAfterOffset(&json, 0, "{year}", year);
  base::ReplaceSubstringsAfterOffset(&json, 0, "{domain}", domain);
  return json;
}

}  // namespace

using ConnectionState = mojom::ConnectionState;
using PurchasedState = mojom::PurchasedState;

class TestBraveVPNServiceObserver : public mojom::ServiceObserver {
 public:
  TestBraveVPNServiceObserver() = default;

  void OnPurchasedStateChanged(PurchasedState state) override {
    purchased_state_ = state;
    if (purchased_callback_)
      std::move(purchased_callback_).Run();
  }
#if !BUILDFLAG(IS_ANDROID)
  void OnConnectionStateChanged(ConnectionState state) override {
    connection_state_ = state;
    if (connection_state_callback_)
      std::move(connection_state_callback_).Run();
  }
  void OnSelectedRegionChanged(mojom::RegionPtr region) override {
    if (selected_region_callback_)
      std::move(selected_region_callback_).Run();
  }
#endif
  void WaitPurchasedStateChange(base::OnceClosure callback) {
    purchased_callback_ = std::move(callback);
  }
#if !BUILDFLAG(IS_ANDROID)
  void WaitConnectionStateChange(base::OnceClosure callback) {
    connection_state_callback_ = std::move(callback);
  }
  void WaitSelectedRegionStateChange(base::OnceClosure callback) {
    selected_region_callback_ = std::move(callback);
  }
#endif
  mojo::PendingRemote<mojom::ServiceObserver> GetReceiver() {
    return observer_receiver_.BindNewPipeAndPassRemote();
  }
  void ResetStates() {
    purchased_state_.reset();
    connection_state_.reset();
  }
  absl::optional<PurchasedState> GetPurchasedState() const {
    return purchased_state_;
  }
  absl::optional<ConnectionState> GetConnectionState() const {
    return connection_state_;
  }

 private:
  absl::optional<PurchasedState> purchased_state_;
  absl::optional<ConnectionState> connection_state_;
  base::OnceClosure purchased_callback_;
  base::OnceClosure connection_state_callback_;
  base::OnceClosure selected_region_callback_;
  mojo::Receiver<mojom::ServiceObserver> observer_receiver_{this};
};

class BraveVPNServiceTest : public testing::Test {
 public:
  BraveVPNServiceTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {
    scoped_feature_list_.InitWithFeatures(
        {skus::features::kSkusFeature, features::kBraveVPN}, {});
  }

  void SetUp() override {
    base::Time future_mock_time;
    if (base::Time::FromString("2023-01-04", &future_mock_time)) {
      task_environment_.AdvanceClock(future_mock_time - base::Time::Now());
    }
    skus::RegisterLocalStatePrefs(local_pref_service_.registry());
    brave_vpn::RegisterLocalStatePrefs(local_pref_service_.registry());
    brave_vpn::RegisterProfilePrefs(profile_pref_service_.registry());
    shared_url_loader_factory_ =
        base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
            &url_loader_factory_);
    url_loader_factory_.SetInterceptor(base::BindRepeating(
        &BraveVPNServiceTest::Interceptor, base::Unretained(this)));
    // Setup required for SKU (dependency of VPN)
    skus_service_ = std::make_unique<skus::SkusServiceImpl>(
        &local_pref_service_, url_loader_factory_.GetSafeWeakWrapper());
    connection_api_ = std::make_unique<BraveVPNOSConnectionAPISim>(
        shared_url_loader_factory_, &local_pref_service_);
    ResetVpnService();
  }

  void TearDown() override {
    if (service_) {
      service_->Shutdown();
    }
  }

  void ResetVpnService() {
    if (service_) {
      service_->Shutdown();
    }
    service_ = std::make_unique<BraveVpnService>(
        connection_api_.get(), url_loader_factory_.GetSafeWeakWrapper(),
        &local_pref_service_, &profile_pref_service_,
        base::BindRepeating(&BraveVPNServiceTest::GetSkusService,
                            base::Unretained(this)));
  }
  mojo::PendingRemote<skus::mojom::SkusService> GetSkusService() {
    if (!skus_service_) {
      return mojo::PendingRemote<skus::mojom::SkusService>();
    }
    return static_cast<skus::SkusServiceImpl*>(skus_service_.get())
        ->MakeRemote();
  }
  void SetInterceptorResponse(const std::string& response) {
    https_response_ = response;
  }
  void Interceptor(const network::ResourceRequest& request) {
    url_loader_factory_.ClearResponses();
    url_loader_factory_.AddResponse(request.url.spec(), https_response_);
  }

  void AddObserver(mojo::PendingRemote<mojom::ServiceObserver> observer) {
    service_->AddObserver(std::move(observer));
  }

  void SetPurchasedState(const std::string& env, PurchasedState state) {
    service_->SetPurchasedState(env, state);
  }
  void LoadPurchasedState(const std::string& domain) {
    service_->LoadPurchasedState(domain);
  }
  PurchasedState GetPurchasedStateSync() const {
    return service_->GetPurchasedStateSync();
  }

#if !BUILDFLAG(IS_ANDROID)
  mojom::Region device_region() const {
    if (auto region_ptr = GetRegionPtrWithNameFromRegionList(
            service_->GetDeviceRegion(), regions())) {
      return *region_ptr;
    }
    return mojom::Region();
  }
  void OnCredentialSummary(const std::string& domain,
                           const std::string& summary) {
    service_->OnCredentialSummary(domain, summary);
  }

  std::vector<mojom::Region>& regions() const { return service_->regions_; }
  void SetSelectedRegion(const std::string& region) {
    service_->SetSelectedRegion(region);
  }

  void OnFetchRegionList(bool background_fetch,
                         const std::string& region_list,
                         bool success) {
    service_->OnFetchRegionList(background_fetch, region_list, success);
  }

  void OnFetchTimezones(const std::string& timezones_list, bool success) {
    service_->OnFetchTimezones(timezones_list, success);
  }
  void LoadCachedRegionData() { service_->LoadCachedRegionData(); }

  BraveVPNOSConnectionAPI* GetBraveVPNConnectionAPI() {
    return service_->GetBraveVPNConnectionAPI();
  }

  void OnGetSubscriberCredentialV12(const std::string& subscriber_credential,
                                    bool success) {
    service_->OnGetSubscriberCredentialV12(base::Time::Now(),
                                           subscriber_credential, success);
  }

  void OnPrepareCredentialsPresentation(
      const std::string& domain,
      const std::string& credential_as_cookie) {
    service_->OnPrepareCredentialsPresentation(domain, credential_as_cookie);
  }

  void SetDeviceRegion(const std::string& name) {
    service_->SetDeviceRegion(name);
  }

  void SetFallbackDeviceRegion() { service_->SetFallbackDeviceRegion(); }

  void SetTestTimezone(const std::string& timezone) {
    service_->test_timezone_ = timezone;
  }

  BraveVPNOSConnectionAPI* GetConnectionAPI() { return connection_api_.get(); }
#endif
  void RecordP3A(bool new_usage) { service_->RecordP3A(new_usage); }

  std::string GetCurrentEnvironment() {
    return service_->GetCurrentEnvironment();
  }

  std::string GetRegionsData() {
    // Give 11 region data.
    return R"([
        {
          "continent": "europe",
          "name": "eu-es",
          "name-pretty": "Spain"
        },
        {
          "continent": "south-america",
          "name": "sa-br",
          "name-pretty": "Brazil"
        },
        {
          "continent": "europe",
          "name": "eu-ch",
          "name-pretty": "Switzerland"
        },
        {
          "continent": "europe",
          "name": "eu-de",
          "name-pretty": "Germany"
        },
        {
          "continent": "asia",
          "name": "asia-sg",
          "name-pretty": "Singapore"
        },
        {
          "continent": "north-america",
          "name": "ca-east",
          "name-pretty": "Canada"
        },
        {
          "continent": "asia",
          "name": "asia-jp",
          "name-pretty": "Japan"
        },
        {
          "continent": "europe",
          "name": "eu-en",
          "name-pretty": "United Kingdom"
        },
        {
          "continent": "europe",
          "name": "eu-nl",
          "name-pretty": "Netherlands"
        },
        {
          "continent": "north-america",
          "name": "us-west",
          "name-pretty": "USA West"
        },
        {
          "continent": "oceania",
          "name": "au-au",
          "name-pretty": "Australia"
        }
      ])";
  }

  std::string GetTimeZonesData() {
    return R"([
        {
          "name": "us-central",
          "timezones": [
            "America/Guatemala",
            "America/Guayaquil",
            "America/Guyana",
            "America/Havana"
          ]
        },
        {
          "name": "eu-es",
          "timezones": [
            "Europe/Madrid",
            "Europe/Gibraltar",
            "Africa/Casablanca",
            "Africa/Algiers"
          ]
        },
        {
          "name": "eu-ch",
          "timezones": [
            "Europe/Zurich"
          ]
        },
        {
          "name": "eu-nl",
          "timezones": [
            "Europe/Amsterdam",
            "Europe/Brussels"
          ]
        },
        {
          "name": "asia-sg",
          "timezones": [
            "Asia/Aden",
            "Asia/Almaty",
            "Asia/Seoul"
          ]
        },
        {
          "name": "asia-jp",
          "timezones": [
            "Pacific/Guam",
            "Pacific/Saipan",
            "Asia/Tokyo"
          ]
        }
      ])";
  }

  void SetValidSubscriberCredential() {
    SetSubscriberCredential(&local_pref_service_, "subscriber_credential",
                            base::Time::Now() + base::Seconds(10));
  }

  void SetInvalidSubscriberCredential() {
    // Set expired date.
    SetSubscriberCredential(&local_pref_service_, "subscriber_credential",
                            base::Time::Now() - base::Seconds(10));
  }

  std::string SetupTestingStoreForEnv(const std::string& env,
                                      bool active_subscription = true) {
    std::string domain = skus::GetDomain("vpn", env);
    auto testing_payload = GenerateTestingCreds(domain, active_subscription);
    base::Value state(base::Value::Type::DICT);
    state.SetStringKey("skus:" + env, testing_payload);
    local_pref_service_.Set(skus::prefs::kSkusState, std::move(state));
    SetInterceptorResponse(GetRegionsData());
    return domain;
  }

  void SetAndExpectPurchasedStateChange(TestBraveVPNServiceObserver* observer,
                                        const std::string& env,
                                        PurchasedState state) {
    observer->ResetStates();
    SetPurchasedState(env, state);
    base::RunLoop().RunUntilIdle();
    EXPECT_TRUE(observer->GetPurchasedState().has_value());
    EXPECT_EQ(observer->GetPurchasedState().value(), state);
  }

  std::string https_response_;
  std::unique_ptr<BraveVPNOSConnectionAPISim> connection_api_;
  base::test::ScopedFeatureList scoped_feature_list_;
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_pref_service_;
  sync_preferences::TestingPrefServiceSyncable profile_pref_service_;
  std::unique_ptr<skus::SkusServiceImpl> skus_service_;
  std::unique_ptr<BraveVpnService> service_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  base::HistogramTester histogram_tester_;
};

TEST(BraveVPNFeatureTest, FeatureTest) {
#if BUILDFLAG(IS_ANDROID) || BUILDFLAG(IS_IOS)
  EXPECT_TRUE(IsBraveVPNEnabled());
#else
  EXPECT_FALSE(IsBraveVPNEnabled());
#endif
}

TEST_F(BraveVPNServiceTest, ResponseSanitizingTest) {
  // Give invalid json data as a server response and check sanitized(empty
  // string) result is returned.
  SetInterceptorResponse("{'a':'b',}");
  base::RunLoop loop;
  service_->GetAllServerRegions(base::BindOnce(
      [](base::OnceClosure callback, const std::string& region_list,
         bool success) {
        EXPECT_TRUE(region_list.empty());
        std::move(callback).Run();
      },
      loop.QuitClosure()));
  loop.Run();
}

#if !BUILDFLAG(IS_ANDROID)
TEST_F(BraveVPNServiceTest, RegionDataTest) {
  // Test invalid region data.
  OnFetchRegionList(false, std::string(), true);
  EXPECT_TRUE(regions().empty());

  // Test valid region data parsing.
  OnFetchRegionList(false, GetRegionsData(), true);
  const size_t kRegionCount = 11;
  EXPECT_EQ(kRegionCount, regions().size());

  // First region in region list is set as a device region when fetch is failed.
  OnFetchTimezones(std::string(), false);
  EXPECT_EQ(regions()[0], device_region());

  // Test fallback region is replaced with proper device region
  // when valid timezone is used.
  // "asia-sg" region is used for "Asia/Seoul" tz.
  SetFallbackDeviceRegion();
  SetTestTimezone("Asia/Seoul");
  OnFetchTimezones(GetTimeZonesData(), true);
  EXPECT_EQ("asia-sg", device_region().name);

  // Test device region is not changed when invalid timezone is set.
  SetFallbackDeviceRegion();
  SetTestTimezone("Invalid");
  OnFetchTimezones(GetTimeZonesData(), true);
  EXPECT_EQ(regions()[0], device_region());

  // Test device region is not changed when invalid timezone is set.
  SetFallbackDeviceRegion();
  SetTestTimezone("Invalid");
  OnFetchTimezones(GetTimeZonesData(), true);
  EXPECT_EQ(regions()[0], device_region());
}

TEST_F(BraveVPNServiceTest, LoadPurchasedStateTest) {
  std::string env = skus::GetDefaultEnvironment();
  std::string domain = skus::GetDomain("vpn", env);
  // Service try loading
  SetPurchasedState(env, PurchasedState::LOADING);
  // Treat not purchased When empty credential string received.
  OnCredentialSummary(domain, "");
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedStateSync());

  // Treat expired when credential with non active received.
  SetPurchasedState(env, PurchasedState::LOADING);
  OnCredentialSummary(domain, R"({ "active": false } )");
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedStateSync());

  // Treat failed when invalid string received.
  SetPurchasedState(env, PurchasedState::LOADING);
  OnCredentialSummary(domain, R"( "invalid" )");
  EXPECT_EQ(PurchasedState::FAILED, GetPurchasedStateSync());

  // Reached to purchased state when valid credential, region data
  // and timezone info.
  SetPurchasedState(env, PurchasedState::LOADING);
  OnCredentialSummary(
      domain, R"({ "active": true, "remaining_credential_count": 1 } )");
  EXPECT_TRUE(regions().empty());
  EXPECT_EQ(PurchasedState::LOADING, GetPurchasedStateSync());
  OnPrepareCredentialsPresentation(
      domain, "credential=abcdefghijk; Expires=Wed, 21 Oct 2050 07:28:00 GMT");
  EXPECT_EQ(PurchasedState::LOADING, GetPurchasedStateSync());
  OnFetchRegionList(false, GetRegionsData(), true);
  EXPECT_EQ(PurchasedState::LOADING, GetPurchasedStateSync());
  SetTestTimezone("Asia/Seoul");
  OnFetchTimezones(GetTimeZonesData(), true);
  EXPECT_EQ(PurchasedState::PURCHASED, GetPurchasedStateSync());

  // Check purchased is set when fetching timezone is failed.
  SetPurchasedState(env, PurchasedState::LOADING);
  OnFetchTimezones("", false);
  EXPECT_EQ(PurchasedState::PURCHASED, GetPurchasedStateSync());

  // Treat not purchased when empty.
  SetPurchasedState(env, PurchasedState::LOADING);
  OnPrepareCredentialsPresentation(
      domain, "credential=; Expires=Wed, 21 Oct 2050 07:28:00 GMT");
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedStateSync());

  // Treat failed when invalid.
  SetPurchasedState(env, PurchasedState::LOADING);
  OnPrepareCredentialsPresentation(domain, "");
  EXPECT_EQ(PurchasedState::FAILED, GetPurchasedStateSync());

  // Treat failed when cookie doesn't have expired date.
  SetPurchasedState(env, PurchasedState::LOADING);
  OnPrepareCredentialsPresentation(domain, "credential=abcdefghijk");
  EXPECT_EQ(PurchasedState::FAILED, GetPurchasedStateSync());
}

TEST_F(BraveVPNServiceTest, ConnectionStateUpdateWithPurchasedStateTest) {
  // Prepare valid connection info.
  auto* test_api = static_cast<BraveVPNOSConnectionAPISim*>(GetConnectionAPI());

  TestBraveVPNServiceObserver observer;
  AddObserver(observer.GetReceiver());
  std::string env = skus::GetDefaultEnvironment();
  SetPurchasedState(env, PurchasedState::PURCHASED);
  test_api->SetConnectionState(ConnectionState::CONNECTING);
  base::RunLoop().RunUntilIdle();
  test_api->SetConnectionState(ConnectionState::CONNECTED);
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(ConnectionState::CONNECTED, observer.GetConnectionState());
}

TEST_F(BraveVPNServiceTest, SelectedRegionChangedUpdateTest) {
  TestBraveVPNServiceObserver observer;
  AddObserver(observer.GetReceiver());

  OnFetchRegionList(false, GetRegionsData(), true);
  SetSelectedRegion("asia-sg");
  base::RunLoop loop;
  observer.WaitSelectedRegionStateChange(loop.QuitClosure());
  loop.Run();
}

// Check SetSelectedRegion is called when default device region is set.
// We use default device region as an initial selected region.
TEST_F(BraveVPNServiceTest, SelectedRegionChangedUpdateWithDeviceRegionTest) {
  TestBraveVPNServiceObserver observer;
  AddObserver(observer.GetReceiver());

  OnFetchRegionList(false, GetRegionsData(), true);
  SetTestTimezone("Asia/Seoul");
  OnFetchTimezones(GetTimeZonesData(), true);
  base::RunLoop loop;
  observer.WaitSelectedRegionStateChange(loop.QuitClosure());
  loop.Run();
}

TEST_F(BraveVPNServiceTest, LoadRegionDataFromPrefsTest) {
  std::string env = skus::GetDefaultEnvironment();
  // Initially, prefs doesn't have region data.
  EXPECT_EQ(mojom::Region(), device_region());
  EXPECT_TRUE(regions().empty());

  // Set proper data to store them in prefs.
  OnFetchRegionList(false, GetRegionsData(), true);
  SetTestTimezone("Asia/Seoul");
  OnFetchTimezones(GetTimeZonesData(), true);

  // Check region data is set with above data.
  EXPECT_FALSE(mojom::Region() == device_region());
  EXPECT_FALSE(regions().empty());

  // Clear region data.
  regions().clear();
  EXPECT_TRUE(regions().empty());

  // Check region data is loaded from prefs.
  SetPurchasedState(env, PurchasedState::LOADING);
  LoadCachedRegionData();
  EXPECT_FALSE(regions().empty());
}

TEST_F(BraveVPNServiceTest, LoadPurchasedStateForAnotherEnvFailed) {
  auto development = SetupTestingStoreForEnv(skus::GetDefaultEnvironment());
  EXPECT_EQ(skus::GetEnvironmentForDomain(development),
            skus::GetDefaultEnvironment());
  TestBraveVPNServiceObserver observer;
  AddObserver(observer.GetReceiver());
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedStateSync());
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_FALSE(observer.GetPurchasedState().has_value());

  LoadPurchasedState(development);
  base::RunLoop().RunUntilIdle();
  // Successfully set purchased state for dev env.
  EXPECT_TRUE(observer.GetPurchasedState().has_value());
  EXPECT_EQ(observer.GetPurchasedState().value(), PurchasedState::PURCHASED);
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_EQ(GetPurchasedStateSync(), PurchasedState::PURCHASED);

  observer.ResetStates();
  SetInterceptorResponse("");
  std::string staging = skus::GetDomain("vpn", skus::kEnvStaging);
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  // no order found for staging.
  LoadPurchasedState(staging);
  base::RunLoop().RunUntilIdle();
  // The purchased state was not changed from dev env.
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_EQ(GetPurchasedStateSync(), PurchasedState::PURCHASED);

  observer.ResetStates();
  staging = SetupTestingStoreForEnv(skus::kEnvStaging, false);
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  // No region data for staging.
  SetInterceptorResponse("");
  LoadPurchasedState(staging);
  base::RunLoop().RunUntilIdle();
  // The purchased state was not changed from dev env.
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_EQ(GetPurchasedStateSync(), PurchasedState::PURCHASED);

  observer.ResetStates();
  staging = SetupTestingStoreForEnv(skus::kEnvStaging, false);
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  // Inactive staging subscription.
  OnCredentialSummary(staging, R"({ "active": false } )");
  // The purchased state was not changed from dev env.
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_EQ(GetPurchasedStateSync(), PurchasedState::PURCHASED);

  observer.ResetStates();
  staging = SetupTestingStoreForEnv(skus::kEnvStaging, false);
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  // Invalid staging subscription.
  OnCredentialSummary(staging, R"([])");
  // The purchased state was not changed from dev env.
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_EQ(GetPurchasedStateSync(), PurchasedState::PURCHASED);
}

TEST_F(BraveVPNServiceTest, CheckInitialPurchasedStateTest) {
  // Purchased state is not checked for fresh user.
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedStateSync());

  // Set valid subscriber credential to pretend it's purchased user.
  SetValidSubscriberCredential();
  ResetVpnService();
  EXPECT_EQ(PurchasedState::LOADING, GetPurchasedStateSync());

  // Set in-valid subscriber credential but not empty to pretend it's purchased
  // user but expired while browser is terminated.
  // In this case, service should try to reload purchased state at startup.
  SetInvalidSubscriberCredential();
  ResetVpnService();
  EXPECT_EQ(PurchasedState::LOADING, GetPurchasedStateSync());
}

TEST_F(BraveVPNServiceTest, SubscribedCredentials) {
  std::string env = skus::GetDefaultEnvironment();
  SetPurchasedState(env, PurchasedState::PURCHASED);
  EXPECT_EQ(PurchasedState::PURCHASED, GetPurchasedStateSync());
  OnGetSubscriberCredentialV12("Token No Longer Valid", false);
  EXPECT_EQ(PurchasedState::EXPIRED, GetPurchasedStateSync());
}

// Test connection check is asked only when purchased state.
TEST_F(BraveVPNServiceTest, CheckConnectionStateAfterPurchased) {
  std::string env = skus::GetDefaultEnvironment();
  auto* test_api = static_cast<BraveVPNOSConnectionAPISim*>(GetConnectionAPI());
  EXPECT_FALSE(test_api->IsConnectionChecked());
  SetPurchasedState(env, PurchasedState::NOT_PURCHASED);
  EXPECT_FALSE(test_api->IsConnectionChecked());
  SetPurchasedState(env, PurchasedState::PURCHASED);
  EXPECT_TRUE(test_api->IsConnectionChecked());
}

#endif

TEST_F(BraveVPNServiceTest, GetPurchasedStateSync) {
  std::string env = skus::GetDefaultEnvironment();
  EXPECT_EQ(mojom::PurchasedState::NOT_PURCHASED, GetPurchasedStateSync());

  SetPurchasedState(env, PurchasedState::LOADING);
  EXPECT_EQ(PurchasedState::LOADING, GetPurchasedStateSync());

  SetPurchasedState(env, PurchasedState::PURCHASED);
  EXPECT_EQ(PurchasedState::PURCHASED, GetPurchasedStateSync());

  SetPurchasedState(env, PurchasedState::EXPIRED);
  EXPECT_EQ(PurchasedState::EXPIRED, GetPurchasedStateSync());

  SetPurchasedState(env, PurchasedState::FAILED);
  EXPECT_EQ(PurchasedState::FAILED, GetPurchasedStateSync());

  SetPurchasedState(env, PurchasedState::NOT_PURCHASED);
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedStateSync());
}

TEST_F(BraveVPNServiceTest, SetPurchasedState) {
  std::string env = skus::GetDefaultEnvironment();
  TestBraveVPNServiceObserver observer;
  AddObserver(observer.GetReceiver());
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedStateSync());

  SetAndExpectPurchasedStateChange(&observer, env, PurchasedState::LOADING);
  SetAndExpectPurchasedStateChange(&observer, env, PurchasedState::EXPIRED);
  SetAndExpectPurchasedStateChange(&observer, env, PurchasedState::FAILED);
  SetAndExpectPurchasedStateChange(&observer, env,
                                   PurchasedState::NOT_PURCHASED);
  SetAndExpectPurchasedStateChange(&observer, env, PurchasedState::PURCHASED);

  SetPurchasedState(env, PurchasedState::PURCHASED);
  base::RunLoop().RunUntilIdle();
  observer.ResetStates();
  // Do not notify if status is not changed.
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  SetPurchasedState(env, PurchasedState::PURCHASED);
  base::RunLoop().RunUntilIdle();
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
}

TEST_F(BraveVPNServiceTest, LoadPurchasedStateNotifications) {
  std::string env = skus::GetDefaultEnvironment();
  std::string domain = skus::GetDomain("vpn", env);
  TestBraveVPNServiceObserver observer;
  AddObserver(observer.GetReceiver());
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedStateSync());

  LoadPurchasedState(domain);
  {
    // Loading state called if we fetch it first time.
    base::RunLoop loop;
    observer.WaitPurchasedStateChange(loop.QuitClosure());
    loop.Run();
    EXPECT_EQ(PurchasedState::LOADING, GetPurchasedStateSync());
    EXPECT_TRUE(observer.GetPurchasedState().has_value());
    EXPECT_EQ(PurchasedState::LOADING, observer.GetPurchasedState().value());
  }
  {
    base::RunLoop loop;
    observer.WaitPurchasedStateChange(loop.QuitClosure());
    loop.Run();
    EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedStateSync());
    EXPECT_TRUE(observer.GetPurchasedState().has_value());
    EXPECT_EQ(PurchasedState::NOT_PURCHASED,
              observer.GetPurchasedState().value());
  }
  observer.ResetStates();
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedStateSync());
  LoadPurchasedState(domain);
  base::RunLoop().RunUntilIdle();
  // Observer event not called second time because the state is not changed.
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  // Observer called when state will be changed.
  SetAndExpectPurchasedStateChange(&observer, env, PurchasedState::PURCHASED);
}

TEST_F(BraveVPNServiceTest, LoadPurchasedStateForAnotherEnv) {
  auto development = SetupTestingStoreForEnv(skus::GetDefaultEnvironment());
  EXPECT_EQ(skus::GetEnvironmentForDomain(development),
            skus::GetDefaultEnvironment());
  TestBraveVPNServiceObserver observer;
  AddObserver(observer.GetReceiver());
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedStateSync());
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  LoadPurchasedState(development);
  base::RunLoop().RunUntilIdle();
  // Successfully set purchased state for dev env.
  EXPECT_TRUE(observer.GetPurchasedState().has_value());
  EXPECT_EQ(observer.GetPurchasedState().value(), PurchasedState::PURCHASED);
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());

  observer.ResetStates();
  auto staging = SetupTestingStoreForEnv(skus::kEnvStaging);
  EXPECT_EQ(skus::GetEnvironmentForDomain(staging), skus::kEnvStaging);
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  LoadPurchasedState(staging);
  base::RunLoop().RunUntilIdle();
  // Successfully changed purchased state for dev env.
  EXPECT_TRUE(observer.GetPurchasedState().has_value());
  EXPECT_EQ(observer.GetPurchasedState().value(), PurchasedState::PURCHASED);
  EXPECT_EQ(GetCurrentEnvironment(), skus::kEnvStaging);
}

TEST_F(BraveVPNServiceTest, NewUserReturningMetric) {
  RecordP3A(false);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 0, 2);

  task_environment_.FastForwardBy(base::Days(1));
  RecordP3A(true);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 2, 1);

  task_environment_.FastForwardBy(base::Days(1));
  RecordP3A(true);
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 3, 1);

  task_environment_.FastForwardBy(base::Days(6));
  histogram_tester_.ExpectBucketCount(kNewUserReturningHistogramName, 1, 1);
}

TEST_F(BraveVPNServiceTest, DaysInMonthUsedMetric) {
  RecordP3A(false);
  histogram_tester_.ExpectTotalCount(kDaysInMonthUsedHistogramName, 0);

  RecordP3A(true);
  histogram_tester_.ExpectBucketCount(kDaysInMonthUsedHistogramName, 1, 1);

  task_environment_.FastForwardBy(base::Days(1));
  RecordP3A(true);
  histogram_tester_.ExpectBucketCount(kDaysInMonthUsedHistogramName, 2, 1);
  task_environment_.FastForwardBy(base::Days(1));
  histogram_tester_.ExpectBucketCount(kDaysInMonthUsedHistogramName, 2, 2);

  RecordP3A(true);
  task_environment_.FastForwardBy(base::Days(30));
  histogram_tester_.ExpectBucketCount(kDaysInMonthUsedHistogramName, 0, 1);
}

TEST_F(BraveVPNServiceTest, LastUsageTimeMetric) {
  histogram_tester_.ExpectTotalCount(kLastUsageTimeHistogramName, 0);

  RecordP3A(true);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 1, 1);

  task_environment_.AdvanceClock(base::Days(10));
  RecordP3A(true);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 1, 2);
  task_environment_.AdvanceClock(base::Days(10));
  RecordP3A(false);
  histogram_tester_.ExpectBucketCount(kLastUsageTimeHistogramName, 2, 1);
}

}  // namespace brave_vpn
