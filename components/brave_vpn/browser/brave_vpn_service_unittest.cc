/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/brave_vpn_service.h"

#include <memory>
#include <optional>
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
#include "brave/components/brave_vpn/browser/api/brave_vpn_api_request.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service_helper.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_manager.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_region_data_helper.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/features.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
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

#if !BUILDFLAG(IS_ANDROID)
#include "brave/components/brave_vpn/browser/connection/ikev2/connection_api_impl_sim.h"
#endif

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
  // Give sufficient additional years to prevent it expire.
  std::string year =
      base::NumberToString(active_subscription ? exploded.year + 10 : 0);
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

  void OnPurchasedStateChanged(
      PurchasedState state,
      const std::optional<std::string>& description) override {
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
  std::optional<PurchasedState> GetPurchasedState() const {
    return purchased_state_;
  }
  std::optional<ConnectionState> GetConnectionState() const {
    return connection_state_;
  }

 private:
  std::optional<PurchasedState> purchased_state_;
  std::optional<ConnectionState> connection_state_;
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
#if !BUILDFLAG(IS_ANDROID)
    connection_manager_ = std::make_unique<BraveVPNConnectionManager>(
        shared_url_loader_factory_, &local_pref_service_, base::NullCallback());
    connection_manager_->SetConnectionAPIImplForTesting(
        std::make_unique<ConnectionAPIImplSim>(connection_manager_.get(),
                                               shared_url_loader_factory_));
#endif
    ResetVpnService();
  }

  void TearDown() override {
    if (service_) {
      service_->Shutdown();
    }
    skus_service_.reset();
#if !BUILDFLAG(IS_ANDROID)
    connection_manager_.reset();
#endif
  }

  void ResetVpnService() {
    if (service_) {
      service_->Shutdown();
    }
    service_ = std::make_unique<BraveVpnService>(
#if !BUILDFLAG(IS_ANDROID)
        connection_manager_.get(),
#else
        nullptr,
#endif
        url_loader_factory_.GetSafeWeakWrapper(), &local_pref_service_,
        &profile_pref_service_,
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

  PurchasedState GetPurchasedInfoSync() const {
    return service_->GetPurchasedInfoSync().state;
  }

  void BlockVPNByPolicy(bool value) {
    profile_pref_service_.SetManagedPref(prefs::kManagedBraveVPNDisabled,
                                         base::Value(value));
    EXPECT_EQ(brave_vpn::IsBraveVPNDisabledByPolicy(&profile_pref_service_),
              value);
  }

#if !BUILDFLAG(IS_ANDROID)
  bool& wait_region_data_ready() { return service_->wait_region_data_ready_; }

  skus::mojom::SkusResultPtr MakeSkusResult(const std::string& result) {
    return skus::mojom::SkusResult::New(skus::mojom::SkusResultCode::Ok,
                                        result);
  }

  void OnCredentialSummary(const std::string& domain,
                           const std::string& summary) {
    service_->OnCredentialSummary(domain, MakeSkusResult(summary));
  }

  const std::vector<mojom::RegionPtr>& regions() const {
    return GetBraveVPNConnectionManager()->GetRegionDataManager().GetRegions();
  }

  void SetSelectedRegion(const std::string& region) {
    GetBraveVPNConnectionManager()->SetSelectedRegion(region);
  }

  void OnFetchRegionList(const std::string& region_list, bool success) {
    GetBraveVPNConnectionManager()->GetRegionDataManager().OnFetchRegionList(
        region_list, success);
  }

  void OnFetchTimezones(const std::string& timezones_list, bool success) {
    GetBraveVPNConnectionManager()->GetRegionDataManager().OnFetchTimezones(
        timezones_list, success);
  }

  BraveVPNConnectionManager* GetBraveVPNConnectionManager() const {
    return service_->connection_manager_;
  }

  void OnGetSubscriberCredentialV12(const std::string& subscriber_credential,
                                    bool success) {
    service_->OnGetSubscriberCredentialV12(base::Time::Now(),
                                           subscriber_credential, success);
  }

  void OnPrepareCredentialsPresentation(
      const std::string& domain,
      const std::string& credential_as_cookie) {
    service_->OnPrepareCredentialsPresentation(
        domain, MakeSkusResult(credential_as_cookie));
  }

  void SetDeviceRegion(const std::string& name) {
    GetBraveVPNConnectionManager()->GetRegionDataManager().SetDeviceRegion(
        name);
  }

  void SetFallbackDeviceRegion() {
    GetBraveVPNConnectionManager()
        ->GetRegionDataManager()
        .SetFallbackDeviceRegion();
  }

  void SetTestTimezone(const std::string& timezone) {
    GetBraveVPNConnectionManager()->GetRegionDataManager().test_timezone_ =
        timezone;
  }

  void SetConnectionStateForTesting(ConnectionState state) {
    GetConnectionAPIImpl()->SetConnectionStateForTesting(state);
  }

  ConnectionState GetConnectionState() {
    return GetConnectionAPIImpl()->GetConnectionState();
  }

  ConnectionAPIImplSim* GetConnectionAPIImpl() {
    return static_cast<ConnectionAPIImplSim*>(
        connection_manager_->connection_api_impl_.get());
  }
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
    base::Value::Dict state;
    state.Set("skus:" + env, testing_payload);
    local_pref_service_.SetDict(skus::prefs::kSkusState, std::move(state));
    SetInterceptorResponse(GetRegionsData());
    return domain;
  }

  void SetAndExpectPurchasedStateChange(TestBraveVPNServiceObserver* observer,
                                        const std::string& env,
                                        PurchasedState state) {
    observer->ResetStates();
    SetPurchasedState(env, state);
    task_environment_.RunUntilIdle();
    EXPECT_TRUE(observer->GetPurchasedState().has_value());
    EXPECT_EQ(observer->GetPurchasedState().value(), state);
  }

  void GetAllRegions(BraveVpnService::ResponseCallback callback) {
    service_->api_request_->GetServerRegions(
        std::move(callback), brave_vpn::mojom::kRegionPrecisionCityByCountry);
  }

#if !BUILDFLAG(IS_ANDROID)
  std::unique_ptr<BraveVPNConnectionManager> connection_manager_;
#endif
  std::string https_response_;
  base::test::ScopedFeatureList scoped_feature_list_;
  content::BrowserTaskEnvironment task_environment_;
  TestingPrefServiceSimple local_pref_service_;
  sync_preferences::TestingPrefServiceSyncable profile_pref_service_;
  std::unique_ptr<skus::SkusServiceImpl> skus_service_;
  std::unique_ptr<BraveVpnService> service_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  base::HistogramTester histogram_tester_;
};

TEST_F(BraveVPNServiceTest, ResponseSanitizingTest) {
  // Give invalid json data as a server response and check sanitized(empty
  // string) result is returned.
  SetInterceptorResponse("{'invalid json':");
  base::RunLoop loop;
  GetAllRegions(base::BindOnce(
      [](base::OnceClosure callback, const std::string& region_list,
         bool success) {
        EXPECT_TRUE(region_list.empty());
        std::move(callback).Run();
      },
      loop.QuitClosure()));
  loop.Run();
}

#if !BUILDFLAG(IS_ANDROID)
TEST_F(BraveVPNServiceTest, SkusCredentialCacheTest) {
  std::string env = skus::GetDefaultEnvironment();
  std::string domain = skus::GetDomain("vpn", env);

  SetPurchasedState(env, PurchasedState::LOADING);
  OnCredentialSummary(
      domain, R"({ "active": true, "remaining_credential_count": 1 } )");
  EXPECT_EQ(PurchasedState::LOADING, GetPurchasedInfoSync());
  OnPrepareCredentialsPresentation(
      domain, "credential=abcdefghijk; Expires=Wed, 21 Oct 2050 07:28:00 GMT");
  EXPECT_TRUE(HasValidSkusCredential(&local_pref_service_));
  OnGetSubscriberCredentialV12("invalid", false);
  EXPECT_EQ(PurchasedState::FAILED, GetPurchasedInfoSync());

  // Trying again with valid subscriber credential.
  // Check cached skus credential is gone and we have valid subs credential
  // instead.
  SetPurchasedState(env, PurchasedState::LOADING);
  OnGetSubscriberCredentialV12("valid-subs-credentials", true);
  EXPECT_FALSE(HasValidSkusCredential(&local_pref_service_));
  EXPECT_TRUE(HasValidSubscriberCredential(&local_pref_service_));
}

TEST_F(BraveVPNServiceTest, LoadPurchasedStateSessionExpiredTest) {
  std::string env = skus::GetDefaultEnvironment();
  std::string domain = skus::GetDomain("vpn", env);

  // Treat as not purchased when active is false but there is remained
  // credentials. In this situation, user should activate vpn account.
  SetPurchasedState(env, PurchasedState::LOADING);
  OnCredentialSummary(
      domain, R"({ "active": false, "remaining_credential_count": 1 } )");
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedInfoSync());

  // Treat as not purchased.
  // Although zero credential credential string received, we think it's
  // fresh user as there is no cached data such as regions list.
  SetPurchasedState(env, PurchasedState::LOADING);
  OnCredentialSummary(
      domain, R"({ "active": false, "remaining_credential_count": 0 } )");
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedInfoSync());
  auto session_expired_time =
      local_pref_service_.GetTime(prefs::kBraveVPNSessionExpiredDate);
  EXPECT_TRUE(session_expired_time.is_null());

  // Session expired state for non-fresh user.
  OnFetchRegionList(GetRegionsData(), true);
  OnCredentialSummary(
      domain, R"({ "active": false, "remaining_credential_count": 0 } )");
  EXPECT_EQ(PurchasedState::SESSION_EXPIRED, GetPurchasedInfoSync());
  // Check session expired start date is set.
  session_expired_time =
      local_pref_service_.GetTime(prefs::kBraveVPNSessionExpiredDate);
  EXPECT_FALSE(session_expired_time.is_null());

  // Check not purchased state is set after 30 days passed since session expired
  // starts.
  SetPurchasedState(env, PurchasedState::LOADING);
  local_pref_service_.SetTime(prefs::kBraveVPNSessionExpiredDate,
                              session_expired_time - base::Days(31));
  OnCredentialSummary(
      domain, R"({ "active": false, "remaining_credential_count": 0 } )");
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedInfoSync());

  // Checks cached time data is cleared when received vaild credential.
  SetPurchasedState(env, PurchasedState::LOADING);
  OnCredentialSummary(
      domain, R"({ "active": true, "remaining_credential_count": 1 } )");
  session_expired_time =
      local_pref_service_.GetTime(prefs::kBraveVPNSessionExpiredDate);
  EXPECT_TRUE(session_expired_time.is_null());
}

TEST_F(BraveVPNServiceTest, LoadPurchasedStateTest) {
  std::string env = skus::GetDefaultEnvironment();
  std::string domain = skus::GetDomain("vpn", env);
  // Service try loading
  SetPurchasedState(env, PurchasedState::LOADING);
  // Treat not purchased When empty credential string received.
  OnCredentialSummary(domain, "");
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedInfoSync());

  // Treat expired when credential with non active received.
  SetPurchasedState(env, PurchasedState::LOADING);
  OnCredentialSummary(domain, R"({ "active": false } )");
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedInfoSync());

  // Treat failed when invalid string received.
  SetPurchasedState(env, PurchasedState::LOADING);
  OnCredentialSummary(domain, R"( "invalid" )");
  EXPECT_EQ(PurchasedState::FAILED, GetPurchasedInfoSync());

  // Reached to purchased state when valid credential, region data
  // and timezone info.
  SetPurchasedState(env, PurchasedState::LOADING);
  // We can set purchased state after getting region data.
  wait_region_data_ready() = true;
  OnCredentialSummary(
      domain, R"({ "active": true, "remaining_credential_count": 1 } )");
  EXPECT_TRUE(regions().empty());
  EXPECT_EQ(PurchasedState::LOADING, GetPurchasedInfoSync());
  OnPrepareCredentialsPresentation(
      domain, "credential=abcdefghijk; Expires=Wed, 21 Oct 2050 07:28:00 GMT");

  EXPECT_EQ(PurchasedState::LOADING, GetPurchasedInfoSync());
  OnFetchRegionList(GetRegionsData(), true);
  EXPECT_EQ(PurchasedState::LOADING, GetPurchasedInfoSync());
  SetTestTimezone("Asia/Seoul");
  OnFetchTimezones(GetTimeZonesData(), true);
  EXPECT_FALSE(wait_region_data_ready());
  EXPECT_EQ(PurchasedState::PURCHASED, GetPurchasedInfoSync());

  // Check purchased is set when fetching timezone is failed.
  SetPurchasedState(env, PurchasedState::LOADING);
  wait_region_data_ready() = true;
  OnFetchTimezones("", false);

  EXPECT_FALSE(wait_region_data_ready());
  EXPECT_EQ(PurchasedState::PURCHASED, GetPurchasedInfoSync());

  // Treat not purchased when empty.
  SetPurchasedState(env, PurchasedState::LOADING);
  OnPrepareCredentialsPresentation(
      domain, "credential=; Expires=Wed, 21 Oct 2050 07:28:00 GMT");
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedInfoSync());

  // Treat failed when invalid.
  SetPurchasedState(env, PurchasedState::LOADING);
  OnPrepareCredentialsPresentation(domain, "");
  EXPECT_EQ(PurchasedState::FAILED, GetPurchasedInfoSync());

  // Treat failed when cookie doesn't have expired date.
  SetPurchasedState(env, PurchasedState::LOADING);
  OnPrepareCredentialsPresentation(domain, "credential=abcdefghijk");
  EXPECT_EQ(PurchasedState::FAILED, GetPurchasedInfoSync());

  // Expired credentials.
  SetPurchasedState(env, PurchasedState::LOADING);
  OnPrepareCredentialsPresentation(
      domain, "credential=abcdefghijk; Expires=Wed, 21 Oct 2000 07:28:00 GMT");
  EXPECT_EQ(GetPurchasedInfoSync(), PurchasedState::FAILED);
}

TEST_F(BraveVPNServiceTest, ResetConnectionStateTest) {
  // Set failed state before setting observer.
  SetConnectionStateForTesting(ConnectionState::CONNECT_FAILED);

  TestBraveVPNServiceObserver observer;
  AddObserver(observer.GetReceiver());
  std::string env = skus::GetDefaultEnvironment();
  SetPurchasedState(env, PurchasedState::PURCHASED);

  service_->ResetConnectionState();

  base::RunLoop loop;
  observer.WaitConnectionStateChange(loop.QuitClosure());
  loop.Run();

  // Check state is changed to disconnected after reset connection state.
  EXPECT_EQ(ConnectionState::DISCONNECTED, GetConnectionState());
  EXPECT_EQ(ConnectionState::DISCONNECTED, observer.GetConnectionState());
}

TEST_F(BraveVPNServiceTest, ConnectionStateUpdateWithPurchasedStateTest) {
  TestBraveVPNServiceObserver observer;
  AddObserver(observer.GetReceiver());
  std::string env = skus::GetDefaultEnvironment();
  SetPurchasedState(env, PurchasedState::PURCHASED);
  SetConnectionStateForTesting(ConnectionState::CONNECTING);
  task_environment_.RunUntilIdle();
  SetConnectionStateForTesting(ConnectionState::CONNECTED);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(ConnectionState::CONNECTED, observer.GetConnectionState());
}

TEST_F(BraveVPNServiceTest, IsConnectedWithPurchasedStateTest) {
  TestBraveVPNServiceObserver observer;
  AddObserver(observer.GetReceiver());
  std::string env = skus::GetDefaultEnvironment();
  SetPurchasedState(env, PurchasedState::PURCHASED);

  // Prepare connected state.
  SetConnectionStateForTesting(ConnectionState::CONNECTING);
  task_environment_.RunUntilIdle();
  SetConnectionStateForTesting(ConnectionState::CONNECTED);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(ConnectionState::CONNECTED, observer.GetConnectionState());
  // Gets connected for purchased user.
  EXPECT_TRUE(service_->IsConnected());

  // Check BraveVpnService gives false for IsConnected() when
  // it's connected state but not purchased.
  SetPurchasedState(env, PurchasedState::NOT_PURCHASED);
  EXPECT_EQ(ConnectionState::CONNECTED, observer.GetConnectionState());
  EXPECT_FALSE(service_->IsConnected());
}

TEST_F(BraveVPNServiceTest, DisconnectedIfDisabledByPolicy) {
  TestBraveVPNServiceObserver observer;
  AddObserver(observer.GetReceiver());
  std::string env = skus::GetDefaultEnvironment();
  SetPurchasedState(env, PurchasedState::PURCHASED);
  SetConnectionStateForTesting(ConnectionState::CONNECTED);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(ConnectionState::CONNECTED, observer.GetConnectionState());
  BlockVPNByPolicy(true);
  task_environment_.RunUntilIdle();
  EXPECT_EQ(ConnectionState::DISCONNECTED, observer.GetConnectionState());
}

TEST_F(BraveVPNServiceTest, SelectedRegionChangedUpdateTest) {
  TestBraveVPNServiceObserver observer;
  AddObserver(observer.GetReceiver());

  OnFetchRegionList(GetRegionsData(), true);
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

  OnFetchRegionList(GetRegionsData(), true);
  SetTestTimezone("Asia/Seoul");
  OnFetchTimezones(GetTimeZonesData(), true);
  base::RunLoop loop;
  observer.WaitSelectedRegionStateChange(loop.QuitClosure());
  loop.Run();
}

TEST_F(BraveVPNServiceTest, LoadPurchasedStateForAnotherEnvFailed) {
  auto development = SetupTestingStoreForEnv(skus::GetDefaultEnvironment());
  EXPECT_EQ(skus::GetEnvironmentForDomain(development),
            skus::GetDefaultEnvironment());
  TestBraveVPNServiceObserver observer;
  AddObserver(observer.GetReceiver());
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedInfoSync());
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_FALSE(observer.GetPurchasedState().has_value());

  LoadPurchasedState(development);
  task_environment_.RunUntilIdle();
  // Successfully set purchased state for dev env.
  EXPECT_TRUE(observer.GetPurchasedState().has_value());
  EXPECT_EQ(observer.GetPurchasedState().value(), PurchasedState::PURCHASED);
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_EQ(GetPurchasedInfoSync(), PurchasedState::PURCHASED);

  observer.ResetStates();
  SetInterceptorResponse("");
  std::string staging = skus::GetDomain("vpn", skus::kEnvStaging);
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  // no order found for staging.
  LoadPurchasedState(staging);
  task_environment_.RunUntilIdle();
  // The purchased state was not changed from dev env.
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_EQ(GetPurchasedInfoSync(), PurchasedState::PURCHASED);

  observer.ResetStates();
  staging = SetupTestingStoreForEnv(skus::kEnvStaging, false);
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  // No region data for staging.
  SetInterceptorResponse("");
  LoadPurchasedState(staging);
  task_environment_.RunUntilIdle();
  // The purchased state was not changed from dev env.
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_EQ(GetPurchasedInfoSync(), PurchasedState::PURCHASED);

  observer.ResetStates();
  staging = SetupTestingStoreForEnv(skus::kEnvStaging, false);
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  // Inactive staging subscription.
  OnCredentialSummary(staging, R"({ "active": false } )");
  // The purchased state was not changed from dev env.
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_EQ(GetPurchasedInfoSync(), PurchasedState::PURCHASED);

  observer.ResetStates();
  staging = SetupTestingStoreForEnv(skus::kEnvStaging, false);
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  // Invalid staging subscription.
  OnCredentialSummary(staging, R"([])");
  // The purchased state was not changed from dev env.
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  EXPECT_EQ(GetPurchasedInfoSync(), PurchasedState::PURCHASED);
}

TEST_F(BraveVPNServiceTest, CheckInitialPurchasedStateTest) {
  // Purchased state is not checked for fresh user.
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedInfoSync());

  // Set valid subscriber credential to pretend it's purchased user.
  SetValidSubscriberCredential();
  ResetVpnService();

  // Loading state if region data is not ready.
  EXPECT_EQ(PurchasedState::LOADING, GetPurchasedInfoSync());

  // Set in-valid subscriber credential but not empty to pretend it's purchased
  // user but expired while browser is terminated.
  // In this case, service should try to reload purchased state at startup.
  SetInvalidSubscriberCredential();
  ResetVpnService();
  EXPECT_EQ(PurchasedState::LOADING, GetPurchasedInfoSync());
}

TEST_F(BraveVPNServiceTest, SubscribedCredentialsWithTokenNoLongerValid) {
  std::string env = skus::GetDefaultEnvironment();

  SetPurchasedState(env, PurchasedState::LOADING);
  OnGetSubscriberCredentialV12(kTokenNoLongerValid, false);
  EXPECT_EQ(PurchasedState::LOADING, GetPurchasedInfoSync());
  EXPECT_TRUE(IsRetriedSkusCredential(&local_pref_service_));

  // Retrying only once.
  OnGetSubscriberCredentialV12(kTokenNoLongerValid, false);
  EXPECT_EQ(PurchasedState::FAILED, GetPurchasedInfoSync());
  EXPECT_TRUE(IsRetriedSkusCredential(&local_pref_service_));

  // Check retrying flag is cleared when we got valid subs-credential.
  OnGetSubscriberCredentialV12("valid-subs-credentials", true);
  EXPECT_FALSE(IsRetriedSkusCredential(&local_pref_service_));
}

// Test connection check is asked only when purchased state.
TEST_F(BraveVPNServiceTest, CheckConnectionStateAfterPurchased) {
  std::string env = skus::GetDefaultEnvironment();
  auto* test_api = GetConnectionAPIImpl();
  EXPECT_FALSE(test_api->IsConnectionChecked());
  SetPurchasedState(env, PurchasedState::NOT_PURCHASED);
  EXPECT_FALSE(test_api->IsConnectionChecked());
  SetPurchasedState(env, PurchasedState::PURCHASED);
  EXPECT_TRUE(test_api->IsConnectionChecked());
}

#endif

TEST_F(BraveVPNServiceTest, GetPurchasedInfoSync) {
  std::string env = skus::GetDefaultEnvironment();
  EXPECT_EQ(mojom::PurchasedState::NOT_PURCHASED, GetPurchasedInfoSync());

  SetPurchasedState(env, PurchasedState::LOADING);
  EXPECT_EQ(PurchasedState::LOADING, GetPurchasedInfoSync());

  SetPurchasedState(env, PurchasedState::PURCHASED);
  EXPECT_EQ(PurchasedState::PURCHASED, GetPurchasedInfoSync());

  SetPurchasedState(env, PurchasedState::FAILED);
  EXPECT_EQ(PurchasedState::FAILED, GetPurchasedInfoSync());

  SetPurchasedState(env, PurchasedState::NOT_PURCHASED);
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedInfoSync());
}

TEST_F(BraveVPNServiceTest, SetPurchasedState) {
  std::string env = skus::GetDefaultEnvironment();
  TestBraveVPNServiceObserver observer;
  AddObserver(observer.GetReceiver());
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedInfoSync());

  SetAndExpectPurchasedStateChange(&observer, env, PurchasedState::LOADING);
  SetAndExpectPurchasedStateChange(&observer, env, PurchasedState::FAILED);
  SetAndExpectPurchasedStateChange(&observer, env,
                                   PurchasedState::NOT_PURCHASED);
  SetAndExpectPurchasedStateChange(&observer, env, PurchasedState::PURCHASED);

  SetPurchasedState(env, PurchasedState::PURCHASED);
  task_environment_.RunUntilIdle();
  observer.ResetStates();
  // Do not notify if status is not changed.
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  SetPurchasedState(env, PurchasedState::PURCHASED);
  task_environment_.RunUntilIdle();
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
}

TEST_F(BraveVPNServiceTest, LoadPurchasedStateNotifications) {
  std::string env = skus::GetDefaultEnvironment();
  std::string domain = skus::GetDomain("vpn", env);
  TestBraveVPNServiceObserver observer;
  AddObserver(observer.GetReceiver());
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedInfoSync());

  LoadPurchasedState(domain);
  {
    // Loading state called if we load purchased state.
    base::RunLoop loop;
    observer.WaitPurchasedStateChange(loop.QuitClosure());
    loop.Run();
    EXPECT_EQ(PurchasedState::LOADING, GetPurchasedInfoSync());
    EXPECT_TRUE(observer.GetPurchasedState().has_value());
    EXPECT_EQ(PurchasedState::LOADING, observer.GetPurchasedState().value());
  }
  {
    base::RunLoop loop;
    observer.WaitPurchasedStateChange(loop.QuitClosure());
    loop.Run();
    EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedInfoSync());
    EXPECT_TRUE(observer.GetPurchasedState().has_value());
    EXPECT_EQ(PurchasedState::NOT_PURCHASED,
              observer.GetPurchasedState().value());
  }
  observer.ResetStates();
  EXPECT_FALSE(observer.GetPurchasedState().has_value());
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedInfoSync());
  LoadPurchasedState(domain);
  {
    // Loading state called whenever we load purchased state.
    base::RunLoop loop;
    observer.WaitPurchasedStateChange(loop.QuitClosure());
    loop.Run();
    EXPECT_EQ(PurchasedState::LOADING, GetPurchasedInfoSync());
    EXPECT_TRUE(observer.GetPurchasedState().has_value());
    EXPECT_EQ(PurchasedState::LOADING, observer.GetPurchasedState().value());
  }
  task_environment_.RunUntilIdle();
  EXPECT_TRUE(observer.GetPurchasedState().has_value());
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedInfoSync());
  // Observer called when state will be changed.
  SetAndExpectPurchasedStateChange(&observer, env, PurchasedState::LOADING);
  SetAndExpectPurchasedStateChange(&observer, env, PurchasedState::PURCHASED);
}

TEST_F(BraveVPNServiceTest, LoadPurchasedStateForAnotherEnv) {
  auto development = SetupTestingStoreForEnv(skus::GetDefaultEnvironment());
  EXPECT_EQ(skus::GetEnvironmentForDomain(development),
            skus::GetDefaultEnvironment());
  TestBraveVPNServiceObserver observer;
  AddObserver(observer.GetReceiver());
  EXPECT_EQ(PurchasedState::NOT_PURCHASED, GetPurchasedInfoSync());
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  LoadPurchasedState(development);
  task_environment_.RunUntilIdle();
  // Successfully set purchased state for dev env.
  EXPECT_TRUE(observer.GetPurchasedState().has_value());
  EXPECT_EQ(observer.GetPurchasedState().value(), PurchasedState::PURCHASED);
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());

  observer.ResetStates();
  auto staging = SetupTestingStoreForEnv(skus::kEnvStaging);
  EXPECT_EQ(skus::GetEnvironmentForDomain(staging), skus::kEnvStaging);
  EXPECT_EQ(GetCurrentEnvironment(), skus::GetDefaultEnvironment());
  LoadPurchasedState(staging);
  task_environment_.RunUntilIdle();
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
