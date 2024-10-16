/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_service_impl.h"

#include <stdint.h>

#include <algorithm>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/barrier_closure.h"
#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/functional/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_string_value_serializer.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/browser/android_util.h"
#include "brave/components/brave_rewards/browser/diagnostic_log.h"
#include "brave/components/brave_rewards/browser/logging.h"
#include "brave/components/brave_rewards/browser/publisher_utils.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_p3a.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/service_sandbox_type.h"
#include "brave/components/brave_rewards/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/common/rewards_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/parameters/rewards_parameters_provider.h"
#include "brave/components/brave_rewards/core/rewards_database.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "components/country_codes/country_codes.h"
#include "components/favicon/core/favicon_service.h"
#include "components/os_crypt/sync/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/service_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/url_data_source.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "net/base/url_util.h"
#include "net/cookies/canonical_cookie.h"
#include "net/cookies/cookie_util.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "third_party/icu/source/common/unicode/locid.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"

namespace brave_rewards {

static const unsigned int kRetriesCountOnNetworkChange = 1;

namespace {

constexpr int kDiagnosticLogMaxVerboseLevel = 6;
constexpr int kDiagnosticLogKeepNumLines = 20000;
constexpr int kDiagnosticLogMaxFileSize = 10 * (1024 * 1024);
constexpr char pref_prefix[] = "brave.rewards";
constexpr base::TimeDelta kP3AMonthlyReportingPeriod = base::Days(30);
constexpr base::TimeDelta kP3ATipReportDelay = base::Seconds(30);
constexpr base::TimeDelta kP3ADailyReportInterval = base::Days(1);
const std::set<std::string> kBitflyerCountries = {
    "JP"  // ID: 19024
};

bool DeleteFilesOnFileTaskRunner(
    const std::vector<base::FilePath>& file_paths) {
  bool result = true;

  for (const auto& file_path : file_paths) {
    if (!base::DeletePathRecursively(file_path)) {
      result = false;
    }
  }

  return result;
}

// Returns pair of string and its parsed counterpart. We parse it on the file
// thread for the performance sake. It's should be better to remove the string
// representation in the [far] future.
std::pair<std::string, base::Value> LoadStateOnFileTaskRunner(
    const base::FilePath& path) {
  std::string data;
  bool success = base::ReadFileToString(path, &data);

  // Make sure the file isn't empty.
  if (!success || data.empty()) {
    return {};
  }
  std::pair<std::string, base::Value> result;
  result.first = data;

  // Save deserialized version for recording P3A and future use.
  JSONStringValueDeserializer deserializer(data);
  int error_code = 0;
  std::string error_message;
  auto value = deserializer.Deserialize(&error_code, &error_message);
  if (!value) {
    VLOG(0) << "Cannot deserialize state, error code: " << error_code
            << " message: " << error_message;
    return result;
  }

  if (!value->is_dict()) {
    VLOG(0) << "Corrupted legacy state.";
    return result;
  }

  result.second = std::move(*value);

  return result;
}

time_t GetCurrentTimestamp() {
  return base::Time::NowFromSystemTime().ToTimeT();
}

std::string LoadOnFileTaskRunner(const base::FilePath& path) {
  std::string data;
  bool success = base::ReadFileToString(path, &data);

  // Make sure the file isn't empty.
  if (!success || data.empty()) {
    return "";
  }

  return data;
}

net::NetworkTrafficAnnotationTag
GetNetworkTrafficAnnotationTagForFaviconFetch() {
  return net::DefineNetworkTrafficAnnotation(
      "brave_rewards_favicon_fetcher", R"(
      semantics {
        sender:
          "Brave Rewards Media Fetcher"
        description:
          "Fetches favicon for media publishers in Rewards."
        trigger:
          "User visits a media publisher content."
        data: "Favicon for media publisher."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "This feature cannot be disabled by settings."
        policy_exception_justification:
          "Not implemented."
      })");
}

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTagForURLLoad() {
  return net::DefineNetworkTrafficAnnotation("rewards_service_impl", R"(
      semantics {
        sender: "Brave Rewards service"
        description:
          "This service lets users anonymously support the sites they visit by "
          "tallying the attention spent on visited sites and dividing up a "
          "monthly BAT contribution."
        trigger:
          "User-initiated for direct tipping or on a set interval while Brave "
          "is running for monthly contributions."
        data:
          "Publisher and contribution data."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature via the BAT icon in the URL "
          "bar or by visiting brave://rewards/."
        policy_exception_justification:
          "Not implemented."
      })");
}

std::string GetPrefPath(const std::string& name) {
  return base::StringPrintf("%s.%s", pref_prefix, name.c_str());
}

std::vector<std::string> GetISOCountries() {
  std::vector<std::string> countries;
  for (const char* const* country_pointer = icu::Locale::getISOCountries();
       *country_pointer; ++country_pointer) {
    countries.emplace_back(*country_pointer);
  }
  return countries;
}

mojom::RewardsParametersPtr RewardsParametersFromPrefs(PrefService& prefs) {
  auto params = internal::RewardsParametersProvider::DictToParameters(
      prefs.GetDict(prefs::kParameters));
  if (params) {
    return params;
  }
  return mojom::RewardsParameters::New();
}

template <typename Callback, typename... Args>
void DeferCallback(base::Location location, Callback callback, Args&&... args) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      location,
      base::BindOnce(std::move(callback), std::forward<Args>(args)...));
}

}  // namespace

// read comment about file pathes at src\base\files\file_path.h
#if BUILDFLAG(IS_WIN)
const base::FilePath::StringType kDiagnosticLogPath(L"Rewards.log");
const base::FilePath::StringType kLegacy_state(L"ledger_state");
const base::FilePath::StringType kPublisher_state(L"publisher_state");
const base::FilePath::StringType kPublisher_info_db(L"publisher_info_db");
const base::FilePath::StringType kPublishers_list(L"publishers_list");
#else
const base::FilePath::StringType kDiagnosticLogPath("Rewards.log");
const base::FilePath::StringType kLegacy_state("ledger_state");
const base::FilePath::StringType kPublisher_state("publisher_state");
const base::FilePath::StringType kPublisher_info_db("publisher_info_db");
const base::FilePath::StringType kPublishers_list("publishers_list");
#endif

RewardsServiceImpl::RewardsServiceImpl(
    PrefService* prefs,
    const base::FilePath& profile_path,
    favicon::FaviconService* favicon_service,
    RequestImageCallback request_image_callback,
    CancelImageRequestCallback cancel_image_request_callback,
    content::StoragePartition* storage_partition,
#if BUILDFLAG(ENABLE_GREASELION)
    greaselion::GreaselionService* greaselion_service,
#endif
    brave_wallet::BraveWalletService* brave_wallet_service)
    : prefs_(prefs),
      favicon_service_(favicon_service),
      request_image_callback_(request_image_callback),
      cancel_image_request_callback_(cancel_image_request_callback),
      storage_partition_(storage_partition),
#if BUILDFLAG(ENABLE_GREASELION)
      greaselion_service_(greaselion_service),
#endif
      brave_wallet_service_(brave_wallet_service),
      receiver_(this),
      file_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      json_sanitizer_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT})),
      legacy_state_path_(profile_path.Append(kLegacy_state)),
      publisher_state_path_(profile_path.Append(kPublisher_state)),
      publisher_info_db_path_(profile_path.Append(kPublisher_info_db)),
      publisher_list_path_(profile_path.Append(kPublishers_list)),
      diagnostic_log_(new DiagnosticLog(profile_path.Append(kDiagnosticLogPath),
                                        kDiagnosticLogMaxFileSize,
                                        kDiagnosticLogKeepNumLines)),
      notification_service_(new RewardsNotificationServiceImpl(prefs)),
      conversion_monitor_(prefs) {
  ready_ = std::make_unique<base::OneShotEvent>();

  if (base::FeatureList::IsEnabled(features::kVerboseLoggingFeature)) {
    persist_log_level_ = kDiagnosticLogMaxVerboseLevel;
  }

#if BUILDFLAG(ENABLE_GREASELION)
  if (greaselion_service_) {
    // Greaselion's rules may be ready before we register our observer, so check
    // for that here
    if (!greaselion_enabled_ && greaselion_service_->rules_ready()) {
      OnRulesReady(greaselion_service_);
    }
    greaselion_service_->AddObserver(this);
  }
#endif

  p3a::RecordAdTypesEnabled(prefs_);
}

RewardsServiceImpl::~RewardsServiceImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

#if BUILDFLAG(ENABLE_GREASELION)
  if (greaselion_service_) {
    greaselion_service_->RemoveObserver(this);
  }
#endif
}

void RewardsServiceImpl::ConnectionClosed() {
  BLOG(0, "Restarting engine process after disconnect");

  Reset();

  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&RewardsServiceImpl::StartEngineProcessIfNecessary,
                     AsWeakPtr()),
      base::Seconds(10));
}

bool RewardsServiceImpl::IsInitialized() {
  return Connected() && ready_->is_signaled();
}

void RewardsServiceImpl::Init(
    std::unique_ptr<RewardsServiceObserver> extension_observer,
    std::unique_ptr<RewardsNotificationServiceObserver> notification_observer) {
  notification_service_->Init(std::move(notification_observer));
  AddObserver(notification_service_.get());

  if (extension_observer) {
    extension_observer_ = std::move(extension_observer);
    AddObserver(extension_observer_.get());
  }

  CheckPreferences();
  InitPrefChangeRegistrar();
}

void RewardsServiceImpl::InitPrefChangeRegistrar() {
  profile_pref_change_registrar_.Init(prefs_);
  profile_pref_change_registrar_.Add(
      prefs::kAutoContributeEnabled,
      base::BindRepeating(&RewardsServiceImpl::OnPreferenceChanged,
                          base::Unretained(this)));
  profile_pref_change_registrar_.Add(
      brave_ads::prefs::kOptedInToNotificationAds,
      base::BindRepeating(&RewardsServiceImpl::OnPreferenceChanged,
                          base::Unretained(this)));
  profile_pref_change_registrar_.Add(
      ntp_background_images::prefs::
          kNewTabPageShowSponsoredImagesBackgroundImage,
      base::BindRepeating(&RewardsServiceImpl::OnPreferenceChanged,
                          base::Unretained(this)));
  profile_pref_change_registrar_.Add(
      brave_ads::prefs::kOptedInToSearchResultAds,
      base::BindRepeating(&RewardsServiceImpl::OnPreferenceChanged,
                          base::Unretained(this)));
}

void RewardsServiceImpl::OnPreferenceChanged(const std::string& key) {
  if (key == prefs::kAutoContributeEnabled) {
    if (prefs_->GetBoolean(prefs::kAutoContributeEnabled)) {
      StartEngineProcessIfNecessary();
    }
    // Must check for connected external wallet before recording
    // AC state.
    RecordBackendP3AStats();
  }

  if (key == ntp_background_images::prefs::
                 kNewTabPageShowSponsoredImagesBackgroundImage ||
      key == brave_ads::prefs::kOptedInToNotificationAds ||
      key == brave_ads::prefs::kOptedInToSearchResultAds) {
    p3a::RecordAdTypesEnabled(prefs_);
  }

  if (key == brave_ads::prefs::kOptedInToSearchResultAds) {
    GetExternalWallet(
        base::BindOnce(&RewardsServiceImpl::OnRecordBackendP3AExternalWallet,
                       AsWeakPtr(), false, true));
  }

#if BUILDFLAG(ENABLE_GREASELION)
  if (key == brave_ads::prefs::kOptedInToNotificationAds) {
    if (greaselion_service_) {
      greaselion_service_->SetFeatureEnabled(
          greaselion::ADS,
          prefs_->GetBoolean(brave_ads::prefs::kOptedInToNotificationAds));
    }
  }
#endif
}

void RewardsServiceImpl::CheckPreferences() {
  if (prefs_->GetBoolean(prefs::kAutoContributeEnabled) ||
      prefs_->GetBoolean(brave_ads::prefs::kOptedInToNotificationAds)) {
    // If the user has enabled Ads or AC, but the "enabled" pref is missing, set
    // the "enabled" pref to true.
    if (!prefs_->GetUserPrefValue(prefs::kEnabled)) {
      prefs_->SetBoolean(prefs::kEnabled, true);
    }
  }

  if (prefs_->GetUserPrefValue(prefs::kEnabled)) {
    // If the "enabled" pref is set, then start the background Rewards
    // utility process.
    StartEngineProcessIfNecessary();
  }

  // If Rewards is enabled and the user has a linked account, then ensure that
  // the "search result ads enabled" pref has the appropriate default value.
  if (prefs_->GetBoolean(prefs::kEnabled) &&
      !prefs_->GetString(prefs::kExternalWalletType).empty() &&
      !prefs_->HasPrefPath(brave_ads::prefs::kOptedInToSearchResultAds)) {
    prefs_->SetBoolean(brave_ads::prefs::kOptedInToSearchResultAds, false);
  }
}

void RewardsServiceImpl::StartEngineProcessIfNecessary() {
  if (Connected()) {
    BLOG(1, "Engine process is already running");
    return;
  }

  rewards_database_ = base::SequenceBound<internal::RewardsDatabase>(
      file_task_runner_, publisher_info_db_path_);

  BLOG(1, "Starting engine process");

  if (!engine_factory_.is_bound()) {
    content::ServiceProcessHost::Launch(
        engine_factory_.BindNewPipeAndPassReceiver(),
        content::ServiceProcessHost::Options()
            .WithDisplayName(IDS_UTILITY_PROCESS_REWARDS_NAME)
            .Pass());

    engine_factory_.set_disconnect_handler(
        base::BindOnce(&RewardsServiceImpl::ConnectionClosed, AsWeakPtr()));
  }

  auto options = HandleFlags(RewardsFlags::ForCurrentProcess());
  PrepareEngineEnvForTesting(*options);
  engine_factory_->CreateRewardsEngine(
      engine_.BindNewEndpointAndPassReceiver(),
      receiver_.BindNewEndpointAndPassRemote(), std::move(options),
      base::BindOnce(&RewardsServiceImpl::OnEngineCreated, AsWeakPtr()));
}

void RewardsServiceImpl::OnEngineCreated() {
  if (!Connected()) {
    return;
  }

  engine_->Initialize(
      base::BindOnce(&RewardsServiceImpl::OnEngineInitialized, AsWeakPtr()));
}

void RewardsServiceImpl::CreateRewardsWallet(
    const std::string& country,
    CreateRewardsWalletCallback callback) {
  using mojom::CreateRewardsWalletResult;

  auto on_start = [](base::WeakPtr<RewardsServiceImpl> self,
                     std::string country,
                     CreateRewardsWalletCallback callback) {
    if (!self || !self->Connected()) {
      std::move(callback).Run(CreateRewardsWalletResult::kUnexpected);
      return;
    }

    auto on_created = [](base::WeakPtr<RewardsServiceImpl> self,
                         std::string country,
                         CreateRewardsWalletCallback callback,
                         CreateRewardsWalletResult result) {
      if (!self) {
        std::move(callback).Run(CreateRewardsWalletResult::kUnexpected);
        return;
      }

      // If the server responds with `kGeoCountryAlreadyDeclared`,
      // optimistically assume that the user has already declared their
      // country correctly and save `country` in preferences.
      if (result != CreateRewardsWalletResult::kSuccess &&
          result != CreateRewardsWalletResult::kGeoCountryAlreadyDeclared) {
        std::move(callback).Run(result);
        return;
      }

      self->prefs_->SetString(prefs::kDeclaredGeo, country);

      // Record in which environment the wallet was created (for display on the
      // rewards internals page).
      auto on_get_environment = [](base::WeakPtr<RewardsServiceImpl> self,
                                   mojom::Environment environment) {
        if (self) {
          self->prefs_->SetInteger(prefs::kWalletCreationEnvironment,
                                   static_cast<int>(environment));
        }
      };

      self->GetEnvironment(base::BindOnce(on_get_environment, self));

      // After successfully creating a Rewards wallet for the first time,
      // automatically enable Ads and AC.
      if (!self->prefs_->GetBoolean(prefs::kEnabled)) {
        self->prefs_->SetBoolean(prefs::kEnabled, true);
        self->prefs_->SetString(prefs::kUserVersion,
                                prefs::kCurrentUserVersion);
        self->prefs_->SetBoolean(brave_ads::prefs::kOptedInToNotificationAds,
                                 true);

        // Set the user's current ToS version.
        self->prefs_->SetInteger(
            prefs::kTosVersion,
            RewardsParametersFromPrefs(*(self->prefs_))->tos_version);

        // Fetch the user's balance before turning on AC. We don't want to
        // automatically turn on AC if for some reason the user has a current
        // balance, as this could result in unintentional BAT transfers.
        auto on_balance = [](base::WeakPtr<RewardsServiceImpl> self,
                             mojom::BalancePtr balance) {
          if (self && balance && balance->total == 0) {
            self->SetAutoContributeEnabled(true);
          }
        };
        self->FetchBalance(base::BindOnce(on_balance, self));
      }

      // Notify observers that the Rewards wallet has been created.
      for (auto& observer : self->observers_) {
        observer.OnRewardsWalletCreated();
      }

      self->conversion_monitor_.RecordRewardsEnable();
      p3a::RecordAdTypesEnabled(self->prefs_);

      std::move(callback).Run(CreateRewardsWalletResult::kSuccess);
    };

    self->engine_->CreateRewardsWallet(
        country,
        base::BindOnce(on_created, self, country, std::move(callback)));
  };

  ready_->Post(FROM_HERE, base::BindOnce(on_start, AsWeakPtr(), country,
                                         std::move(callback)));
  StartEngineProcessIfNecessary();
}

void RewardsServiceImpl::GetUserType(
    base::OnceCallback<void(mojom::UserType)> callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         mojom::UserType::kUnconnected);
  }

  auto on_external_wallet =
      [](base::OnceCallback<void(mojom::UserType)> callback,
         mojom::ExternalWalletPtr wallet) {
        std::move(callback).Run(wallet ? mojom::UserType::kConnected
                                       : mojom::UserType::kUnconnected);
      };

  GetExternalWallet(base::BindOnce(on_external_wallet, std::move(callback)));
}

bool RewardsServiceImpl::IsTermsOfServiceUpdateRequired() {
  if (!prefs_->GetBoolean(prefs::kEnabled)) {
    return false;
  }
  int params_version = RewardsParametersFromPrefs(*prefs_)->tos_version;
  int user_version = prefs_->GetInteger(prefs::kTosVersion);
  return user_version < params_version;
}

void RewardsServiceImpl::AcceptTermsOfServiceUpdate() {
  if (IsTermsOfServiceUpdateRequired()) {
    int params_version = RewardsParametersFromPrefs(*prefs_)->tos_version;
    prefs_->SetInteger(prefs::kTosVersion, params_version);
    for (auto& observer : observers_) {
      observer.OnTermsOfServiceUpdateAccepted();
    }
  }
}

std::string RewardsServiceImpl::GetCountryCode() const {
  std::string declared_geo = prefs_->GetString(prefs::kDeclaredGeo);
  return !declared_geo.empty()
             ? declared_geo
             : country_codes::CountryIDToCountryString(
                   country_codes::GetCountryIDFromPrefs(prefs_));
}

void RewardsServiceImpl::GetAvailableCountries(
    GetAvailableCountriesCallback callback) const {
  static const std::vector<std::string> kISOCountries = GetISOCountries();

  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), kISOCountries);
  }

  auto on_external_wallet = [](GetAvailableCountriesCallback callback,
                               mojom::ExternalWalletPtr wallet) {
    // If the user is not currently connected to any wallet provider, then all
    // ISO country codes are available.
    if (!wallet) {
      return std::move(callback).Run(kISOCountries);
    }

    // If the user is currently connected to a bitFlyer wallet, then the only
    // available countries are |kBitflyerCountries|.
    if (wallet->type == internal::constant::kWalletBitflyer) {
      return std::move(callback).Run(std::vector<std::string>(
          kBitflyerCountries.begin(), kBitflyerCountries.end()));
    }

    // If the user is currently connected to any other external wallet provider,
    // then remove |kBitflyerCountries| from the list of ISO countries.
    static const std::vector<std::string> kNonBitflyerCountries = []() {
      auto countries = kISOCountries;
      auto removed =
          base::ranges::remove_if(countries, [](const std::string& country) {
            return base::Contains(kBitflyerCountries, country);
          });
      countries.erase(removed, countries.end());
      return countries;
    }();

    return std::move(callback).Run(kNonBitflyerCountries);
  };

  engine_->GetExternalWallet(
      base::BindOnce(on_external_wallet, std::move(callback)));
}

void RewardsServiceImpl::GetActivityInfoList(
    uint32_t start,
    uint32_t limit,
    mojom::ActivityInfoFilterPtr filter,
    GetPublisherInfoListCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         std::vector<mojom::PublisherInfoPtr>());
  }

  engine_->GetActivityInfoList(
      start, limit, std::move(filter),
      base::BindOnce(&RewardsServiceImpl::OnGetPublisherInfoList, AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::GetPublishersVisitedCount(
    base::OnceCallback<void(int)> callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), 0);
  }
  engine_->GetPublishersVisitedCount(std::move(callback));
}

void RewardsServiceImpl::GetExcludedList(
    GetPublisherInfoListCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         std::vector<mojom::PublisherInfoPtr>());
  }

  engine_->GetExcludedList(
      base::BindOnce(&RewardsServiceImpl::OnGetPublisherInfoList, AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnGetPublisherInfoList(
    GetPublisherInfoListCallback callback,
    std::vector<mojom::PublisherInfoPtr> list) {
  std::move(callback).Run(std::move(list));
}

void RewardsServiceImpl::OnLoad(SessionID tab_id, const GURL& url) {
  if (!Connected()) {
    return;
  }

  if (IsAutoContributeHandledByContentScript(url)) {
    return;
  }

  auto publisher_domain = GetPublisherDomainFromURL(url);
  if (!publisher_domain) {
    return;
  }

  mojom::VisitDataPtr data = mojom::VisitData::New();
  data->domain = *publisher_domain;
  data->name = *publisher_domain;
  data->path = url.path();
  data->tab_id = tab_id.id();
  data->url = url.scheme() + "://" + *publisher_domain + "/";
  engine_->OnLoad(std::move(data), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnUnload(SessionID tab_id) {
  if (!Connected()) {
    return;
  }

  engine_->OnUnload(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnShow(SessionID tab_id) {
  if (!Connected()) {
    return;
  }

  engine_->OnShow(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnHide(SessionID tab_id) {
  if (!Connected()) {
    return;
  }

  engine_->OnHide(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnForeground(SessionID tab_id) {
  if (!Connected()) {
    return;
  }

  engine_->OnForeground(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnBackground(SessionID tab_id) {
  if (!Connected()) {
    return;
  }

  engine_->OnBackground(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnXHRLoad(SessionID tab_id,
                                   const GURL& url,
                                   const GURL& first_party_url,
                                   const GURL& referrer) {
  if (!Connected()) {
    return;
  }

  if (IsAutoContributeHandledByContentScript(url) ||
      !GetPublisherDomainFromURL(url)) {
    return;
  }

  base::flat_map<std::string, std::string> parts;

  for (net::QueryIterator it(url); !it.IsAtEnd(); it.Advance()) {
    parts[std::string(it.GetKey())] = it.GetUnescapedValue();
  }

  mojom::VisitDataPtr data = mojom::VisitData::New();
  data->path = url.spec();
  data->tab_id = tab_id.id();

  engine_->OnXHRLoad(tab_id.id(), url.spec(), parts, first_party_url.spec(),
                     referrer.spec(), std::move(data));
}

void RewardsServiceImpl::OnRestorePublishers(const mojom::Result result) {
  if (result != mojom::Result::OK) {
    return;
  }

  for (auto& observer : observers_) {
    observer.OnExcludedSitesChanged(
        this, "-1", static_cast<int>(mojom::PublisherExclude::ALL));
  }
}

void RewardsServiceImpl::RestorePublishers() {
  if (!Connected()) {
    return;
  }

  engine_->RestorePublishers(
      base::BindOnce(&RewardsServiceImpl::OnRestorePublishers, AsWeakPtr()));
}

std::string RewardsServiceImpl::UrlMethodToRequestType(
    mojom::UrlMethod method) {
  switch (method) {
    case mojom::UrlMethod::GET:
      return "GET";
    case mojom::UrlMethod::POST:
      return "POST";
    case mojom::UrlMethod::PUT:
      return "PUT";
    case mojom::UrlMethod::PATCH:
      return "PATCH";
    case mojom::UrlMethod::DEL:
      return "DELETE";
  }
  NOTREACHED();
}

void RewardsServiceImpl::Shutdown() {
  engine_.reset();
  receiver_.reset();

  RemoveObserver(notification_service_.get());

  if (extension_observer_) {
    RemoveObserver(extension_observer_.get());
  }

  if (!cancel_image_request_callback_.is_null()) {
    for (auto mapping : current_media_fetchers_) {
      cancel_image_request_callback_.Run(mapping.second);
    }
  }

  url_loaders_.clear();

  RewardsService::Shutdown();
}

void RewardsServiceImpl::OnEngineInitialized(mojom::Result result) {
  if (!ready_->is_signaled()) {
    ready_->Signal();
  }

  RecordBackendP3AStats(/*delay_report*/ true);
  p3a_daily_timer_.Start(
      FROM_HERE, base::Time::Now() + kP3ADailyReportInterval,
      base::BindOnce(&RewardsServiceImpl::OnP3ADailyTimer, AsWeakPtr()));

  for (auto& observer : observers_) {
    observer.OnRewardsInitialized(this);
  }
}

void RewardsServiceImpl::IsAutoContributeSupported(
    base::OnceCallback<void(bool)> callback) {
  IsAutoContributeSupportedForClient(std::move(callback));
}

void RewardsServiceImpl::GetAutoContributeProperties(
    GetAutoContributePropertiesCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), nullptr);
  }

  engine_->GetAutoContributeProperties(std::move(callback));
}

void RewardsServiceImpl::OnReconcileComplete(
    mojom::Result result,
    mojom::ContributionInfoPtr contribution) {
  if (result == mojom::Result::OK &&
      contribution->type == mojom::RewardsType::RECURRING_TIP) {
    MaybeShowNotificationTipsPaid();
  }

  if (result == mojom::Result::OK) {
    RecordBackendP3AStats();
  }

  for (auto& observer : observers_)
    observer.OnReconcileComplete(
        this,
        result,
        contribution->contribution_id,
        contribution->amount,
        contribution->type,
        contribution->processor);
}

void RewardsServiceImpl::LoadLegacyState(LoadLegacyStateCallback callback) {
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&LoadStateOnFileTaskRunner, legacy_state_path_),
      base::BindOnce(&RewardsServiceImpl::OnLegacyStateLoaded, AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnLegacyStateLoaded(
    LoadLegacyStateCallback callback,
    std::pair<std::string, base::Value> state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (state.second.is_dict()) {
    // Record stats.
    RecordBackendP3AStats();
  }
  if (state.first.empty()) {
    p3a::RecordNoWalletCreatedForAllMetrics();
  }

  // Run callbacks.
  const std::string& data = state.first;
  std::move(callback).Run(
      data.empty() ? mojom::Result::NO_LEGACY_STATE : mojom::Result::OK, data);
}

void RewardsServiceImpl::LoadPublisherState(
    LoadPublisherStateCallback callback) {
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&LoadOnFileTaskRunner, publisher_state_path_),
      base::BindOnce(&RewardsServiceImpl::OnPublisherStateLoaded, AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnPublisherStateLoaded(
    LoadPublisherStateCallback callback,
    const std::string& data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!Connected()) {
    return;
  }

  std::move(callback).Run(
      data.empty() ? mojom::Result::NO_PUBLISHER_STATE : mojom::Result::OK,
      data);
}

void RewardsServiceImpl::LoadURL(mojom::UrlRequestPtr request,
                                 LoadURLCallback callback) {
  if (!request || request->url.empty()) {
    auto response = mojom::UrlResponse::New();
    response->status_code = net::HTTP_BAD_REQUEST;
    std::move(callback).Run(std::move(response));
    return;
  }

  GURL parsed_url(request->url);
  if (!parsed_url.is_valid()) {
    auto response = mojom::UrlResponse::New();
    response->url = request->url;
    response->status_code = net::HTTP_BAD_REQUEST;
    std::move(callback).Run(std::move(response));
    return;
  }

  if (test_response_callback_) {
    std::string test_response;
    base::flat_map<std::string, std::string> test_headers;
    int response_status_code = net::HTTP_OK;
    test_response_callback_.Run(
        request->url,
        static_cast<int>(request->method),
        &response_status_code,
        &test_response,
        &test_headers);

    auto response = mojom::UrlResponse::New();
    response->url = request->url;
    response->status_code = response_status_code;
    response->body = test_response;
    response->headers = std::move(test_headers);
    std::move(callback).Run(std::move(response));
    return;
  }

  auto net_request = std::make_unique<network::ResourceRequest>();
  net_request->url = parsed_url;
  net_request->method = UrlMethodToRequestType(request->method);
  net_request->load_flags = request->load_flags;

  // Loading Twitter requires credentials
  if (net_request->url.DomainIs("twitter.com")) {
    net_request->credentials_mode = network::mojom::CredentialsMode::kInclude;

#if BUILDFLAG(IS_ANDROID)
    net_request->headers.SetHeader(
        net::HttpRequestHeaders::kUserAgent,
        "DESKTOP");
#endif

  } else {
    net_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  }

  for (const auto& header : request->headers) {
    net_request->headers.AddHeaderFromString(header);
  }

  auto loader = network::SimpleURLLoader::Create(
      std::move(net_request), GetNetworkTrafficAnnotationTagForURLLoad());
  loader->SetAllowHttpErrorResults(true);
  loader->SetRetryOptions(
      kRetriesCountOnNetworkChange,
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);

  if (!request->content.empty()) {
    loader->AttachStringForUpload(request->content, request->content_type);
  }

  auto loader_it = url_loaders_.insert(url_loaders_.begin(), std::move(loader));
  loader_it->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      storage_partition_->GetURLLoaderFactoryForBrowserProcess().get(),
      base::BindOnce(&RewardsServiceImpl::OnURLLoaderComplete,
                     base::Unretained(this), loader_it, std::move(callback)));
}

void RewardsServiceImpl::OnURLLoaderComplete(
    SimpleURLLoaderList::iterator loader_it,
    LoadURLCallback callback,
    std::unique_ptr<std::string> response_body) {
  auto loader = std::move(*loader_it);
  url_loaders_.erase(loader_it);

  auto response = mojom::UrlResponse::New();
  response->body = response_body ? *response_body : "";

  if (loader->NetError() != net::OK) {
    response->error = net::ErrorToString(loader->NetError());
  }

  int response_code = -1;
  if (loader->ResponseInfo() && loader->ResponseInfo()->headers) {
    response_code = loader->ResponseInfo()->headers->response_code();
  }
  response->status_code = response_code;

  const auto url = loader->GetFinalURL();
  response->url = url.spec();

  if (loader->ResponseInfo()) {
    scoped_refptr<net::HttpResponseHeaders> headersList =
        loader->ResponseInfo()->headers;

    if (headersList) {
      size_t iter = 0;
      std::string key;
      std::string value;
      while (headersList->EnumerateHeaderLines(&iter, &key, &value)) {
        key = base::ToLowerASCII(key);
        response->headers[key] = value;
      }
    }
  }

  if (response_body && !response_body->empty() && loader->ResponseInfo() &&
      base::Contains(loader->ResponseInfo()->mime_type, "json")) {
    json_sanitizer_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](std::unique_ptr<std::string> response_body,
               LoadURLCallback callback, mojom::UrlResponsePtr response,
               scoped_refptr<base::SequencedTaskRunner> post_response_runner) {
              api_request_helper::SanitizeAndParseJson(
                  *response_body,
                  base::BindOnce(
                      [](LoadURLCallback callback,
                         mojom::UrlResponsePtr response,
                         const scoped_refptr<base::SequencedTaskRunner>&
                             post_response_runner,
                         api_request_helper::ValueOrError result) {
                        if (result.has_value()) {
                          std::string json;
                          base::JSONWriter::Write(std::move(result).value(),
                                                  &json);
                          response->body = std::move(json);
                        } else {
                          response->body = {};
                          VLOG(0) << "Response sanitization error: "
                                  << result.error();
                        }

                        post_response_runner->PostTask(
                            FROM_HERE, base::BindOnce(std::move(callback),
                                                      std::move(response)));
                      },
                      std::move(callback), std::move(response),
                      std::move(post_response_runner)));
            },
            std::move(response_body), std::move(callback), std::move(response),
            base::SequencedTaskRunner::GetCurrentDefault()));

    return;
  }

  std::move(callback).Run(std::move(response));
}

void RewardsServiceImpl::GetSPLTokenAccountBalance(
    const std::string& solana_address,
    const std::string& token_mint_address,
    GetSPLTokenAccountBalanceCallback callback) {
  if (!brave_wallet_service_) {
    std::move(callback).Run(nullptr);
    return;
  }

  brave_wallet_service_->json_rpc_service()->GetSPLTokenAccountBalance(
      solana_address, token_mint_address, brave_wallet::mojom::kSolanaMainnet,
      base::BindOnce(&RewardsServiceImpl::OnGetSPLTokenAccountBalance,
                     AsWeakPtr(), std::move(callback)));
}

void RewardsServiceImpl::OnGetSPLTokenAccountBalance(
    GetSPLTokenAccountBalanceCallback callback,
    const std::string& amount,
    uint8_t decimals,
    const std::string& amount_string,
    brave_wallet::mojom::SolanaProviderError error,
    const std::string& error_message) {
  if (error != brave_wallet::mojom::SolanaProviderError::kSuccess) {
    BLOG(0, "Unable to retrieve Solana account balance: " << error);
    std::move(callback).Run(nullptr);
    return;
  }

  auto balance = mojom::SolanaAccountBalance::New();
  balance->amount = amount;
  balance->decimals = decimals;
  std::move(callback).Run(std::move(balance));
}

void RewardsServiceImpl::GetRewardsParameters(
    GetRewardsParametersCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), nullptr);
  }

  engine_->GetRewardsParameters(
      base::BindOnce(&OnGetRewardsParameters, std::move(callback)));
}

void RewardsServiceImpl::OnGetRewardsParameters(
    GetRewardsParametersCallback callback,
    mojom::RewardsParametersPtr parameters) {
  if (parameters) {
    if (base::FeatureList::IsEnabled(
            features::kAllowUnsupportedWalletProvidersFeature)) {
      parameters->wallet_provider_regions.clear();
    }
  }

  std::move(callback).Run(std::move(parameters));
}

std::vector<std::string> RewardsServiceImpl::GetExternalWalletProviders()
    const {
  std::vector<std::string> providers;

  if (IsBitFlyerCountry()) {
    providers.push_back(internal::constant::kWalletBitflyer);
  } else if (GetCountryCode() == "IN") {
    providers.push_back(internal::constant::kWalletZebPay);
  } else {
    providers.push_back(internal::constant::kWalletUphold);

#if BUILDFLAG(ENABLE_GEMINI_WALLET)
    if (base::FeatureList::IsEnabled(features::kGeminiFeature)) {
      providers.push_back(internal::constant::kWalletGemini);
    }
#endif
  }

  if (base::FeatureList::IsEnabled(
          features::kAllowSelfCustodyProvidersFeature)) {
    auto& self_custody_dict = prefs_->GetDict(prefs::kSelfCustodyAvailable);

    if (auto solana_entry =
            self_custody_dict.FindBool(internal::constant::kWalletSolana);
        solana_entry && *solana_entry) {
      providers.push_back(internal::constant::kWalletSolana);
    }
  }

  return providers;
}

void RewardsServiceImpl::GetReconcileStamp(GetReconcileStampCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), 0);
  }

  engine_->GetReconcileStamp(std::move(callback));
}

#if BUILDFLAG(ENABLE_GREASELION)
void RewardsServiceImpl::EnableGreaselion() {
  if (!greaselion_service_) {
    return;
  }

  greaselion_service_->SetFeatureEnabled(greaselion::REWARDS, true);
  greaselion_service_->SetFeatureEnabled(
      greaselion::AUTO_CONTRIBUTION,
      prefs_->GetBoolean(prefs::kAutoContributeEnabled));
  greaselion_service_->SetFeatureEnabled(
      greaselion::ADS,
      prefs_->GetBoolean(brave_ads::prefs::kOptedInToNotificationAds));

  greaselion_enabled_ = true;
}

void RewardsServiceImpl::OnRulesReady(
    greaselion::GreaselionService* greaselion_service) {
  EnableGreaselion();
}
#endif

void RewardsServiceImpl::StopEngine(StopEngineCallback callback) {
  BLOG(1, "Shutting down rewards process");
  if (!Connected()) {
    BLOG(1, "Rewards process not running");
    OnStopEngine(std::move(callback), mojom::Result::OK);
    return;
  }

  p3a_daily_timer_.Stop();
  p3a_tip_report_timer_.Stop();
  p3a::RecordNoWalletCreatedForAllMetrics();

  engine_->Shutdown(base::BindOnce(&RewardsServiceImpl::OnStopEngine,
                                   AsWeakPtr(), std::move(callback)));
}

void RewardsServiceImpl::OnStopEngine(StopEngineCallback callback,
                                      const mojom::Result result) {
  BLOG_IF(1, result != mojom::Result::OK,
          "Rewards process was not shut down successfully");
  Reset();
  BLOG(1, "Successfully shutdown rewards process");
  std::move(callback).Run(result);
}

void RewardsServiceImpl::OnStopEngineForCompleteReset(
    SuccessCallback callback,
    const mojom::Result result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  prefs_->ClearPrefsWithPrefixSilently(pref_prefix);
  diagnostic_log_->Delete(base::BindOnce(
      &RewardsServiceImpl::OnDiagnosticLogDeletedForCompleteReset, AsWeakPtr(),
      std::move(callback)));

  p3a::RecordAdTypesEnabled(prefs_);
}

void RewardsServiceImpl::OnDiagnosticLogDeletedForCompleteReset(
    SuccessCallback callback,
    bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const std::vector<base::FilePath> paths = {
      legacy_state_path_,
      publisher_state_path_,
      publisher_info_db_path_,
      publisher_list_path_,
  };
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&DeleteFilesOnFileTaskRunner, paths),
      base::BindOnce(&RewardsServiceImpl::OnFilesDeletedForCompleteReset,
                     AsWeakPtr(), std::move(callback)));
}

void RewardsServiceImpl::Reset() {
  url_loaders_.clear();

  if (!cancel_image_request_callback_.is_null()) {
    for (auto mapping : current_media_fetchers_) {
      cancel_image_request_callback_.Run(mapping.second);
    }
  }

  current_media_fetchers_.clear();
  engine_.reset();
  receiver_.reset();
  engine_factory_.reset();
  ready_ = std::make_unique<base::OneShotEvent>();
  rewards_database_.Reset();
  BLOG(1, "Successfully reset rewards service");
}

void RewardsServiceImpl::SetBooleanState(const std::string& name,
                                         bool value,
                                         SetBooleanStateCallback callback) {
  prefs_->SetBoolean(GetPrefPath(name), value);
  std::move(callback).Run();
}

void RewardsServiceImpl::GetBooleanState(const std::string& name,
                                         GetBooleanStateCallback callback) {
  std::move(callback).Run(prefs_->GetBoolean(GetPrefPath(name)));
}

void RewardsServiceImpl::SetIntegerState(const std::string& name,
                                         int32_t value,
                                         SetIntegerStateCallback callback) {
  prefs_->SetInteger(GetPrefPath(name), value);
  std::move(callback).Run();
}

void RewardsServiceImpl::GetIntegerState(const std::string& name,
                                         GetIntegerStateCallback callback) {
  std::move(callback).Run(prefs_->GetInteger(GetPrefPath(name)));
}

void RewardsServiceImpl::SetDoubleState(const std::string& name,
                                        double value,
                                        SetDoubleStateCallback callback) {
  prefs_->SetDouble(GetPrefPath(name), value);
  std::move(callback).Run();
}

void RewardsServiceImpl::GetDoubleState(const std::string& name,
                                        GetDoubleStateCallback callback) {
  std::move(callback).Run(prefs_->GetDouble(GetPrefPath(name)));
}

void RewardsServiceImpl::SetStringState(const std::string& name,
                                        const std::string& value,
                                        SetStringStateCallback callback) {
  prefs_->SetString(GetPrefPath(name), value);
  std::move(callback).Run();
}

void RewardsServiceImpl::GetStringState(const std::string& name,
                                        GetStringStateCallback callback) {
  std::move(callback).Run(prefs_->GetString(GetPrefPath(name)));
}

void RewardsServiceImpl::SetInt64State(const std::string& name,
                                       int64_t value,
                                       SetInt64StateCallback callback) {
  prefs_->SetInt64(GetPrefPath(name), value);
  std::move(callback).Run();
}

void RewardsServiceImpl::GetInt64State(const std::string& name,
                                       GetInt64StateCallback callback) {
  std::move(callback).Run(prefs_->GetInt64(GetPrefPath(name)));
}

void RewardsServiceImpl::SetUint64State(const std::string& name,
                                        uint64_t value,
                                        SetUint64StateCallback callback) {
  prefs_->SetUint64(GetPrefPath(name), value);
  std::move(callback).Run();
}

void RewardsServiceImpl::GetUint64State(const std::string& name,
                                        GetUint64StateCallback callback) {
  std::move(callback).Run(prefs_->GetUint64(GetPrefPath(name)));
}

void RewardsServiceImpl::SetValueState(const std::string& name,
                                       base::Value value,
                                       SetValueStateCallback callback) {
  prefs_->Set(GetPrefPath(name), std::move(value));
  std::move(callback).Run();
}

void RewardsServiceImpl::GetValueState(const std::string& name,
                                       GetValueStateCallback callback) {
  std::move(callback).Run(prefs_->GetValue(GetPrefPath(name)).Clone());
}

void RewardsServiceImpl::SetTimeState(const std::string& name,
                                      base::Time value,
                                      SetTimeStateCallback callback) {
  prefs_->SetTime(GetPrefPath(name), value);
  std::move(callback).Run();
}

void RewardsServiceImpl::GetTimeState(const std::string& name,
                                      GetTimeStateCallback callback) {
  std::move(callback).Run(prefs_->GetTime(GetPrefPath(name)));
}

void RewardsServiceImpl::ClearState(const std::string& name,
                                    ClearStateCallback callback) {
  prefs_->ClearPref(GetPrefPath(name));
  std::move(callback).Run();
}

void RewardsServiceImpl::GetClientCountryCode(
    GetClientCountryCodeCallback callback) {
  std::move(callback).Run(GetCountryCode());
}

void RewardsServiceImpl::IsAutoContributeSupportedForClient(
    IsAutoContributeSupportedForClientCallback callback) {
  const auto country_code = GetCountryCode();
  std::move(callback).Run(IsAutoContributeSupportedForCountry(country_code));
}

void RewardsServiceImpl::GetPublisherMinVisitTime(
    GetPublisherMinVisitTimeCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), 0);
  }

  engine_->GetPublisherMinVisitTime(std::move(callback));
}

void RewardsServiceImpl::SetPublisherMinVisitTime(
    int duration_in_seconds) const {
  if (!Connected()) {
    return;
  }

  engine_->SetPublisherMinVisitTime(duration_in_seconds);
}

void RewardsServiceImpl::GetPublisherMinVisits(
    GetPublisherMinVisitsCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), 0);
  }

  engine_->GetPublisherMinVisits(std::move(callback));
}

void RewardsServiceImpl::SetPublisherMinVisits(int visits) const {
  if (!Connected()) {
    return;
  }

  engine_->SetPublisherMinVisits(visits);
}

void RewardsServiceImpl::SetAutoContributionAmount(const double amount) const {
  if (!Connected()) {
    return;
  }

  engine_->SetAutoContributionAmount(amount);
}

void RewardsServiceImpl::GetAutoContributeEnabled(
    GetAutoContributeEnabledCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), false);
  }

  engine_->GetAutoContributeEnabled(std::move(callback));
}

void RewardsServiceImpl::SetAutoContributeEnabled(bool enabled) {
  if (!Connected()) {
    return;
  }

  engine_->SetAutoContributeEnabled(enabled);
}

void RewardsServiceImpl::OnGetBalanceReport(
    GetBalanceReportCallback callback,
    const mojom::Result result,
    mojom::BalanceReportInfoPtr report) {
  std::move(callback).Run(result, std::move(report));
}

void RewardsServiceImpl::GetBalanceReport(
    const uint32_t month,
    const uint32_t year,
    GetBalanceReportCallback callback) {
  if (!Connected() || month < 1 || month > 12) {
    return DeferCallback(FROM_HERE, std::move(callback), mojom::Result::FAILED,
                         nullptr);
  }

  engine_->GetBalanceReport(
      static_cast<mojom::ActivityMonth>(month), year,
      base::BindOnce(&RewardsServiceImpl::OnGetBalanceReport, AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::GetPublisherActivityFromUrl(
    uint64_t windowId,
    const std::string& url,
    const std::string& favicon_url,
    const std::string& publisher_blob) {
  GURL parsed_url(url);
  if (!parsed_url.is_valid() ||
      IsAutoContributeHandledByContentScript(parsed_url)) {
    return;
  }

  auto publisher_domain = GetPublisherDomainFromURL(parsed_url);
  if (!publisher_domain) {
    mojom::PublisherInfoPtr info;
    OnPanelPublisherInfo(mojom::Result::NOT_FOUND, std::move(info), windowId);
    return;
  }

  if (!Connected()) {
    return;
  }

  mojom::VisitDataPtr visit_data = mojom::VisitData::New();
  visit_data->domain = *publisher_domain;
  visit_data->name = *publisher_domain;
  visit_data->path = parsed_url.has_path() ? parsed_url.PathForRequest() : "";
  visit_data->url = parsed_url.scheme() + "://" + *publisher_domain + "/";
  visit_data->favicon_url = favicon_url;

  engine_->GetPublisherActivityFromUrl(windowId, std::move(visit_data),
                                       publisher_blob);
}

void RewardsServiceImpl::OnPanelPublisherInfo(mojom::Result result,
                                              mojom::PublisherInfoPtr info,
                                              uint64_t windowId) {
  if (result != mojom::Result::OK && result != mojom::Result::NOT_FOUND) {
    return;
  }

  for (auto& observer : observers_) {
    observer.OnPanelPublisherInfo(this, result, info.get(), windowId);
  }
}

void RewardsServiceImpl::GetAutoContributionAmount(
    GetAutoContributionAmountCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), 0);
  }

  engine_->GetAutoContributionAmount(std::move(callback));
}

void RewardsServiceImpl::FetchFavIcon(const std::string& url,
                                      const std::string& favicon_key,
                                      FetchFavIconCallback callback) {
  GURL parsedUrl(url);

  if (!parsedUrl.is_valid()) {
    return std::move(callback).Run(false, "");
  }

  auto it = current_media_fetchers_.find(url);
  if (it != current_media_fetchers_.end()) {
    BLOG(1, "Already fetching favicon");
    return std::move(callback).Run(false, "");
  }

  if (!request_image_callback_.is_null()) {
    current_media_fetchers_[url] = request_image_callback_.Run(
        parsedUrl,
        base::BindOnce(
            &RewardsServiceImpl::OnFetchFavIconCompleted, AsWeakPtr(),
            // Internally, BitmapFetcherService::OnFetchComplete() passes the
            // bitmap to the request via NotifyImageChanged(), and then drops
            // the request. BitmapFetcherRequest::NotifyImageChanged(), however,
            // fails to call the callback if !bitmap || bitmap->empty(), hence
            // BitmapFetcherService might end up dropping the callback while
            // dropping the request.
            // Wrap the callback until upstream sorts out the issue!
            mojo::WrapCallbackWithDefaultInvokeIfNotRun(std::move(callback),
                                                        false, ""),
            favicon_key, parsedUrl),
        GetNetworkTrafficAnnotationTagForFaviconFetch());
  } else {
    std::move(callback).Run(false, "");
  }
}

void RewardsServiceImpl::OnFetchFavIconCompleted(FetchFavIconCallback callback,
                                                 const std::string& favicon_key,
                                                 const GURL& url,
                                                 const SkBitmap& image) {
  GURL favicon_url(favicon_key);
  gfx::Image gfx_image = gfx::Image::CreateFrom1xBitmap(image);

  favicon_service_->SetOnDemandFavicons(
      favicon_url, url, favicon_base::IconType::kFavicon, gfx_image,
      base::BindOnce(&RewardsServiceImpl::OnSetOnDemandFaviconComplete,
                     AsWeakPtr(), favicon_url.spec(), std::move(callback)));

  auto it_url = current_media_fetchers_.find(url.spec());
  if (it_url != current_media_fetchers_.end()) {
    current_media_fetchers_.erase(it_url);
  }
}

void RewardsServiceImpl::OnSetOnDemandFaviconComplete(
    const std::string& favicon_url,
    FetchFavIconCallback callback,
    bool success) {
  std::move(callback).Run(success, favicon_url);
}

void RewardsServiceImpl::GetPublisherBanner(
    const std::string& publisher_id,
    GetPublisherBannerCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), nullptr);
  }

  engine_->GetPublisherBanner(
      publisher_id, base::BindOnce(&RewardsServiceImpl::OnPublisherBanner,
                                   AsWeakPtr(), std::move(callback)));
}

void RewardsServiceImpl::OnPublisherBanner(GetPublisherBannerCallback callback,
                                           mojom::PublisherBannerPtr banner) {
  std::move(callback).Run(std::move(banner));
}

void RewardsServiceImpl::OnSaveRecurringTip(OnTipCallback callback,
                                            mojom::Result result) {
  for (auto& observer : observers_) {
    observer.OnRecurringTipSaved(this, result == mojom::Result::OK);
  }

  std::move(callback).Run(result);
}

void RewardsServiceImpl::SaveRecurringTip(const std::string& publisher_key,
                                          double amount,
                                          OnTipCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), mojom::Result::FAILED);
  }

  mojom::RecurringTipPtr info = mojom::RecurringTip::New();
  info->publisher_key = publisher_key;
  info->amount = amount;
  info->created_at = GetCurrentTimestamp();

  engine_->SaveRecurringTip(
      std::move(info), base::BindOnce(&RewardsServiceImpl::OnSaveRecurringTip,
                                      AsWeakPtr(), std::move(callback)));
}

void RewardsServiceImpl::SendContribution(
    const std::string& publisher_key,
    double amount,
    bool set_monthly,
    base::OnceCallback<void(bool)> callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), false);
  }

  engine_->SendContribution(
      publisher_key, amount, set_monthly,
      base::BindOnce(&RewardsServiceImpl::OnContributionSent, AsWeakPtr(),
                     set_monthly, std::move(callback)));
}

void RewardsServiceImpl::OnContributionSent(
    bool set_monthly,
    base::OnceCallback<void(bool)> callback,
    bool success) {
  if (set_monthly) {
    for (auto& observer : observers_) {
      observer.OnRecurringTipSaved(this, success);
    }
  }

  RecordBackendP3AStats(/*delay_report*/ true);

  std::move(callback).Run(success);
}

void RewardsServiceImpl::UpdateMediaDuration(
    const uint64_t window_id,
    const std::string& publisher_key,
    const uint64_t duration,
    const bool first_visit) {
  if (!Connected()) {
    return;
  }

  engine_->UpdateMediaDuration(window_id, publisher_key, duration, first_visit);
}

void RewardsServiceImpl::IsPublisherRegistered(
    const std::string& publisher_id,
    base::OnceCallback<void(bool)> callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), false);
  }

  engine_->IsPublisherRegistered(publisher_id, std::move(callback));
}

void RewardsServiceImpl::GetPublisherInfo(
    const std::string& publisher_key,
    GetPublisherInfoCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), mojom::Result::FAILED,
                         nullptr);
  }

  engine_->GetPublisherInfo(publisher_key, std::move(callback));
}

void RewardsServiceImpl::GetPublisherPanelInfo(
    const std::string& publisher_key,
    GetPublisherInfoCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), mojom::Result::FAILED,
                         nullptr);
  }

  engine_->GetPublisherPanelInfo(publisher_key, std::move(callback));
}

void RewardsServiceImpl::SavePublisherInfo(
    const uint64_t window_id,
    mojom::PublisherInfoPtr publisher_info,
    SavePublisherInfoCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), mojom::Result::FAILED);
  }

  engine_->SavePublisherInfo(window_id, std::move(publisher_info),
                             std::move(callback));
}

void RewardsServiceImpl::GetRecurringTips(
    GetRecurringTipsCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         std::vector<mojom::PublisherInfoPtr>());
  }

  engine_->GetRecurringTips(std::move(callback));
}

void RewardsServiceImpl::GetOneTimeTips(GetOneTimeTipsCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         std::vector<mojom::PublisherInfoPtr>());
  }

  engine_->GetOneTimeTips(std::move(callback));
}

void RewardsServiceImpl::OnRecurringTip(const mojom::Result result) {
  bool success = result == mojom::Result::OK;
  for (auto& observer : observers_) {
    observer.OnRecurringTipRemoved(this, success);
  }
}

void RewardsServiceImpl::RemoveRecurringTip(
    const std::string& publisher_key) {
  if (!Connected()) {
    return;
  }

  engine_->RemoveRecurringTip(
      publisher_key,
      base::BindOnce(&RewardsServiceImpl::OnRecurringTip, AsWeakPtr()));
}

void RewardsServiceImpl::OnSetPublisherExclude(const std::string& publisher_key,
                                               const bool exclude,
                                               const mojom::Result result) {
  if (result != mojom::Result::OK) {
    return;
  }

  for (auto& observer : observers_) {
    observer.OnExcludedSitesChanged(this, publisher_key, exclude);
  }
}

void RewardsServiceImpl::SetPublisherExclude(
    const std::string& publisher_key,
    bool exclude) {
  if (!Connected()) {
    return;
  }

  mojom::PublisherExclude status = exclude ? mojom::PublisherExclude::EXCLUDED
                                           : mojom::PublisherExclude::INCLUDED;

  engine_->SetPublisherExclude(
      publisher_key, status,
      base::BindOnce(&RewardsServiceImpl::OnSetPublisherExclude, AsWeakPtr(),
                     publisher_key, exclude));
}

RewardsNotificationService* RewardsServiceImpl::GetNotificationService() const {
  return notification_service_.get();
}

void RewardsServiceImpl::MaybeShowNotificationTipsPaid() {
  GetAutoContributeEnabled(base::BindOnce(
      &RewardsServiceImpl::ShowNotificationTipsPaid,
      AsWeakPtr()));
}

void RewardsServiceImpl::ShowNotificationTipsPaid(bool ac_enabled) {
  if (ac_enabled)
    return;

  RewardsNotificationService::RewardsNotificationArgs args;
  notification_service_->AddNotification(
      RewardsNotificationService::REWARDS_NOTIFICATION_TIPS_PROCESSED, args,
      "rewards_notification_tips_processed");
}

void RewardsServiceImpl::WriteDiagnosticLog(const std::string& file,
                                            const int line,
                                            const int verbose_level,
                                            const std::string& message) {
  if (engine_for_testing_ || resetting_rewards_) {
    return;
  }

  if (verbose_level > kDiagnosticLogMaxVerboseLevel ||
      verbose_level > persist_log_level_) {
    return;
  }

  diagnostic_log_->Write(
      message, base::Time::Now(), file, line, verbose_level,
      base::BindOnce(&RewardsServiceImpl::OnDiagnosticLogWritten, AsWeakPtr()));
}

void RewardsServiceImpl::OnDiagnosticLogWritten(const bool success) {
  DCHECK(success);
}

void RewardsServiceImpl::LoadDiagnosticLog(
      const int num_lines,
      LoadDiagnosticLogCallback callback) {
  diagnostic_log_->ReadLastNLines(
      num_lines, base::BindOnce(&RewardsServiceImpl::OnDiagnosticLogLoaded,
                                AsWeakPtr(), std::move(callback)));
}

void RewardsServiceImpl::OnDiagnosticLogLoaded(
    LoadDiagnosticLogCallback callback,
    const std::string& value) {
  std::move(callback).Run(value);
}

void RewardsServiceImpl::ClearDiagnosticLog(
    ClearDiagnosticLogCallback callback) {
  diagnostic_log_->Delete(std::move(callback));
}

void RewardsServiceImpl::Log(const std::string& file,
                             int32_t line,
                             int32_t verbose_level,
                             const std::string& message) {
  WriteDiagnosticLog(file, line, verbose_level, message);

  const int vlog_level =
      ::logging::GetVlogLevelHelper(file.c_str(), strlen(file.c_str()));
  if (verbose_level <= vlog_level) {
    ::logging::LogMessage(file.c_str(), line, -verbose_level).stream()
        << message;
  }
}

mojom::RewardsEngineOptionsPtr RewardsServiceImpl::HandleFlags(
    const RewardsFlags& flags) {
  auto options = mojom::RewardsEngineOptions::New();

  if (flags.environment) {
    switch (*flags.environment) {
      case RewardsFlags::Environment::kDevelopment:
        options->environment = mojom::Environment::kDevelopment;
        break;
      case RewardsFlags::Environment::kStaging:
        options->environment = mojom::Environment::kStaging;
        break;
      case RewardsFlags::Environment::kProduction:
        options->environment = mojom::Environment::kProduction;
        break;
    }
  } else {
    options->environment = GetDefaultServerEnvironment();
  }

  if (flags.reconcile_interval) {
    options->reconcile_interval = *flags.reconcile_interval;
  }

  if (flags.retry_interval) {
    options->retry_interval = *flags.retry_interval;
  }

  // The "persist-logs" command-line flag is deprecated and will be removed
  // in a future version. Use --enable-features=BraveRewardsVerboseLogging
  // instead.
  if (flags.persist_logs) {
    persist_log_level_ = kDiagnosticLogMaxVerboseLevel;
  }

  return options;
}

void RewardsServiceImpl::GetRewardsInternalsInfo(
    GetRewardsInternalsInfoCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), nullptr);
  }

  engine_->GetRewardsInternalsInfo(std::move(callback));
}

void RewardsServiceImpl::OnTip(const std::string& publisher_key,
                               double amount,
                               bool recurring,
                               OnTipCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), mojom::Result::FAILED);
  }

  RecordBackendP3AStats(/*delay_report*/ true);

  if (recurring) {
    return SaveRecurringTip(publisher_key, amount, std::move(callback));
  }

  engine_->OneTimeTip(publisher_key, amount, std::move(callback));
}

bool RewardsServiceImpl::Connected() const {
  return engine_.is_bound();
}

void RewardsServiceImpl::StartProcessForTesting(base::OnceClosure callback) {
  ready_->Post(FROM_HERE, std::move(callback));
  StartEngineProcessIfNecessary();
}

void RewardsServiceImpl::SetEngineEnvForTesting() {
  engine_for_testing_ = true;
}

void RewardsServiceImpl::SetEngineStateTargetVersionForTesting(int version) {
  engine_state_target_version_for_testing_ = version;
}

void RewardsServiceImpl::PrepareEngineEnvForTesting(
    mojom::RewardsEngineOptions& options) {
  if (!engine_for_testing_) {
    return;
  }

  options.is_testing = true;
  options.state_migration_target_version_for_testing =
      engine_state_target_version_for_testing_;
  options.retry_interval = 1;

  prefs_->SetInteger(prefs::kMinVisitTime, 1);
}

void RewardsServiceImpl::StartContributionsForTesting() {
  if (!Connected()) {
    return;
  }

  engine_->StartContributionsForTesting();  // IN-TEST
}

void RewardsServiceImpl::GetEnvironment(GetEnvironmentCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         GetDefaultServerEnvironment());
  }
  engine_->GetEnvironment(std::move(callback));
}

p3a::ConversionMonitor* RewardsServiceImpl::GetP3AConversionMonitor() {
  return &conversion_monitor_;
}

void RewardsServiceImpl::OnRewardsPageShown() {
  p3a::RecordRewardsPageViews(prefs_, true);
}

void RewardsServiceImpl::PublisherListNormalized(
    std::vector<mojom::PublisherInfoPtr> list) {
  for (auto& observer : observers_) {
    std::vector<mojom::PublisherInfoPtr> new_list;
    for (const auto& publisher : list) {
      if (publisher->percent >= 1) {
        new_list.push_back(publisher->Clone());
      }
    }
    observer.OnPublisherListNormalized(this, std::move(new_list));
  }
}

void RewardsServiceImpl::OnPublisherRegistryUpdated() {
  for (auto& observer : observers_) {
    observer.OnPublisherRegistryUpdated();
  }
}

void RewardsServiceImpl::OnPublisherUpdated(const std::string& publisher_id) {
  for (auto& observer : observers_) {
    observer.OnPublisherUpdated(publisher_id);
  }
}

void RewardsServiceImpl::RefreshPublisher(
    const std::string& publisher_key,
    RefreshPublisherCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         mojom::PublisherStatus::NOT_VERIFIED, "");
  }
  engine_->RefreshPublisher(
      publisher_key,
      base::BindOnce(&RewardsServiceImpl::OnRefreshPublisher, AsWeakPtr(),
                     std::move(callback), publisher_key));
}

void RewardsServiceImpl::OnRefreshPublisher(RefreshPublisherCallback callback,
                                            const std::string& publisher_key,
                                            mojom::PublisherStatus status) {
  std::move(callback).Run(status, publisher_key);
}

const RewardsNotificationService::RewardsNotificationsMap&
RewardsServiceImpl::GetAllNotifications() {
  return notification_service_->GetAllNotifications();
}

void RewardsServiceImpl::GetShareURL(
      const base::flat_map<std::string, std::string>& args,
      GetShareURLCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), "");
  }

  engine_->GetShareURL(args, std::move(callback));
}

void RewardsServiceImpl::FetchBalance(FetchBalanceCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), nullptr);
  }

  engine_->FetchBalance(std::move(callback));
}

void RewardsServiceImpl::GetLegacyWallet(GetLegacyWalletCallback callback) {
  const auto& dict = prefs_->GetDict(prefs::kExternalWallets);

  std::string json;
  for (auto it = dict.begin(); it != dict.end(); ++it) {
    base::JSONWriter::Write(std::move(it->second), &json);
  }

  std::move(callback).Run(std::move(json));
}

void RewardsServiceImpl::GetExternalWallet(GetExternalWalletCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), nullptr);
  }

  engine_->GetExternalWallet(std::move(callback));
}

void RewardsServiceImpl::BeginExternalWalletLogin(
    const std::string& wallet_type,
    BeginExternalWalletLoginCallback callback) {
  if (!Connected() || !IsValidWalletType(wallet_type)) {
    return DeferCallback(FROM_HERE, std::move(callback), nullptr);
  }

  engine_->BeginExternalWalletLogin(
      wallet_type,
      base::BindOnce(&RewardsServiceImpl::OnExternalWalletLoginStarted,
                     AsWeakPtr(), std::move(callback)));
}

void RewardsServiceImpl::OnExternalWalletLoginStarted(
    BeginExternalWalletLoginCallback callback,
    mojom::ExternalWalletLoginParamsPtr params) {
  if (!params || params->cookies.empty()) {
    std::move(callback).Run(std::move(params));
    return;
  }

  GURL url(params->url);
  CHECK(url.is_valid());

  // For each cookie, we'll need to set the cookie and provide a callback for
  // when the cookie has been set. This barrier callback ensures that once all
  // "set cookie" callbacks have executed, the provided callback will be run.
  base::RepeatingClosure set_cookie_callback = base::BarrierClosure(
      params->cookies.size(),
      base::BindOnce(std::move(callback), params->Clone()));

  auto on_cookie_set = [](base::RepeatingClosure set_cookie_callback,
                          net::CookieAccessResult result) {
    set_cookie_callback.Run();
  };

  auto* cookie_manager =
      storage_partition_->GetCookieManagerForBrowserProcess();

  net::CookieOptions options;
  options.set_include_httponly();
  options.set_same_site_cookie_context(
      net::CookieOptions::SameSiteCookieContext::MakeInclusive());

  base::Time now = base::Time::Now();
  base::Time expiration_time = now + base::Minutes(10);

  for (auto& [key, value] : params->cookies) {
    auto cookie = net::CanonicalCookie::CreateSanitizedCookie(
        url, key, value, /*domain=*/"", url.path(),
        /*creation_time=*/now, expiration_time, /*last_access_time=*/now,
        /*secure=*/true, /*httponly=*/false, net::CookieSameSite::STRICT_MODE,
        net::COOKIE_PRIORITY_DEFAULT, /*partition_key=*/std::nullopt,
        /*status=*/nullptr);

    cookie_manager->SetCanonicalCookie(
        *cookie,
        net::cookie_util::SimulatedCookieSource(*cookie, url::kHttpsScheme),
        options, base::BindOnce(on_cookie_set, set_cookie_callback));
  }
}

void RewardsServiceImpl::ConnectExternalWallet(
    const std::string& path,
    const std::string& query,
    ConnectExternalWalletCallback callback) {
  const auto path_items = base::SplitString(path, "/", base::TRIM_WHITESPACE,
                                            base::SPLIT_WANT_NONEMPTY);
  if (path_items.empty()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         mojom::ConnectExternalWalletResult::kUnexpected);
  }

  const std::string wallet_type = path_items.at(0);
  base::flat_map<std::string, std::string> query_parameters;

  const auto url = GURL("brave:/" + path + query);
  for (net::QueryIterator it(url); !it.IsAtEnd(); it.Advance()) {
    query_parameters[static_cast<std::string>(it.GetKey())] =
        it.GetUnescapedValue();
  }

  ConnectExternalWallet(wallet_type, query_parameters, std::move(callback));
}

void RewardsServiceImpl::ConnectExternalWallet(
    const std::string& provider,
    const base::flat_map<std::string, std::string>& args,
    ConnectExternalWalletCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         mojom::ConnectExternalWalletResult::kUnexpected);
  }

  auto inner_callback = base::BindOnce(
      [](base::WeakPtr<RewardsServiceImpl> self,
         ConnectExternalWalletCallback callback,
         mojom::ConnectExternalWalletResult result) {
        std::move(callback).Run(result);
        self->RecordBackendP3AStats();
      },
      AsWeakPtr(), std::move(callback));

  engine_->ConnectExternalWallet(provider, args, std::move(inner_callback));
}

void RewardsServiceImpl::ShowNotification(const std::string& type,
                                          const std::vector<std::string>& args,
                                          ShowNotificationCallback callback) {
  if (type.empty()) {
    std::move(callback).Run(mojom::Result::FAILED);
    return;
  }

  RewardsNotificationService::RewardsNotificationArgs notification_args;
  notification_args.push_back(type);
  notification_args.insert(notification_args.end(), args.begin(), args.end());
  notification_service_->AddNotification(
      RewardsNotificationService::REWARDS_NOTIFICATION_GENERAL,
      notification_args, "rewards_notification_general_" + type);

  std::move(callback).Run(mojom::Result::OK);
}

void RewardsServiceImpl::RecordBackendP3AStats(bool delay_report) {
  if (!Connected()) {
    return;
  }

  p3a::RecordRewardsPageViews(prefs_, false);

  GetExternalWallet(
      base::BindOnce(&RewardsServiceImpl::OnRecordBackendP3AExternalWallet,
                     AsWeakPtr(), delay_report, false));
}

void RewardsServiceImpl::OnP3ADailyTimer() {
  RecordBackendP3AStats();
  p3a_daily_timer_.Start(
      FROM_HERE, base::Time::Now() + kP3ADailyReportInterval,
      base::BindOnce(&RewardsServiceImpl::OnP3ADailyTimer, AsWeakPtr()));
}

void RewardsServiceImpl::OnRecordBackendP3AExternalWallet(
    bool delay_report,
    bool search_result_optin_changed,
    mojom::ExternalWalletPtr wallet) {
  if (!Connected()) {
    return;
  }

  if (!wallet || wallet->status != mojom::WalletStatus::kConnected) {
    // Do not report "tips sent" and "auto-contribute" enabled metrics if user
    // does not have a custodial account linked.
    p3a::RecordNoWalletCreatedForAllMetrics();
    return;
  }

  if (search_result_optin_changed) {
    p3a::RecordSearchResultAdsOptinChange(prefs_);
  } else if (delay_report) {
    // Use delay to ensure tips are confirmed when counting.
    p3a_tip_report_timer_.Start(
        FROM_HERE, kP3ATipReportDelay,
        base::BindOnce(&RewardsServiceImpl::GetAllContributionsForP3A,
                       AsWeakPtr()));
  } else {
    GetAllContributionsForP3A();
  }
}

void RewardsServiceImpl::GetAllContributionsForP3A() {
  if (!Connected()) {
    return;
  }

  engine_->GetAllContributions(base::BindOnce(
      &RewardsServiceImpl::OnRecordBackendP3AStatsContributions, AsWeakPtr()));
  GetRecurringTips(base::BindOnce(
      &RewardsServiceImpl::OnRecordBackendP3AStatsRecurringTips, AsWeakPtr()));
}

void RewardsServiceImpl::OnRecordBackendP3AStatsContributions(
    std::vector<mojom::ContributionInfoPtr> list) {
  size_t tip_count = 0;

  base::Time now = base::Time::Now();
  for (const auto& contribution : list) {
    if (contribution->type == mojom::RewardsType::ONE_TIME_TIP ||
        contribution->type == mojom::RewardsType::RECURRING_TIP) {
      if (now - base::Time::FromTimeT(contribution->created_at) <
          kP3AMonthlyReportingPeriod) {
        tip_count++;
      }
    }
  }

  p3a::RecordTipsSent(tip_count);

  GetAutoContributeEnabled(base::BindOnce(
      &RewardsServiceImpl::OnRecordBackendP3AStatsAC, AsWeakPtr()));
}

void RewardsServiceImpl::OnRecordBackendP3AStatsRecurringTips(
    std::vector<mojom::PublisherInfoPtr> list) {
  p3a::RecordRecurringTipConfigured(!list.empty());
}

void RewardsServiceImpl::OnRecordBackendP3AStatsAC(bool ac_enabled) {
  p3a::RecordAutoContributionsState(ac_enabled);
}

mojom::Environment RewardsServiceImpl::GetDefaultServerEnvironment() {
  mojom::Environment environment = mojom::Environment::kStaging;
#if defined(OFFICIAL_BUILD) && BUILDFLAG(IS_ANDROID)
  environment = GetDefaultServerEnvironmentForAndroid();
#elif defined(OFFICIAL_BUILD)
  environment = mojom::Environment::kProduction;
#endif
  return environment;
}

#if BUILDFLAG(IS_ANDROID)
mojom::Environment RewardsServiceImpl::GetDefaultServerEnvironmentForAndroid() {
  auto result = mojom::Environment::kProduction;
  bool use_staging = false;
  if (prefs_) {
    use_staging = prefs_->GetBoolean(prefs::kUseRewardsStagingServer);
  }

  if (use_staging) {
    result = mojom::Environment::kStaging;
  }

  return result;
}
#endif

mojom::ClientInfoPtr GetDesktopClientInfo() {
  auto info = mojom::ClientInfo::New();
  info->platform = mojom::Platform::DESKTOP;
#if BUILDFLAG(IS_MAC)
  info->os = mojom::OperatingSystem::MACOS;
#elif BUILDFLAG(IS_WIN)
  info->os = mojom::OperatingSystem::WINDOWS;
#elif BUILDFLAG(IS_LINUX)
  info->os = mojom::OperatingSystem::LINUX;
#else
  info->os = mojom::OperatingSystem::UNDEFINED;
#endif

  return info;
}

void RewardsServiceImpl::GetClientInfo(GetClientInfoCallback callback) {
#if BUILDFLAG(IS_ANDROID)
  std::move(callback).Run(android_util::GetAndroidClientInfo());
#else
  std::move(callback).Run(GetDesktopClientInfo());
#endif
}

void RewardsServiceImpl::ReconcileStampReset() {
  for (auto& observer : observers_) {
    observer.ReconcileStampReset();
  }
}

void RewardsServiceImpl::RunDBTransaction(mojom::DBTransactionPtr transaction,
                                          RunDBTransactionCallback callback) {
  DCHECK(rewards_database_);
  rewards_database_.AsyncCall(&internal::RewardsDatabase::RunTransaction)
      .WithArgs(std::move(transaction))
      .Then(base::BindOnce(&RewardsServiceImpl::OnRunDBTransaction, AsWeakPtr(),
                           std::move(callback)));
}

void RewardsServiceImpl::OnRunDBTransaction(
    RunDBTransactionCallback callback,
    mojom::DBCommandResponsePtr response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::move(callback).Run(std::move(response));
}

void RewardsServiceImpl::ForTestingSetTestResponseCallback(
    const GetTestResponseCallback& callback) {
  test_response_callback_ = callback;
}

void RewardsServiceImpl::GetAllContributions(
    GetAllContributionsCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         std::vector<mojom::ContributionInfoPtr>());
  }

  engine_->GetAllContributions(std::move(callback));
}

void RewardsServiceImpl::ClearAllNotifications() {
  notification_service_->DeleteAllNotifications(false);
}

void RewardsServiceImpl::CompleteReset(SuccessCallback callback) {
  resetting_rewards_ = true;

  notification_service_->DeleteAllNotifications(true);

  auto stop_callback =
      base::BindOnce(&RewardsServiceImpl::OnStopEngineForCompleteReset,
                     AsWeakPtr(), std::move(callback));
  StopEngine(std::move(stop_callback));
}

void RewardsServiceImpl::OnFilesDeletedForCompleteReset(
    SuccessCallback callback,
    const bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  resetting_rewards_ = false;
  for (auto& observer : observers_) {
    observer.OnCompleteReset(success);
  }
  std::move(callback).Run(success);
}

void RewardsServiceImpl::ExternalWalletConnected() {
  // When the user connects an external wallet, turn off search result ads since
  // users cannot earn BAT for them yet. The user can turn them on manually.
  prefs_->SetBoolean(brave_ads::prefs::kOptedInToSearchResultAds, false);
  for (auto& observer : observers_) {
    observer.OnExternalWalletConnected();
  }
}

void RewardsServiceImpl::ExternalWalletLoggedOut() {
  for (auto& observer : observers_) {
    observer.OnExternalWalletLoggedOut();
  }
}

void RewardsServiceImpl::ExternalWalletReconnected() {
  for (auto& observer : observers_) {
    observer.OnExternalWalletReconnected();
  }
}

void RewardsServiceImpl::ExternalWalletDisconnected() {
  for (auto& observer : observers_) {
    observer.OnExternalWalletDisconnected();
  }
}

void RewardsServiceImpl::DeleteLog(DeleteLogCallback callback) {
  diagnostic_log_->Delete(
      base::BindOnce(&RewardsServiceImpl::OnDiagnosticLogDeleted, AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnDiagnosticLogDeleted(DeleteLogCallback callback,
                                                bool success) {
  const auto result = success ? mojom::Result::OK : mojom::Result::FAILED;
  std::move(callback).Run(result);
}

void RewardsServiceImpl::GetEventLogs(GetEventLogsCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         std::vector<mojom::EventLogPtr>());
  }

  engine_->GetEventLogs(std::move(callback));
}

void RewardsServiceImpl::EncryptString(const std::string& value,
                                       EncryptStringCallback callback) {
  std::string encrypted;
  if (OSCrypt::EncryptString(value, &encrypted)) {
    return std::move(callback).Run(std::move(encrypted));
  }

  std::move(callback).Run(std::nullopt);
}

void RewardsServiceImpl::DecryptString(const std::string& value,
                                       DecryptStringCallback callback) {
  std::string decrypted;
  if (OSCrypt::DecryptString(value, &decrypted)) {
    return std::move(callback).Run(std::move(decrypted));
  }

  std::move(callback).Run(std::nullopt);
}

void RewardsServiceImpl::GetRewardsWallet(GetRewardsWalletCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), nullptr);
  }

  engine_->GetRewardsWallet(std::move(callback));
}

base::WeakPtr<RewardsServiceImpl> RewardsServiceImpl::AsWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

bool RewardsServiceImpl::IsBitFlyerCountry() const {
  return base::Contains(kBitflyerCountries, GetCountryCode());
}

bool RewardsServiceImpl::IsValidWalletType(
    const std::string& wallet_type) const {
  for (auto& provider : GetExternalWalletProviders()) {
    if (wallet_type == provider)
      return true;
  }
  return false;
}

std::string RewardsServiceImpl::GetExternalWalletType() const {
  if (IsBitFlyerCountry()) {
    return internal::constant::kWalletBitflyer;
  }

  const std::string type = prefs_->GetString(prefs::kExternalWalletType);

  if (IsValidWalletType(type)) {
    return type;
  }

  return internal::constant::kWalletUphold;
}

}  // namespace brave_rewards
