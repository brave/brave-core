/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_service_impl.h"

#include <stdint.h>
#include <string>

#include <algorithm>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/containers/contains.h"
#include "base/feature_list.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/functional/bind.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_reader.h"
#include "base/json/json_string_value_serializer.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/ranges/algorithm.h"
#include "base/strings/escape.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/public/ledger_database.h"
#include "brave/browser/ui/webui/brave_rewards_source.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_rewards/browser/android_util.h"
#include "brave/components/brave_rewards/browser/diagnostic_log.h"
#include "brave/components/brave_rewards/browser/logging.h"
#include "brave/components/brave_rewards/browser/publisher_utils.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_p3a.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/service_sandbox_type.h"
#include "brave/components/brave_rewards/browser/static_values.h"
#include "brave/components/brave_rewards/common/buildflags/buildflags.h"
#include "brave/components/brave_rewards/common/features.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "brave/components/services/bat_ledger/public/cpp/ledger_client_mojo_bridge.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service_factory.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/country_codes/country_codes.h"
#include "components/favicon/core/favicon_service.h"
#include "components/favicon_base/favicon_types.h"
#include "components/grit/brave_components_resources.h"
#include "components/os_crypt/os_crypt.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/service_process_host.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/url_data_source.h"
#include "net/base/url_util.h"
#include "net/http/http_status_code.h"
#include "services/data_decoder/public/cpp/json_sanitizer.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "third_party/icu/source/common/unicode/locid.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"
#include "url/url_canon_stdstring.h"
#include "url/url_util.h"

namespace brave_rewards {

static const unsigned int kRetriesCountOnNetworkChange = 1;

namespace {

constexpr int kDiagnosticLogMaxVerboseLevel = 6;
constexpr int kDiagnosticLogKeepNumLines = 20000;
constexpr int kDiagnosticLogMaxFileSize = 10 * (1024 * 1024);
constexpr char pref_prefix[] = "brave.rewards";

std::string URLMethodToRequestType(ledger::mojom::UrlMethod method) {
  switch (method) {
    case ledger::mojom::UrlMethod::GET:
      return "GET";
    case ledger::mojom::UrlMethod::POST:
      return "POST";
    case ledger::mojom::UrlMethod::PUT:
      return "PUT";
    case ledger::mojom::UrlMethod::PATCH:
      return "PATCH";
    case ledger::mojom::UrlMethod::DEL:
      return "DELETE";
    default:
      NOTREACHED();
      return "GET";
  }
}

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
    VLOG(0) << "Cannot deserialize ledger state, error code: " << error_code
        << " message: " << error_message;
    return result;
  }

  if (!value->is_dict()) {
    VLOG(0) << "Corrupted ledger state.";
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

ledger::mojom::InlineTipsPlatforms ConvertInlineTipStringToPlatform(
    const std::string& key) {
  if (key == "reddit") {
    return ledger::mojom::InlineTipsPlatforms::REDDIT;
  }
  if (key == "twitter") {
    return ledger::mojom::InlineTipsPlatforms::TWITTER;
  }
  if (key == "github") {
    return ledger::mojom::InlineTipsPlatforms::GITHUB;
  }

  NOTREACHED();
  return ledger::mojom::InlineTipsPlatforms::TWITTER;
}

bool IsAdsOrAutoContributeEnabled(Profile* profile) {
  DCHECK(profile);
  auto* prefs = profile->GetPrefs();
  return prefs->GetBoolean(prefs::kAutoContributeEnabled) ||
         prefs->GetBoolean(ads::prefs::kEnabled);
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
const base::FilePath::StringType kLedger_state(L"ledger_state");
const base::FilePath::StringType kPublisher_state(L"publisher_state");
const base::FilePath::StringType kPublisher_info_db(L"publisher_info_db");
const base::FilePath::StringType kPublishers_list(L"publishers_list");
#else
const base::FilePath::StringType kDiagnosticLogPath("Rewards.log");
const base::FilePath::StringType kLedger_state("ledger_state");
const base::FilePath::StringType kPublisher_state("publisher_state");
const base::FilePath::StringType kPublisher_info_db("publisher_info_db");
const base::FilePath::StringType kPublishers_list("publishers_list");
#endif

#if BUILDFLAG(ENABLE_GREASELION)
RewardsServiceImpl::RewardsServiceImpl(
    Profile* profile,
    greaselion::GreaselionService* greaselion_service)
#else
RewardsServiceImpl::RewardsServiceImpl(Profile* profile)
#endif
    : profile_(profile),
#if BUILDFLAG(ENABLE_GREASELION)
      greaselion_service_(greaselion_service),
#endif
      bat_ledger_client_receiver_(new bat_ledger::LedgerClientMojoBridge(this)),
      file_task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      ledger_state_path_(profile_->GetPath().Append(kLedger_state)),
      publisher_state_path_(profile_->GetPath().Append(kPublisher_state)),
      publisher_info_db_path_(profile->GetPath().Append(kPublisher_info_db)),
      publisher_list_path_(profile->GetPath().Append(kPublishers_list)),
      diagnostic_log_(
          new DiagnosticLog(profile_->GetPath().Append(kDiagnosticLogPath),
                            kDiagnosticLogMaxFileSize,
                            kDiagnosticLogKeepNumLines)),
      notification_service_(new RewardsNotificationServiceImpl(profile)),
      next_timer_id_(0) {
  // Set up the rewards data source
  content::URLDataSource::Add(profile_,
                              std::make_unique<BraveRewardsSource>(profile_));
  ready_ = std::make_unique<base::OneShotEvent>();

  if (base::FeatureList::IsEnabled(features::kVerboseLoggingFeature))
    persist_log_level_ = kDiagnosticLogMaxVerboseLevel;

#if BUILDFLAG(ENABLE_GREASELION)
  if (greaselion_service_) {
    greaselion_service_->AddObserver(this);
  }
#endif
}

RewardsServiceImpl::~RewardsServiceImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

#if BUILDFLAG(ENABLE_GREASELION)
  if (greaselion_service_) {
    greaselion_service_->RemoveObserver(this);
  }
#endif

  StopNotificationTimers();
}

void RewardsServiceImpl::ConnectionClosed() {
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&RewardsServiceImpl::StartLedgerProcessIfNecessary,
                     AsWeakPtr()),
      base::Seconds(1));
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
  p3a::RecordAdsEnabledDuration(
      profile_->GetPrefs(),
      profile_->GetPrefs()->GetBoolean(ads::prefs::kEnabled));
}

void RewardsServiceImpl::InitPrefChangeRegistrar() {
  profile_pref_change_registrar_.Init(profile_->GetPrefs());
  profile_pref_change_registrar_.Add(
      prefs::kShowButton,
      base::BindRepeating(&RewardsServiceImpl::OnPreferenceChanged,
                          base::Unretained(this)));
  profile_pref_change_registrar_.Add(
      prefs::kInlineTipTwitterEnabled,
      base::BindRepeating(&RewardsServiceImpl::OnPreferenceChanged,
                          base::Unretained(this)));
  profile_pref_change_registrar_.Add(
      prefs::kInlineTipRedditEnabled,
      base::BindRepeating(&RewardsServiceImpl::OnPreferenceChanged,
                          base::Unretained(this)));
  profile_pref_change_registrar_.Add(
      prefs::kInlineTipGithubEnabled,
      base::BindRepeating(&RewardsServiceImpl::OnPreferenceChanged,
                          base::Unretained(this)));
  profile_pref_change_registrar_.Add(
      prefs::kAutoContributeEnabled,
      base::BindRepeating(&RewardsServiceImpl::OnPreferenceChanged,
                          base::Unretained(this)));
  profile_pref_change_registrar_.Add(
      ads::prefs::kEnabled,
      base::BindRepeating(&RewardsServiceImpl::OnPreferenceChanged,
                          base::Unretained(this)));
}

void RewardsServiceImpl::OnPreferenceChanged(const std::string& key) {
  if (key == prefs::kAutoContributeEnabled) {
    if (profile_->GetPrefs()->GetBoolean(prefs::kAutoContributeEnabled)) {
      StartLedgerProcessIfNecessary();
    } else {
      // Just record the disabled state.
      p3a::RecordAutoContributionsState(
          p3a::AutoContributionsState::kWalletCreatedAutoContributeOff, 0);
    }
  }

  if (key == prefs::kAutoContributeEnabled || key == ads::prefs::kEnabled) {
    if (IsAdsOrAutoContributeEnabled(profile_)) {
      RecordBackendP3AStats();
    } else {
      p3a::RecordRewardsDisabledForSomeMetrics();
    }
    p3a::RecordAdsEnabledDuration(
        profile_->GetPrefs(),
        profile_->GetPrefs()->GetBoolean(ads::prefs::kEnabled));
  }

  if (key == ads::prefs::kEnabled) {
    p3a::UpdateAdsStateOnPreferenceChange(profile_->GetPrefs(),
                                          ads::prefs::kEnabled);

    bool ads_enabled = profile_->GetPrefs()->GetBoolean(ads::prefs::kEnabled);

#if BUILDFLAG(ENABLE_GREASELION)
    if (greaselion_service_) {
      greaselion_service_->SetFeatureEnabled(greaselion::ADS, ads_enabled);
    }
#endif

    for (auto& observer : observers_) {
      observer.OnAdsEnabled(this, ads_enabled);
    }
  }
}

void RewardsServiceImpl::CheckPreferences() {
  auto* prefs = profile_->GetPrefs();

  if (prefs->GetBoolean(prefs::kAutoContributeEnabled) ||
      prefs->GetBoolean(ads::prefs::kEnabled)) {
    // If the user has enabled Ads or AC, but the "enabled" pref is missing, set
    // the "enabled" pref to true.
    if (!prefs->GetUserPrefValue(prefs::kEnabled)) {
      prefs->SetBoolean(prefs::kEnabled, true);
    }
  }

  if (prefs->GetUserPrefValue(prefs::kEnabled)) {
    // If the "enabled" pref is set, then start the background Rewards
    // utility process.
    StartLedgerProcessIfNecessary();
  }
}

void RewardsServiceImpl::StartLedgerProcessIfNecessary() {
  if (Connected()) {
    BLOG(1, "Ledger process is already running");
    return;
  }

  ledger_database_ = base::SequenceBound<ledger::LedgerDatabase>(
      file_task_runner_, publisher_info_db_path_);

  BLOG(1, "Starting ledger process");

  if (!bat_ledger_service_.is_bound()) {
    content::ServiceProcessHost::Launch(
        bat_ledger_service_.BindNewPipeAndPassReceiver(),
        content::ServiceProcessHost::Options()
            .WithDisplayName(IDS_UTILITY_PROCESS_LEDGER_NAME)
            .Pass());

    bat_ledger_service_.set_disconnect_handler(
        base::BindOnce(&RewardsServiceImpl::ConnectionClosed, AsWeakPtr()));
  }

  // Environment
  SetEnvironment(GetDefaultServerEnvironment());

  SetDebug(false);

  HandleFlags(RewardsFlags::ForCurrentProcess());

  bat_ledger_service_->Create(
      bat_ledger_client_receiver_.BindNewEndpointAndPassRemote(),
      bat_ledger_.BindNewEndpointAndPassReceiver(),
      base::BindOnce(&RewardsServiceImpl::OnLedgerCreated, AsWeakPtr()));
}

void RewardsServiceImpl::OnLedgerCreated() {
  if (!Connected()) {
    BLOG(0, "Ledger instance could not be created");
    return;
  }

  PrepareLedgerEnvForTesting();

  bat_ledger_->Initialize(
      false,
      base::BindOnce(&RewardsServiceImpl::OnLedgerInitialized, AsWeakPtr()));
}

void RewardsServiceImpl::OnResult(ledger::LegacyResultCallback callback,
                                  ledger::mojom::Result result) {
  callback(result);
}

void RewardsServiceImpl::CreateRewardsWallet(
    const std::string& country,
    CreateRewardsWalletCallback callback) {
  using ledger::mojom::CreateRewardsWalletResult;

  auto on_start = [](base::WeakPtr<RewardsServiceImpl> self,
                     std::string country,
                     CreateRewardsWalletCallback callback) {
    if (!self) {
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

      auto* prefs = self->profile_->GetPrefs();
      prefs->SetString(prefs::kDeclaredGeo, country);

      // Record in which environment the wallet was created (for display on the
      // rewards internals page).
      auto on_get_environment = [](base::WeakPtr<RewardsServiceImpl> self,
                                   ledger::mojom::Environment environment) {
        if (self) {
          self->profile_->GetPrefs()->SetInteger(
              prefs::kWalletCreationEnvironment, static_cast<int>(environment));
        }
      };

      self->GetEnvironment(base::BindOnce(on_get_environment, self));

      // After successfully creating a Rewards wallet for the first time,
      // automatically enable Ads and AC.
      if (!prefs->GetBoolean(prefs::kEnabled)) {
        prefs->SetBoolean(prefs::kEnabled, true);
        prefs->SetString(prefs::kUserVersion, prefs::kCurrentUserVersion);
        prefs->SetBoolean(ads::prefs::kEnabled, true);

        // Fetch the user's balance before turning on AC. We don't want to
        // automatically turn on AC if for some reason the user has a current
        // balance, as this could result in unintentional BAT transfers.
        auto on_balance = [](base::WeakPtr<RewardsServiceImpl> self,
                             ledger::mojom::Result result,
                             ledger::mojom::BalancePtr balance) {
          if (self && balance && balance->total == 0) {
            self->SetAutoContributeEnabled(true);
          }
        };
        self->FetchBalance(base::BindOnce(on_balance, self));
      }

      // Notify observers that the Rewards wallet has been created.
      for (auto& observer : self->observers_) {
        observer.OnRewardsWalletUpdated();
      }

      std::move(callback).Run(CreateRewardsWalletResult::kSuccess);
    };

    self->bat_ledger_->CreateRewardsWallet(
        country,
        base::BindOnce(on_created, self, country, std::move(callback)));
  };

  ready_->Post(FROM_HERE, base::BindOnce(on_start, AsWeakPtr(), country,
                                         std::move(callback)));
  StartLedgerProcessIfNecessary();
}

void RewardsServiceImpl::GetUserType(
    base::OnceCallback<void(ledger::mojom::UserType)> callback) {
  using ledger::mojom::UserType;

  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         UserType::kUnconnected);
  }

  auto on_external_wallet = [](base::WeakPtr<RewardsServiceImpl> self,
                               base::OnceCallback<void(UserType)> callback,
                               GetExternalWalletResult result) {
    if (!self) {
      std::move(callback).Run(UserType::kUnconnected);
      return;
    }

    auto wallet = std::move(result).value_or(nullptr);
    if (!wallet ||
        wallet->status != ledger::mojom::WalletStatus::kNotConnected) {
      std::move(callback).Run(UserType::kConnected);
      return;
    }

    auto* prefs = self->profile_->GetPrefs();
    base::Version version(prefs->GetString(prefs::kUserVersion));
    if (!version.IsValid()) {
      version = base::Version({1});
    }

    if (!prefs->GetBoolean(prefs::kParametersVBatExpired) &&
        version.CompareTo(base::Version({2, 5})) < 0) {
      std::move(callback).Run(UserType::kLegacyUnconnected);
      return;
    }

    std::move(callback).Run(UserType::kUnconnected);
  };

  GetExternalWallet(
      base::BindOnce(on_external_wallet, AsWeakPtr(), std::move(callback)));
}

std::string RewardsServiceImpl::GetCountryCode() const {
  auto* prefs = profile_->GetPrefs();
  std::string country = prefs->GetString(prefs::kDeclaredGeo);
  if (!country.empty()) {
    return country;
  }

  int32_t country_id = country_id_;
  if (!country_id) {
    country_id = country_codes::GetCountryIDFromPrefs(prefs);
  }
  if (country_id < 0) {
    return "";
  }
  return {base::ToUpperASCII(static_cast<char>(country_id >> 8)),
          base::ToUpperASCII(static_cast<char>(0xFF & country_id))};
}

void RewardsServiceImpl::GetAvailableCountries(
    GetAvailableCountriesCallback callback) const {
  static const std::vector<std::string> kISOCountries = GetISOCountries();

  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), kISOCountries);
  }

  auto on_external_wallet = [](GetAvailableCountriesCallback callback,
                               GetExternalWalletResult result) {
    auto wallet = std::move(result).value_or(nullptr);
    // If the user is not currently connected to any wallet provider, then all
    // ISO country codes are available.
    if (!wallet ||
        wallet->status == ledger::mojom::WalletStatus::kNotConnected) {
      return std::move(callback).Run(kISOCountries);
    }

    // If the user is currently connected to a bitFlyer wallet, then the only
    // available countries are |kBitflyerCountries|.
    if (wallet->type == ledger::constant::kWalletBitflyer) {
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

  bat_ledger_->GetExternalWallet(
      GetExternalWalletType(),
      base::BindOnce(on_external_wallet, std::move(callback)));
}

void RewardsServiceImpl::GetActivityInfoList(
    uint32_t start,
    uint32_t limit,
    ledger::mojom::ActivityInfoFilterPtr filter,
    GetPublisherInfoListCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         std::vector<ledger::mojom::PublisherInfoPtr>());
  }

  bat_ledger_->GetActivityInfoList(
      start, limit, std::move(filter),
      base::BindOnce(&RewardsServiceImpl::OnGetPublisherInfoList, AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::GetPublishersVisitedCount(
    base::OnceCallback<void(int)> callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), 0);
  }
  bat_ledger_->GetPublishersVisitedCount(std::move(callback));
}

void RewardsServiceImpl::GetExcludedList(
    GetPublisherInfoListCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         std::vector<ledger::mojom::PublisherInfoPtr>());
  }

  bat_ledger_->GetExcludedList(
      base::BindOnce(&RewardsServiceImpl::OnGetPublisherInfoList, AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnGetPublisherInfoList(
    GetPublisherInfoListCallback callback,
    std::vector<ledger::mojom::PublisherInfoPtr> list) {
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

  ledger::mojom::VisitDataPtr data = ledger::mojom::VisitData::New();
  data->domain = *publisher_domain;
  data->name = *publisher_domain;
  data->path = url.path();
  data->tab_id = tab_id.id();
  data->url = url.scheme() + "://" + *publisher_domain + "/";
  bat_ledger_->OnLoad(std::move(data), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnUnload(SessionID tab_id) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->OnUnload(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnShow(SessionID tab_id) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->OnShow(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnHide(SessionID tab_id) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->OnHide(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnForeground(SessionID tab_id) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->OnForeground(tab_id.id(), GetCurrentTimestamp());
}

void RewardsServiceImpl::OnBackground(SessionID tab_id) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->OnBackground(tab_id.id(), GetCurrentTimestamp());
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

  ledger::mojom::VisitDataPtr data = ledger::mojom::VisitData::New();
  data->path = url.spec();
  data->tab_id = tab_id.id();

  bat_ledger_->OnXHRLoad(tab_id.id(),
                         url.spec(),
                         parts,
                         first_party_url.spec(),
                         referrer.spec(),
                         std::move(data));
}

void RewardsServiceImpl::OnRestorePublishers(
    const ledger::mojom::Result result) {
  if (result != ledger::mojom::Result::LEDGER_OK) {
    return;
  }

  for (auto& observer : observers_) {
    observer.OnExcludedSitesChanged(
        this, "-1", static_cast<int>(ledger::mojom::PublisherExclude::ALL));
  }
}

void RewardsServiceImpl::RestorePublishers() {
  if (!Connected()) {
    return;
  }

  bat_ledger_->RestorePublishers(
    base::BindOnce(&RewardsServiceImpl::OnRestorePublishers,
                   AsWeakPtr()));
}

std::string RewardsServiceImpl::URIEncode(const std::string& value) {
  return base::EscapeQueryParamValue(value, false);
}

void RewardsServiceImpl::Shutdown() {
  RemoveObserver(notification_service_.get());

  if (extension_observer_) {
    RemoveObserver(extension_observer_.get());
  }

  BitmapFetcherService* image_service =
      BitmapFetcherServiceFactory::GetForBrowserContext(profile_);
  if (image_service) {
    for (auto mapping : current_media_fetchers_) {
      image_service->CancelRequest(mapping.second);
    }
  }

  bat_ledger_client_receiver_.reset();
  url_loaders_.clear();

  bat_ledger_.reset();
  RewardsService::Shutdown();
}

void RewardsServiceImpl::OnLedgerInitialized(ledger::mojom::Result result) {
  if (result == ledger::mojom::Result::LEDGER_OK) {
    StartNotificationTimers();
  }

  if (!ready_->is_signaled()) {
    ready_->Signal();
  }

  if (IsAdsOrAutoContributeEnabled(profile_)) {
    RecordBackendP3AStats();
  } else {
    p3a::RecordRewardsDisabledForSomeMetrics();
  }

  GetRewardsWallet(base::BindOnce(&RewardsServiceImpl::OnGetRewardsWalletForP3A,
                                  AsWeakPtr()));

  for (auto& observer : observers_) {
    observer.OnRewardsInitialized(this);
  }
}

void RewardsServiceImpl::OnGetRewardsWalletForP3A(
    ledger::mojom::RewardsWalletPtr wallet) {
  if (!wallet) {
    p3a::RecordNoWalletCreatedForAllMetrics();
  }
}

void RewardsServiceImpl::GetAutoContributeProperties(
    GetAutoContributePropertiesCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), nullptr);
  }

  bat_ledger_->GetAutoContributeProperties(std::move(callback));
}

void RewardsServiceImpl::OnReconcileComplete(
    const ledger::mojom::Result result,
    ledger::mojom::ContributionInfoPtr contribution) {
  if (result == ledger::mojom::Result::LEDGER_OK &&
      contribution->type == ledger::mojom::RewardsType::RECURRING_TIP) {
    MaybeShowNotificationTipsPaid();
  }

  if (result == ledger::mojom::Result::LEDGER_OK) {
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

void RewardsServiceImpl::LoadLedgerState(
    ledger::client::OnLoadCallback callback) {
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&LoadStateOnFileTaskRunner, ledger_state_path_),
      base::BindOnce(&RewardsServiceImpl::OnLedgerStateLoaded, AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnLedgerStateLoaded(
    ledger::client::OnLoadCallback callback,
    std::pair<std::string, base::Value> state) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (state.second.is_dict()) {
    // Record stats.
    RecordBackendP3AStats();
    p3a::MaybeRecordInitialAdsState(profile_->GetPrefs());
  }
  if (state.first.empty()) {
    p3a::RecordNoWalletCreatedForAllMetrics();
  }

  // Run callbacks.
  const std::string& data = state.first;
  callback(data.empty() ? ledger::mojom::Result::NO_LEDGER_STATE
                        : ledger::mojom::Result::LEDGER_OK,
           data);
}

void RewardsServiceImpl::LoadPublisherState(
    ledger::client::OnLoadCallback callback) {
  file_task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&LoadOnFileTaskRunner, publisher_state_path_),
      base::BindOnce(&RewardsServiceImpl::OnPublisherStateLoaded, AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnPublisherStateLoaded(
    ledger::client::OnLoadCallback callback,
    const std::string& data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!Connected()) {
    return;
  }

  callback(data.empty() ? ledger::mojom::Result::NO_PUBLISHER_STATE
                        : ledger::mojom::Result::LEDGER_OK,
           data);
}

void RewardsServiceImpl::LoadURL(ledger::mojom::UrlRequestPtr request,
                                 ledger::client::LoadURLCallback callback) {
  if (!request || request->url.empty()) {
    ledger::mojom::UrlResponse response;
    response.status_code = net::HTTP_BAD_REQUEST;
    std::move(callback).Run(response);
    return;
  }

  GURL parsed_url(request->url);
  if (!parsed_url.is_valid()) {
    ledger::mojom::UrlResponse response;
    response.url = request->url;
    response.status_code = net::HTTP_BAD_REQUEST;
    std::move(callback).Run(response);
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

    ledger::mojom::UrlResponse response;
    response.url = request->url;
    response.status_code = response_status_code;
    response.body = test_response;
    response.headers = test_headers;
    std::move(callback).Run(response);
    return;
  }

  auto net_request = std::make_unique<network::ResourceRequest>();
  net_request->url = parsed_url;
  net_request->method = URLMethodToRequestType(request->method);
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
      profile_->GetDefaultStoragePartition()
          ->GetURLLoaderFactoryForBrowserProcess()
          .get(),
      base::BindOnce(&RewardsServiceImpl::OnURLLoaderComplete,
                     base::Unretained(this), loader_it, std::move(callback)));
}

void RewardsServiceImpl::OnURLLoaderComplete(
    SimpleURLLoaderList::iterator loader_it,
    ledger::client::LoadURLCallback callback,
    std::unique_ptr<std::string> response_body) {
  auto loader = std::move(*loader_it);
  url_loaders_.erase(loader_it);

  ledger::mojom::UrlResponse response;
  response.body = response_body ? *response_body : "";

  if (loader->NetError() != net::OK) {
    response.error = net::ErrorToString(loader->NetError());
  }

  int response_code = -1;
  if (loader->ResponseInfo() && loader->ResponseInfo()->headers) {
    response_code = loader->ResponseInfo()->headers->response_code();
  }
  response.status_code = response_code;

  const auto url = loader->GetFinalURL();
  response.url = url.spec();

  if (loader->ResponseInfo()) {
    scoped_refptr<net::HttpResponseHeaders> headersList =
        loader->ResponseInfo()->headers;

    if (headersList) {
      size_t iter = 0;
      std::string key;
      std::string value;
      while (headersList->EnumerateHeaderLines(&iter, &key, &value)) {
        key = base::ToLowerASCII(key);
        response.headers[key] = value;
      }
    }
  }

  if (response_body && !response_body->empty() && loader->ResponseInfo() &&
      base::Contains(loader->ResponseInfo()->mime_type, "json")) {
    return data_decoder::JsonSanitizer::Sanitize(
        *response_body,
        base::BindOnce(
            [](ledger::client::LoadURLCallback callback,
               ledger::mojom::UrlResponse response,
               data_decoder::JsonSanitizer::Result result) {
              if (result.value) {
                response.body = std::move(*result.value);
              } else {
                response.body = {};
                VLOG(0) << "Response sanitization error: "
                        << (result.error ? *result.error : "unknown");
              }

              std::move(callback).Run(response);
            },
            std::move(callback), std::move(response)));
  }

  std::move(callback).Run(response);
}

void RewardsServiceImpl::GetRewardsParameters(
    GetRewardsParametersCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), nullptr);
  }

  bat_ledger_->GetRewardsParameters(
      base::BindOnce(&OnGetRewardsParameters, std::move(callback)));
}

void RewardsServiceImpl::OnGetRewardsParameters(
    GetRewardsParametersCallback callback,
    ledger::mojom::RewardsParametersPtr parameters) {
  if (parameters) {
    if (base::FeatureList::IsEnabled(
            brave_rewards::features::kAllowUnsupportedWalletProvidersFeature)) {
      parameters->wallet_provider_regions.clear();
    }

    // If the user has disabled the "VBAT notice" feature then clear the
    // corresponding deadline from the returned data.
    if (!base::FeatureList::IsEnabled(features::kVBatNoticeFeature)) {
      parameters->vbat_deadline = base::Time();
    }
  }

  std::move(callback).Run(std::move(parameters));
}

void RewardsServiceImpl::FetchPromotions(FetchPromotionsCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         std::vector<ledger::mojom::PromotionPtr>());
  }

  auto on_fetch = [](base::WeakPtr<RewardsServiceImpl> self,
                     FetchPromotionsCallback callback,
                     ledger::mojom::Result result,
                     std::vector<ledger::mojom::PromotionPtr> promotions) {
    if (self) {
      for (auto& observer : self->observers_) {
        observer.OnFetchPromotions(self.get(), result, promotions);
      }
    }

    std::move(callback).Run(std::move(promotions));
  };

  bat_ledger_->FetchPromotions(
      base::BindOnce(on_fetch, AsWeakPtr(), std::move(callback)));
}

void ParseCaptchaResponse(
    const std::string& response,
    std::string* image,
    std::string* id,
    std::string* hint) {
  absl::optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return;
  }

  const auto& dict = value->GetDict();
  const auto* captcha_image = dict.FindString("captchaImage");
  const auto* captcha_hint = dict.FindString("hint");
  const auto* captcha_id = dict.FindString("captchaId");
  if (!captcha_image || !captcha_hint || !captcha_id) {
    return;
  }

  *image = *captcha_image;
  *hint = *captcha_hint;
  *id = *captcha_id;
}

void RewardsServiceImpl::OnClaimPromotion(ClaimPromotionCallback callback,
                                          const ledger::mojom::Result result,
                                          const std::string& response) {
  std::string image;
  std::string hint;
  std::string id;

  if (result != ledger::mojom::Result::LEDGER_OK) {
    std::move(callback).Run(result, image, hint, id);
    return;
  }

  ParseCaptchaResponse(response, &image, &id, &hint);

  if (image.empty() || hint.empty() || id.empty()) {
    std::move(callback).Run(result, "", "", "");
    return;
  }

  std::move(callback).Run(result, image, hint, id);
}

void RewardsServiceImpl::AttestationAndroid(const std::string& promotion_id,
                                            AttestPromotionCallback callback,
                                            const ledger::mojom::Result result,
                                            const std::string& nonce) {
  if (result != ledger::mojom::Result::LEDGER_OK) {
    std::move(callback).Run(result, nullptr);
    return;
  }

  if (nonce.empty()) {
    std::move(callback).Run(ledger::mojom::Result::LEDGER_ERROR, nullptr);
    return;
  }

#if BUILDFLAG(IS_ANDROID)
  auto attest_callback =
      base::BindOnce(&RewardsServiceImpl::OnAttestationAndroid, AsWeakPtr(),
                     promotion_id, std::move(callback), nonce);
  safetynet_check_runner_.performSafetynetCheck(nonce,
                                                std::move(attest_callback));
#endif
}

void RewardsServiceImpl::OnAttestationAndroid(
    const std::string& promotion_id,
    AttestPromotionCallback callback,
    const std::string& nonce,
    const bool token_received,
    const std::string& token,
    const bool attestation_passed) {
  if (!Connected() || !token_received) {
    std::move(callback).Run(ledger::mojom::Result::LEDGER_ERROR, nullptr);
    return;
  }

  base::Value::Dict solution;
  solution.Set("nonce", nonce);
  solution.Set("token", token);

  std::string json;
  base::JSONWriter::Write(solution, &json);

  bat_ledger_->AttestPromotion(
      promotion_id,
      json,
      base::BindOnce(&RewardsServiceImpl::OnAttestPromotion,
          AsWeakPtr(),
          std::move(callback)));
}

void RewardsServiceImpl::ClaimPromotion(
    const std::string& promotion_id,
    ClaimPromotionCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         ledger::mojom::Result::LEDGER_ERROR, "", "", "");
  }

  auto claim_callback = base::BindOnce(&RewardsServiceImpl::OnClaimPromotion,
      AsWeakPtr(),
      std::move(callback));

  bat_ledger_->ClaimPromotion(promotion_id, "", std::move(claim_callback));
}

void RewardsServiceImpl::ClaimPromotion(
    const std::string& promotion_id,
    AttestPromotionCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         ledger::mojom::Result::LEDGER_ERROR, nullptr);
  }

  auto claim_callback = base::BindOnce(&RewardsServiceImpl::AttestationAndroid,
      AsWeakPtr(),
      promotion_id,
      std::move(callback));

  bat_ledger_->ClaimPromotion(promotion_id, "", std::move(claim_callback));
}

std::vector<std::string> RewardsServiceImpl::GetExternalWalletProviders()
    const {
  std::vector<std::string> providers;

  if (IsBitFlyerRegion()) {
    providers.push_back(ledger::constant::kWalletBitflyer);
    return providers;
  }

  providers.push_back(ledger::constant::kWalletUphold);

#if BUILDFLAG(ENABLE_GEMINI_WALLET)
  if (base::FeatureList::IsEnabled(features::kGeminiFeature)) {
    providers.push_back(ledger::constant::kWalletGemini);
  }
#endif
  return providers;
}

void RewardsServiceImpl::AttestPromotion(
    const std::string& promotion_id,
    const std::string& solution,
    AttestPromotionCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         ledger::mojom::Result::LEDGER_ERROR, nullptr);
  }

  bat_ledger_->AttestPromotion(
      promotion_id,
      solution,
      base::BindOnce(&RewardsServiceImpl::OnAttestPromotion,
          AsWeakPtr(),
          std::move(callback)));
}

void RewardsServiceImpl::OnAttestPromotion(
    AttestPromotionCallback callback,
    const ledger::mojom::Result result,
    ledger::mojom::PromotionPtr promotion) {
  if (result != ledger::mojom::Result::LEDGER_OK) {
    std::move(callback).Run(result, nullptr);
    return;
  }

  for (auto& observer : observers_) {
    observer.OnPromotionFinished(this, result, promotion->Clone());
  }

  std::move(callback).Run(result, std::move(promotion));
}

void RewardsServiceImpl::GetReconcileStamp(GetReconcileStampCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), 0);
  }

  bat_ledger_->GetReconcileStamp(std::move(callback));
}

#if BUILDFLAG(ENABLE_GREASELION)
void RewardsServiceImpl::EnableGreaseLion() {
  if (!greaselion_service_) {
    return;
  }

  greaselion_service_->SetFeatureEnabled(greaselion::REWARDS, true);
  greaselion_service_->SetFeatureEnabled(
      greaselion::AUTO_CONTRIBUTION,
      profile_->GetPrefs()->GetBoolean(prefs::kAutoContributeEnabled));
  greaselion_service_->SetFeatureEnabled(
      greaselion::ADS, profile_->GetPrefs()->GetBoolean(ads::prefs::kEnabled));

  const bool show_button = profile_->GetPrefs()->GetBoolean(prefs::kShowButton);
  greaselion_service_->SetFeatureEnabled(
      greaselion::TWITTER_TIPS,
      profile_->GetPrefs()->GetBoolean(prefs::kInlineTipTwitterEnabled) &&
          show_button);
  greaselion_service_->SetFeatureEnabled(
      greaselion::REDDIT_TIPS,
      profile_->GetPrefs()->GetBoolean(prefs::kInlineTipRedditEnabled) &&
          show_button);
  greaselion_service_->SetFeatureEnabled(
      greaselion::GITHUB_TIPS,
      profile_->GetPrefs()->GetBoolean(prefs::kInlineTipGithubEnabled) &&
          show_button);
}

void RewardsServiceImpl::OnRulesReady(
    greaselion::GreaselionService* greaselion_service) {
  EnableGreaseLion();
}
#endif

void RewardsServiceImpl::StopLedger(StopLedgerCallback callback) {
  BLOG(1, "Shutting down ledger process");
  if (!Connected()) {
    BLOG(1, "Ledger process not running");
    OnStopLedger(std::move(callback), ledger::mojom::Result::LEDGER_OK);
    return;
  }

  bat_ledger_->Shutdown(base::BindOnce(
      &RewardsServiceImpl::OnStopLedger,
      AsWeakPtr(),
      std::move(callback)));
}

void RewardsServiceImpl::OnStopLedger(StopLedgerCallback callback,
                                      const ledger::mojom::Result result) {
  BLOG_IF(1, result != ledger::mojom::Result::LEDGER_OK,
          "Ledger process was not shut down successfully");
  Reset();
  BLOG(1, "Successfully shutdown ledger");
  std::move(callback).Run(result);
}

void RewardsServiceImpl::OnStopLedgerForCompleteReset(
    SuccessCallback callback,
    const ledger::mojom::Result result) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  profile_->GetPrefs()->ClearPrefsWithPrefixSilently(pref_prefix);
  diagnostic_log_->Delete(base::BindOnce(
      &RewardsServiceImpl::OnDiagnosticLogDeletedForCompleteReset, AsWeakPtr(),
      std::move(callback)));
}

void RewardsServiceImpl::OnDiagnosticLogDeletedForCompleteReset(
    SuccessCallback callback,
    bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  const std::vector<base::FilePath> paths = {
    ledger_state_path_,
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

  BitmapFetcherService* image_service =
      BitmapFetcherServiceFactory::GetForBrowserContext(profile_);
  if (image_service) {
    for (auto mapping : current_media_fetchers_) {
      image_service->CancelRequest(mapping.second);
    }
  }

  current_media_fetchers_.clear();
  bat_ledger_.reset();
  bat_ledger_client_receiver_.reset();
  bat_ledger_service_.reset();
  ready_ = std::make_unique<base::OneShotEvent>();
  ledger_database_.Reset();
  BLOG(1, "Successfully reset rewards service");
}

void RewardsServiceImpl::SetBooleanState(const std::string& name, bool value) {
  profile_->GetPrefs()->SetBoolean(GetPrefPath(name), value);
}

bool RewardsServiceImpl::GetBooleanState(const std::string& name) const {
  return profile_->GetPrefs()->GetBoolean(GetPrefPath(name));
}

void RewardsServiceImpl::SetIntegerState(const std::string& name, int value) {
  profile_->GetPrefs()->SetInteger(GetPrefPath(name), value);
}

int RewardsServiceImpl::GetIntegerState(const std::string& name) const {
  return profile_->GetPrefs()->GetInteger(GetPrefPath(name));
}

void RewardsServiceImpl::SetDoubleState(const std::string& name, double value) {
  profile_->GetPrefs()->SetDouble(GetPrefPath(name), value);
}

double RewardsServiceImpl::GetDoubleState(const std::string& name) const {
  return profile_->GetPrefs()->GetDouble(GetPrefPath(name));
}

void RewardsServiceImpl::SetStringState(const std::string& name,
                                        const std::string& value) {
  profile_->GetPrefs()->SetString(GetPrefPath(name), value);
}

std::string RewardsServiceImpl::GetStringState(const std::string& name) const {
  return profile_->GetPrefs()->GetString(GetPrefPath(name));
}

void RewardsServiceImpl::SetInt64State(const std::string& name, int64_t value) {
  profile_->GetPrefs()->SetInt64(GetPrefPath(name), value);
}

int64_t RewardsServiceImpl::GetInt64State(const std::string& name) const {
  return profile_->GetPrefs()->GetInt64(GetPrefPath(name));
}

void RewardsServiceImpl::SetUint64State(const std::string& name,
                                        uint64_t value) {
  profile_->GetPrefs()->SetUint64(GetPrefPath(name), value);
}

uint64_t RewardsServiceImpl::GetUint64State(const std::string& name) const {
  return profile_->GetPrefs()->GetUint64(GetPrefPath(name));
}

void RewardsServiceImpl::SetValueState(const std::string& name,
                                       base::Value value) {
  profile_->GetPrefs()->Set(GetPrefPath(name), std::move(value));
}

base::Value RewardsServiceImpl::GetValueState(const std::string& name) const {
  return profile_->GetPrefs()->GetValue(GetPrefPath(name)).Clone();
}

void RewardsServiceImpl::SetTimeState(const std::string& name,
                                      base::Time time) {
  profile_->GetPrefs()->SetTime(GetPrefPath(name), time);
}

base::Time RewardsServiceImpl::GetTimeState(const std::string& name) const {
  return profile_->GetPrefs()->GetTime(GetPrefPath(name));
}

void RewardsServiceImpl::ClearState(const std::string& name) {
  profile_->GetPrefs()->ClearPref(GetPrefPath(name));
}

bool RewardsServiceImpl::GetBooleanOption(const std::string& name) const {
  DCHECK(!name.empty());

  if (name == ledger::option::kIsBitflyerRegion)
    return GetExternalWalletType() == ledger::constant::kWalletBitflyer;

  const auto it = kBoolOptions.find(name);
  DCHECK(it != kBoolOptions.end());

  return kBoolOptions.at(name);
}

int RewardsServiceImpl::GetIntegerOption(const std::string& name) const {
  DCHECK(!name.empty());

  const auto it = kIntegerOptions.find(name);
  DCHECK(it != kIntegerOptions.end());

  return kIntegerOptions.at(name);
}

double RewardsServiceImpl::GetDoubleOption(const std::string& name) const {
  DCHECK(!name.empty());

  const auto it = kDoubleOptions.find(name);
  DCHECK(it != kDoubleOptions.end());

  return kDoubleOptions.at(name);
}

std::string RewardsServiceImpl::GetStringOption(const std::string& name) const {
  DCHECK(!name.empty());

  const auto it = kStringOptions.find(name);
  DCHECK(it != kStringOptions.end());

  return kStringOptions.at(name);
}

int64_t RewardsServiceImpl::GetInt64Option(const std::string& name) const {
  DCHECK(!name.empty());

  const auto it = kInt64Options.find(name);
  DCHECK(it != kInt64Options.end());

  return kInt64Options.at(name);
}

uint64_t RewardsServiceImpl::GetUint64Option(const std::string& name) const {
  DCHECK(!name.empty());

  const auto it = kUInt64Options.find(name);
  DCHECK(it != kUInt64Options.end());

  return kUInt64Options.at(name);
}

void RewardsServiceImpl::GetPublisherMinVisitTime(
    GetPublisherMinVisitTimeCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), 0);
  }

  bat_ledger_->GetPublisherMinVisitTime(std::move(callback));
}

void RewardsServiceImpl::SetPublisherMinVisitTime(
    int duration_in_seconds) const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetPublisherMinVisitTime(duration_in_seconds);
}

void RewardsServiceImpl::GetPublisherMinVisits(
    GetPublisherMinVisitsCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), 0);
  }

  bat_ledger_->GetPublisherMinVisits(std::move(callback));
}

void RewardsServiceImpl::SetPublisherMinVisits(int visits) const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetPublisherMinVisits(visits);
}

void RewardsServiceImpl::GetPublisherAllowNonVerified(
    GetPublisherAllowNonVerifiedCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), false);
  }

  bat_ledger_->GetPublisherAllowNonVerified(std::move(callback));
}

void RewardsServiceImpl::SetPublisherAllowNonVerified(bool allow) const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetPublisherAllowNonVerified(allow);
}

void RewardsServiceImpl::GetPublisherAllowVideos(
    GetPublisherAllowVideosCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), false);
  }

  bat_ledger_->GetPublisherAllowVideos(std::move(callback));
}

void RewardsServiceImpl::SetPublisherAllowVideos(bool allow) const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetPublisherAllowVideos(allow);
}

void RewardsServiceImpl::SetAutoContributionAmount(const double amount) const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetAutoContributionAmount(amount);
}

void RewardsServiceImpl::GetAutoContributeEnabled(
    GetAutoContributeEnabledCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), false);
  }

  bat_ledger_->GetAutoContributeEnabled(std::move(callback));
}

void RewardsServiceImpl::SetAutoContributeEnabled(bool enabled) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetAutoContributeEnabled(enabled);
}

void RewardsServiceImpl::OnGetBalanceReport(
    GetBalanceReportCallback callback,
    const ledger::mojom::Result result,
    ledger::mojom::BalanceReportInfoPtr report) {
  std::move(callback).Run(result, std::move(report));
}

void RewardsServiceImpl::GetBalanceReport(
    const uint32_t month,
    const uint32_t year,
    GetBalanceReportCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         ledger::mojom::Result::LEDGER_OK, nullptr);
  }

  bat_ledger_->GetBalanceReport(
      static_cast<ledger::mojom::ActivityMonth>(month), year,
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
    ledger::mojom::PublisherInfoPtr info;
    OnPanelPublisherInfo(ledger::mojom::Result::NOT_FOUND, std::move(info),
                         windowId);
    return;
  }

  if (!Connected()) {
    return;
  }

  ledger::mojom::VisitDataPtr visit_data = ledger::mojom::VisitData::New();
  visit_data->domain = *publisher_domain;
  visit_data->name = *publisher_domain;
  visit_data->path = parsed_url.has_path() ? parsed_url.PathForRequest() : "";
  visit_data->url = parsed_url.scheme() + "://" + *publisher_domain + "/";
  visit_data->favicon_url = favicon_url;

  bat_ledger_->GetPublisherActivityFromUrl(windowId, std::move(visit_data),
                                           publisher_blob);
}

void RewardsServiceImpl::OnPanelPublisherInfo(
    const ledger::mojom::Result result,
    ledger::mojom::PublisherInfoPtr info,
    uint64_t windowId) {
  if (result != ledger::mojom::Result::LEDGER_OK &&
      result != ledger::mojom::Result::NOT_FOUND) {
    return;
  }

  for (auto& observer : observers_)
    observer.OnPanelPublisherInfo(this,
                                  result,
                                  info.get(),
                                  windowId);
}

void RewardsServiceImpl::GetAutoContributionAmount(
    GetAutoContributionAmountCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), 0);
  }

  bat_ledger_->GetAutoContributionAmount(std::move(callback));
}

void RewardsServiceImpl::FetchFavIcon(
    const std::string& url,
    const std::string& favicon_key,
    ledger::client::FetchIconCallback callback) {
  GURL parsedUrl(url);

  if (!parsedUrl.is_valid()) {
    return;
  }

  auto it = current_media_fetchers_.find(url);
  if (it != current_media_fetchers_.end()) {
    BLOG(1, "Already fetching favicon");
    return;
  }

  BitmapFetcherService* image_service =
      BitmapFetcherServiceFactory::GetForBrowserContext(profile_);
  if (image_service) {
    current_media_fetchers_[url] = image_service->RequestImage(
        parsedUrl,
        base::BindOnce(&RewardsServiceImpl::OnFetchFavIconCompleted,
                       AsWeakPtr(), std::move(callback), favicon_key,
                       parsedUrl),
        GetNetworkTrafficAnnotationTagForFaviconFetch());
  }
}

void RewardsServiceImpl::OnFetchFavIconCompleted(
    ledger::client::FetchIconCallback callback,
    const std::string& favicon_key,
    const GURL& url,
    const SkBitmap& image) {
  GURL favicon_url(favicon_key);
  gfx::Image gfx_image = gfx::Image::CreateFrom1xBitmap(image);
  favicon::FaviconService* favicon_service =
          FaviconServiceFactory::GetForProfile(profile_,
              ServiceAccessType::EXPLICIT_ACCESS);
  favicon_service->SetOnDemandFavicons(
      favicon_url,
      url,
      favicon_base::IconType::kFavicon,
      gfx_image,
      base::BindOnce(&RewardsServiceImpl::OnSetOnDemandFaviconComplete,
          AsWeakPtr(), favicon_url.spec(), callback));

  auto it_url = current_media_fetchers_.find(url.spec());
  if (it_url != current_media_fetchers_.end()) {
    current_media_fetchers_.erase(it_url);
  }
}

void RewardsServiceImpl::OnSetOnDemandFaviconComplete(
    const std::string& favicon_url,
    ledger::client::FetchIconCallback callback,
    bool success) {
  callback(success, favicon_url);
}

void RewardsServiceImpl::GetPublisherBanner(
    const std::string& publisher_id,
    GetPublisherBannerCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), nullptr);
  }

  bat_ledger_->GetPublisherBanner(publisher_id,
      base::BindOnce(&RewardsServiceImpl::OnPublisherBanner,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnPublisherBanner(
    GetPublisherBannerCallback callback,
    ledger::mojom::PublisherBannerPtr banner) {
  std::move(callback).Run(std::move(banner));
}

void RewardsServiceImpl::OnSaveRecurringTip(OnTipCallback callback,
                                            ledger::mojom::Result result) {
  for (auto& observer : observers_) {
    observer.OnRecurringTipSaved(this,
                                 result == ledger::mojom::Result::LEDGER_OK);
  }

  std::move(callback).Run(result);
}

void RewardsServiceImpl::SaveRecurringTip(const std::string& publisher_key,
                                          double amount,
                                          OnTipCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         ledger::mojom::Result::LEDGER_ERROR);
  }

  ledger::mojom::RecurringTipPtr info = ledger::mojom::RecurringTip::New();
  info->publisher_key = publisher_key;
  info->amount = amount;
  info->created_at = GetCurrentTimestamp();

  bat_ledger_->SaveRecurringTip(
      std::move(info), base::BindOnce(&RewardsServiceImpl::OnSaveRecurringTip,
                                      AsWeakPtr(), std::move(callback)));
}

void RewardsServiceImpl::UpdateMediaDuration(
    const uint64_t window_id,
    const std::string& publisher_key,
    const uint64_t duration,
    const bool first_visit) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->UpdateMediaDuration(
      window_id,
      publisher_key,
      duration,
      first_visit);
}

void RewardsServiceImpl::IsPublisherRegistered(
    const std::string& publisher_id,
    base::OnceCallback<void(bool)> callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), false);
  }

  bat_ledger_->IsPublisherRegistered(publisher_id, std::move(callback));
}

void RewardsServiceImpl::GetPublisherInfo(
    const std::string& publisher_key,
    GetPublisherInfoCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         ledger::mojom::Result::LEDGER_ERROR, nullptr);
  }

  bat_ledger_->GetPublisherInfo(publisher_key, std::move(callback));
}

void RewardsServiceImpl::GetPublisherPanelInfo(
    const std::string& publisher_key,
    GetPublisherInfoCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         ledger::mojom::Result::LEDGER_ERROR, nullptr);
  }

  bat_ledger_->GetPublisherPanelInfo(publisher_key, std::move(callback));
}

void RewardsServiceImpl::SavePublisherInfo(
    const uint64_t window_id,
    ledger::mojom::PublisherInfoPtr publisher_info,
    SavePublisherInfoCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         ledger::mojom::Result::LEDGER_ERROR);
  }

  bat_ledger_->SavePublisherInfo(window_id, std::move(publisher_info),
                                 std::move(callback));
}

void RewardsServiceImpl::GetRecurringTips(
    GetRecurringTipsCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         std::vector<ledger::mojom::PublisherInfoPtr>());
  }

  bat_ledger_->GetRecurringTips(std::move(callback));
}

void RewardsServiceImpl::GetOneTimeTips(GetOneTimeTipsCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         std::vector<ledger::mojom::PublisherInfoPtr>());
  }

  bat_ledger_->GetOneTimeTips(std::move(callback));
}

void RewardsServiceImpl::OnRecurringTip(const ledger::mojom::Result result) {
  bool success = result == ledger::mojom::Result::LEDGER_OK;
  for (auto& observer : observers_) {
    observer.OnRecurringTipRemoved(this, success);
  }
}

void RewardsServiceImpl::RemoveRecurringTip(
    const std::string& publisher_key) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->RemoveRecurringTip(
      publisher_key,
      base::BindOnce(&RewardsServiceImpl::OnRecurringTip, AsWeakPtr()));
}

void RewardsServiceImpl::OnSetPublisherExclude(
    const std::string& publisher_key,
    const bool exclude,
    const ledger::mojom::Result result) {
  if (result != ledger::mojom::Result::LEDGER_OK) {
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

  ledger::mojom::PublisherExclude status =
      exclude ? ledger::mojom::PublisherExclude::EXCLUDED
              : ledger::mojom::PublisherExclude::INCLUDED;

  bat_ledger_->SetPublisherExclude(
    publisher_key,
    status,
    base::BindOnce(&RewardsServiceImpl::OnSetPublisherExclude,
                   AsWeakPtr(),
                   publisher_key,
                   exclude));
}

RewardsNotificationService* RewardsServiceImpl::GetNotificationService() const {
  return notification_service_.get();
}

void RewardsServiceImpl::StartNotificationTimers() {
  // Startup timer, begins after 30-second delay.
  PrefService* pref_service = profile_->GetPrefs();
  notification_startup_timer_ = std::make_unique<base::OneShotTimer>();
  notification_startup_timer_->Start(
      FROM_HERE,
      pref_service->GetTimeDelta(
        prefs::kNotificationStartupDelay),
      this,
      &RewardsServiceImpl::OnNotificationTimerFired);
  DCHECK(notification_startup_timer_->IsRunning());

  // Periodic timer, runs once per day by default.
  base::TimeDelta periodic_timer_interval =
      pref_service->GetTimeDelta(prefs::kNotificationTimerInterval);
  notification_periodic_timer_ = std::make_unique<base::RepeatingTimer>();
  notification_periodic_timer_->Start(
      FROM_HERE, periodic_timer_interval, this,
      &RewardsServiceImpl::OnNotificationTimerFired);
  DCHECK(notification_periodic_timer_->IsRunning());
}

void RewardsServiceImpl::StopNotificationTimers() {
  notification_startup_timer_.reset();
  notification_periodic_timer_.reset();
}

void RewardsServiceImpl::OnNotificationTimerFired() {
  if (!Connected()) {
    return;
  }

  FetchPromotions(base::DoNothing());
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
  if (ledger_for_testing_ || resetting_rewards_) {
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

void RewardsServiceImpl::Log(
    const char* file,
    const int line,
    const int verbose_level,
    const std::string& message) {
  DCHECK(file);

  WriteDiagnosticLog(file, line, verbose_level, message);

  const int vlog_level = ::logging::GetVlogLevelHelper(file, strlen(file));
  if (verbose_level <= vlog_level) {
    ::logging::LogMessage(file, line, -verbose_level).stream() << message;
  }
}

// static
void RewardsServiceImpl::HandleFlags(const RewardsFlags& flags) {
  if (flags.environment) {
    switch (*flags.environment) {
      case RewardsFlags::Environment::kDevelopment:
        SetEnvironment(ledger::mojom::Environment::DEVELOPMENT);
        break;
      case RewardsFlags::Environment::kStaging:
        SetEnvironment(ledger::mojom::Environment::STAGING);
        break;
      case RewardsFlags::Environment::kProduction:
        SetEnvironment(ledger::mojom::Environment::PRODUCTION);
        break;
    }
  }

  if (flags.debug) {
    SetDebug(true);
  }

  if (flags.reconcile_interval) {
    SetReconcileInterval(*flags.reconcile_interval);
  }

  if (flags.retry_interval) {
    SetRetryInterval(*flags.retry_interval);
  }

  // The "persist-logs" command-line flag is deprecated and will be removed
  // in a future version. Use --enable-features=BraveRewardsVerboseLogging
  // instead.
  if (flags.persist_logs) {
    persist_log_level_ = kDiagnosticLogMaxVerboseLevel;
  }

  if (flags.country_id) {
    country_id_ = *flags.country_id;
  }
}

void RewardsServiceImpl::GetRewardsInternalsInfo(
    GetRewardsInternalsInfoCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), nullptr);
  }

  bat_ledger_->GetRewardsInternalsInfo(std::move(callback));
}

void RewardsServiceImpl::OnTip(const std::string& publisher_key,
                               const double amount,
                               const bool recurring,
                               ledger::mojom::PublisherInfoPtr publisher) {
  if (!Connected() || !publisher) {
    return;
  }
  publisher->id = publisher_key;

  bat_ledger_->SavePublisherInfoForTip(
      std::move(publisher),
      base::BindOnce(&RewardsServiceImpl::OnTipPublisherSaved,
          AsWeakPtr(),
          publisher_key,
          amount,
          recurring));
}

void RewardsServiceImpl::OnTipPublisherSaved(
    const std::string& publisher_key,
    const double amount,
    const bool recurring,
    const ledger::mojom::Result result) {
  if (result != ledger::mojom::Result::LEDGER_OK) {
    return;
  }

  OnTip(publisher_key, amount, recurring, base::DoNothing());
}

void RewardsServiceImpl::OnTip(const std::string& publisher_key,
                               double amount,
                               bool recurring,
                               OnTipCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         ledger::mojom::Result::LEDGER_ERROR);
  }

  if (recurring) {
    return SaveRecurringTip(publisher_key, amount, std::move(callback));
  }

  bat_ledger_->OneTimeTip(publisher_key, amount, std::move(callback));
}

bool RewardsServiceImpl::Connected() const {
  return bat_ledger_.is_bound();
}

void RewardsServiceImpl::StartProcessForTesting(base::OnceClosure callback) {
  ready_->Post(FROM_HERE, std::move(callback));
  StartLedgerProcessIfNecessary();
}

void RewardsServiceImpl::SetLedgerEnvForTesting() {
  ledger_for_testing_ = true;
}

void RewardsServiceImpl::SetLedgerStateTargetVersionForTesting(int version) {
  ledger_state_target_version_for_testing_ = version;
}

void RewardsServiceImpl::PrepareLedgerEnvForTesting() {
  if (!ledger_for_testing_) {
    return;
  }

  bat_ledger_service_->SetTesting();
  bat_ledger_service_->SetStateMigrationTargetVersionForTesting(
      ledger_state_target_version_for_testing_);
  SetRetryInterval(1);

  profile_->GetPrefs()->SetInteger(prefs::kMinVisitTime, 1);

  // this is needed because we are using braveledger_request_util::buildURL
  // directly in RewardsBrowserTest
  #if defined(OFFICIAL_BUILD)
  ledger::_environment = ledger::mojom::Environment::PRODUCTION;
#else
  ledger::_environment = ledger::mojom::Environment::STAGING;
#endif
}

void RewardsServiceImpl::StartMonthlyContributionForTest() {
  if (!Connected()) {
    return;
  }

  bat_ledger_->StartMonthlyContribution();
}

void RewardsServiceImpl::GetEnvironment(GetEnvironmentCallback callback) {
  if (!bat_ledger_service_.is_bound()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         GetDefaultServerEnvironment());
  }
  bat_ledger_service_->GetEnvironment(std::move(callback));
}

void RewardsServiceImpl::GetDebug(GetDebugCallback callback) {
  bat_ledger_service_->GetDebug(std::move(callback));
}

void RewardsServiceImpl::GetReconcileInterval(
    GetReconcileIntervalCallback callback) {
  bat_ledger_service_->GetReconcileInterval(std::move(callback));
}

void RewardsServiceImpl::GetRetryInterval(GetRetryIntervalCallback callback) {
  bat_ledger_service_->GetRetryInterval(std::move(callback));
}

void RewardsServiceImpl::SetEnvironment(
    ledger::mojom::Environment environment) {
  bat_ledger_service_->SetEnvironment(environment);
}

void RewardsServiceImpl::SetDebug(bool debug) {
  bat_ledger_service_->SetDebug(debug);
}

void RewardsServiceImpl::SetReconcileInterval(const int32_t interval) {
  bat_ledger_service_->SetReconcileInterval(interval);
}

void RewardsServiceImpl::SetRetryInterval(int32_t interval) {
  bat_ledger_service_->SetRetryInterval(interval);
}

void RewardsServiceImpl::GetPendingContributionsTotal(
    GetPendingContributionsTotalCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), 0);
  }

  bat_ledger_->GetPendingContributionsTotal(std::move(callback));
}

void RewardsServiceImpl::PublisherListNormalized(
    std::vector<ledger::mojom::PublisherInfoPtr> list) {
  for (auto& observer : observers_) {
    std::vector<ledger::mojom::PublisherInfoPtr> new_list;
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
                         ledger::mojom::PublisherStatus::NOT_VERIFIED, "");
  }
  bat_ledger_->RefreshPublisher(
      publisher_key,
      base::BindOnce(&RewardsServiceImpl::OnRefreshPublisher,
        AsWeakPtr(),
        std::move(callback),
        publisher_key));
}

void RewardsServiceImpl::OnRefreshPublisher(
    RefreshPublisherCallback callback,
    const std::string& publisher_key,
    ledger::mojom::PublisherStatus status) {
  std::move(callback).Run(status, publisher_key);
}

const RewardsNotificationService::RewardsNotificationsMap&
RewardsServiceImpl::GetAllNotifications() {
  return notification_service_->GetAllNotifications();
}

void RewardsServiceImpl::SetInlineTippingPlatformEnabled(
    const std::string& key,
    bool enabled) {
  if (!Connected()) {
    return;
  }

  const auto platform = ConvertInlineTipStringToPlatform(key);
  bat_ledger_->SetInlineTippingPlatformEnabled(platform, enabled);
}

void RewardsServiceImpl::GetInlineTippingPlatformEnabled(
      const std::string& key,
      GetInlineTippingPlatformEnabledCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), false);
  }

  const auto platform = ConvertInlineTipStringToPlatform(key);
  bat_ledger_->GetInlineTippingPlatformEnabled(platform, std::move(callback));
}

void RewardsServiceImpl::GetShareURL(
      const base::flat_map<std::string, std::string>& args,
      GetShareURLCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), "");
  }

  bat_ledger_->GetShareURL(args, std::move(callback));
}

void RewardsServiceImpl::GetPendingContributions(
    GetPendingContributionsCallback callback) {
  if (!Connected()) {
    return DeferCallback(
        FROM_HERE, std::move(callback),
        std::vector<ledger::mojom::PendingContributionInfoPtr>());
  }

  bat_ledger_->GetPendingContributions(std::move(callback));
}

void RewardsServiceImpl::OnPendingContributionRemoved(
    const ledger::mojom::Result result) {
  for (auto& observer : observers_) {
    observer.OnPendingContributionRemoved(this, result);
  }
}

void RewardsServiceImpl::RemovePendingContribution(const uint64_t id) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->RemovePendingContribution(
      id,
      base::BindOnce(&RewardsServiceImpl::OnPendingContributionRemoved,
                     AsWeakPtr()));
}

void RewardsServiceImpl::OnRemoveAllPendingContributions(
    const ledger::mojom::Result result) {
  for (auto& observer : observers_) {
    observer.OnPendingContributionRemoved(this, result);
  }
}

void RewardsServiceImpl::RemoveAllPendingContributions() {
  if (!Connected()) {
    return;
  }

  bat_ledger_->RemoveAllPendingContributions(
      base::BindOnce(&RewardsServiceImpl::OnRemoveAllPendingContributions,
                     AsWeakPtr()));
}

void RewardsServiceImpl::OnContributeUnverifiedPublishers(
    ledger::mojom::Result result,
    const std::string& publisher_key,
    const std::string& publisher_name) {
  switch (result) {
    case ledger::mojom::Result::PENDING_PUBLISHER_REMOVED: {
      for (auto& observer : observers_) {
        observer.OnPendingContributionRemoved(this,
                                              ledger::mojom::Result::LEDGER_OK);
      }
      break;
    }
    case ledger::mojom::Result::VERIFIED_PUBLISHER: {
      RewardsNotificationService::RewardsNotificationArgs args;
      args.push_back(publisher_name);
      notification_service_->AddNotification(
          RewardsNotificationService::REWARDS_NOTIFICATION_VERIFIED_PUBLISHER,
          args,
          "rewards_notification_verified_publisher_" + publisher_key);
      break;
    }
    default:
      break;
  }
}

void RewardsServiceImpl::FetchBalance(FetchBalanceCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         ledger::mojom::Result::LEDGER_ERROR, nullptr);
  }

  bat_ledger_->FetchBalance(std::move(callback));
}

bool RewardsServiceImpl::IsAutoContributeSupported() const {
  // Auto-contribute is currently not supported in bitFlyer regions
  return !IsBitFlyerRegion();
}

std::string RewardsServiceImpl::GetLegacyWallet() {
  const auto& dict = profile_->GetPrefs()->GetDict(prefs::kExternalWallets);

  std::string json;
  for (auto it = dict.begin(); it != dict.end(); ++it) {
    base::JSONWriter::Write(std::move(it->second), &json);
  }

  return json;
}

void RewardsServiceImpl::GetExternalWallet(GetExternalWalletCallback callback) {
  if (!Connected()) {
    return DeferCallback(
        FROM_HERE, std::move(callback),
        base::unexpected(ledger::mojom::GetExternalWalletError::kUnexpected));
  }

  bat_ledger_->GetExternalWallet(GetExternalWalletType(), std::move(callback));
}

void RewardsServiceImpl::ConnectExternalWallet(
    const std::string& path,
    const std::string& query,
    ConnectExternalWalletCallback callback) {
  if (!Connected()) {
    return DeferCallback(
        FROM_HERE, std::move(callback),
        base::unexpected(
            ledger::mojom::ConnectExternalWalletError::kUnexpected));
  }

  const auto path_items = base::SplitString(path, "/", base::TRIM_WHITESPACE,
                                            base::SPLIT_WANT_NONEMPTY);
  if (path_items.empty()) {
    return DeferCallback(
        FROM_HERE, std::move(callback),
        base::unexpected(
            ledger::mojom::ConnectExternalWalletError::kUnexpected));
  }

  const std::string wallet_type = path_items.at(0);
  base::flat_map<std::string, std::string> query_parameters;

  const auto url = GURL("brave:/" + path + query);
  for (net::QueryIterator it(url); !it.IsAtEnd(); it.Advance()) {
    query_parameters[static_cast<std::string>(it.GetKey())] =
        it.GetUnescapedValue();
  }

  bat_ledger_->ConnectExternalWallet(wallet_type, query_parameters,
                                     std::move(callback));
}

void RewardsServiceImpl::ShowNotification(
    const std::string& type,
    const std::vector<std::string>& args,
    ledger::LegacyResultCallback callback) {
  if (type.empty()) {
    callback(ledger::mojom::Result::LEDGER_ERROR);
    return;
  }

  RewardsNotificationService::RewardsNotificationArgs notification_args;
  notification_args.push_back(type);
  notification_args.insert(notification_args.end(), args.begin(), args.end());
  notification_service_->AddNotification(
      RewardsNotificationService::REWARDS_NOTIFICATION_GENERAL_LEDGER,
      notification_args,
      "rewards_notification_general_ledger_" + type);

  callback(ledger::mojom::Result::LEDGER_OK);
}

void RewardsServiceImpl::RecordBackendP3AStats() {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetRecurringTips(
      base::BindOnce(&RewardsServiceImpl::OnRecordBackendP3AStatsRecurring,
          AsWeakPtr()));
}

void RewardsServiceImpl::OnRecordBackendP3AStatsRecurring(
    std::vector<ledger::mojom::PublisherInfoPtr> list) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetAllContributions(
      base::BindOnce(&RewardsServiceImpl::OnRecordBackendP3AStatsContributions,
          AsWeakPtr(),
          list.size()));
}

void RewardsServiceImpl::OnRecordBackendP3AStatsContributions(
    const uint32_t recurring_donation_size,
    std::vector<ledger::mojom::ContributionInfoPtr> list) {
  int auto_contributions = 0;
  int tips = 0;
  int queued_recurring = 0;

  for (const auto& contribution : list) {
    switch (contribution->type) {
      case ledger::mojom::RewardsType::AUTO_CONTRIBUTE: {
        auto_contributions += 1;
        break;
      }
      case ledger::mojom::RewardsType::ONE_TIME_TIP: {
        tips += 1;
        break;
      }
      case ledger::mojom::RewardsType::RECURRING_TIP: {
        queued_recurring += 1;
        break;
      }
    default:
      NOTREACHED();
    }
  }

  if (queued_recurring == 0) {
    queued_recurring = recurring_donation_size;
  }

  p3a::RecordTipsState(true, true, tips, queued_recurring);

  GetAutoContributeEnabled(base::BindOnce(
    &RewardsServiceImpl::OnRecordBackendP3AStatsAC,
    AsWeakPtr(),
    auto_contributions));
}

void RewardsServiceImpl::OnRecordBackendP3AStatsAC(
    const int auto_contributions,
    bool ac_enabled) {
  const auto auto_contributions_state =
      ac_enabled ? p3a::AutoContributionsState::kAutoContributeOn
                 : p3a::AutoContributionsState::kWalletCreatedAutoContributeOff;
  p3a::RecordAutoContributionsState(auto_contributions_state,
                                    auto_contributions);
}

ledger::mojom::Environment RewardsServiceImpl::GetDefaultServerEnvironment() {
  ledger::mojom::Environment environment = ledger::mojom::Environment::STAGING;
#if defined(OFFICIAL_BUILD) && BUILDFLAG(IS_ANDROID)
  environment = GetDefaultServerEnvironmentForAndroid();
#elif defined(OFFICIAL_BUILD)
  environment = ledger::mojom::Environment::PRODUCTION;
#endif
  return environment;
}

#if BUILDFLAG(IS_ANDROID)
ledger::mojom::Environment
RewardsServiceImpl::GetDefaultServerEnvironmentForAndroid() {
  auto result = ledger::mojom::Environment::PRODUCTION;
  bool use_staging = false;
  if (profile_ && profile_->GetPrefs()) {
    use_staging =
        profile_->GetPrefs()->GetBoolean(prefs::kUseRewardsStagingServer);
  }

  if (use_staging) {
    result = ledger::mojom::Environment::STAGING;
  }

  return result;
}
#endif

ledger::mojom::ClientInfoPtr GetDesktopClientInfo() {
  auto info = ledger::mojom::ClientInfo::New();
  info->platform = ledger::mojom::Platform::DESKTOP;
#if BUILDFLAG(IS_MAC)
  info->os = ledger::mojom::OperatingSystem::MACOS;
#elif BUILDFLAG(IS_WIN)
  info->os = ledger::mojom::OperatingSystem::WINDOWS;
#elif BUILDFLAG(IS_LINUX)
  info->os = ledger::mojom::OperatingSystem::LINUX;
#else
  info->os = ledger::mojom::OperatingSystem::UNDEFINED;
#endif

  return info;
}

ledger::mojom::ClientInfoPtr RewardsServiceImpl::GetClientInfo() {
#if BUILDFLAG(IS_ANDROID)
  return android_util::GetAndroidClientInfo();
#else
  return GetDesktopClientInfo();
#endif
}

void RewardsServiceImpl::UnblindedTokensReady() {
  for (auto& observer : observers_) {
    observer.OnUnblindedTokensReady(this);
  }
}

void RewardsServiceImpl::GetMonthlyReport(
    const uint32_t month,
    const uint32_t year,
    GetMonthlyReportCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), nullptr);
  }

  bat_ledger_->GetMonthlyReport(
      static_cast<ledger::mojom::ActivityMonth>(month), year,
      base::BindOnce(&RewardsServiceImpl::OnGetMonthlyReport, AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnGetMonthlyReport(
    GetMonthlyReportCallback callback,
    const ledger::mojom::Result result,
    ledger::mojom::MonthlyReportInfoPtr report) {
  std::move(callback).Run(std::move(report));
}

void RewardsServiceImpl::ReconcileStampReset() {
  for (auto& observer : observers_) {
    observer.ReconcileStampReset();
  }
}

void RewardsServiceImpl::RunDBTransaction(
    ledger::mojom::DBTransactionPtr transaction,
    ledger::client::RunDBTransactionCallback callback) {
  DCHECK(ledger_database_);
  ledger_database_.AsyncCall(&ledger::LedgerDatabase::RunTransaction)
      .WithArgs(std::move(transaction))
      .Then(base::BindOnce(&RewardsServiceImpl::OnRunDBTransaction, AsWeakPtr(),
                           std::move(callback)));
}

void RewardsServiceImpl::OnRunDBTransaction(
    ledger::client::RunDBTransactionCallback callback,
    ledger::mojom::DBCommandResponsePtr response) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::move(callback).Run(std::move(response));
}

void RewardsServiceImpl::GetCreateScript(
    ledger::client::GetCreateScriptCallback callback) {
  callback("", 0);
}

void RewardsServiceImpl::PendingContributionSaved(
    const ledger::mojom::Result result) {
  for (auto& observer : observers_) {
    observer.OnPendingContributionSaved(this, result);
  }
}

void RewardsServiceImpl::ForTestingSetTestResponseCallback(
    const GetTestResponseCallback& callback) {
  test_response_callback_ = callback;
}

void RewardsServiceImpl::GetAllMonthlyReportIds(
      GetAllMonthlyReportIdsCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         std::vector<std::string>());
  }

  bat_ledger_->GetAllMonthlyReportIds(std::move(callback));
}

void RewardsServiceImpl::GetAllContributions(
    GetAllContributionsCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         std::vector<ledger::mojom::ContributionInfoPtr>());
  }

  bat_ledger_->GetAllContributions(std::move(callback));
}

void RewardsServiceImpl::GetAllPromotions(GetAllPromotionsCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         std::vector<ledger::mojom::PromotionPtr>());
  }

  bat_ledger_->GetAllPromotions(
      base::BindOnce(&RewardsServiceImpl::OnGetAllPromotions,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnGetAllPromotions(
    GetAllPromotionsCallback callback,
    base::flat_map<std::string, ledger::mojom::PromotionPtr> promotions) {
  std::vector<ledger::mojom::PromotionPtr> list;
  for (const auto& promotion : promotions) {
    if (!promotion.second) {
      continue;
    }
    list.push_back(promotion.second->Clone());
  }
  std::move(callback).Run(std::move(list));
}

void RewardsServiceImpl::ClearAllNotifications() {
  notification_service_->DeleteAllNotifications(false);
}

void RewardsServiceImpl::CompleteReset(SuccessCallback callback) {
  resetting_rewards_ = true;

  StopNotificationTimers();
  notification_service_->DeleteAllNotifications(true);

  auto stop_callback =
      base::BindOnce(
          &RewardsServiceImpl::OnStopLedgerForCompleteReset,
          AsWeakPtr(),
          std::move(callback));
  StopLedger(std::move(stop_callback));
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

void RewardsServiceImpl::ExternalWalletConnected() const {
  for (auto& observer : observers_) {
    observer.OnExternalWalletConnected();
  }
}

void RewardsServiceImpl::ExternalWalletLoggedOut() const {
  for (auto& observer : observers_) {
    observer.OnExternalWalletLoggedOut();
  }
}

void RewardsServiceImpl::ExternalWalletReconnected() const {
  for (auto& observer : observers_) {
    observer.OnExternalWalletReconnected();
  }
}

void RewardsServiceImpl::DeleteLog(ledger::LegacyResultCallback callback) {
  diagnostic_log_->Delete(
      base::BindOnce(&RewardsServiceImpl::OnDiagnosticLogDeleted, AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnDiagnosticLogDeleted(
    ledger::LegacyResultCallback callback,
    bool success) {
  const auto result = success ? ledger::mojom::Result::LEDGER_OK
                              : ledger::mojom::Result::LEDGER_ERROR;
  callback(result);
}

void RewardsServiceImpl::GetEventLogs(GetEventLogsCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback),
                         std::vector<ledger::mojom::EventLogPtr>());
  }

  bat_ledger_->GetEventLogs(std::move(callback));
}

absl::optional<std::string> RewardsServiceImpl::EncryptString(
    const std::string& value) {
  std::string encrypted;
  if (OSCrypt::EncryptString(value, &encrypted))
    return encrypted;

  return {};
}

absl::optional<std::string> RewardsServiceImpl::DecryptString(
    const std::string& value) {
  std::string decrypted;
  if (OSCrypt::DecryptString(value, &decrypted))
    return decrypted;

  return {};
}

void RewardsServiceImpl::GetRewardsWallet(GetRewardsWalletCallback callback) {
  if (!Connected()) {
    return DeferCallback(FROM_HERE, std::move(callback), nullptr);
  }

  bat_ledger_->GetRewardsWallet(std::move(callback));
}

bool RewardsServiceImpl::IsBitFlyerRegion() const {
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
  if (IsBitFlyerRegion()) {
    return ledger::constant::kWalletBitflyer;
  }

  const std::string type =
      profile_->GetPrefs()->GetString(prefs::kExternalWalletType);

  if (IsValidWalletType(type)) {
    return type;
  }

  return ledger::constant::kWalletUphold;
}

void RewardsServiceImpl::SetExternalWalletType(const std::string& wallet_type) {
  if (IsValidWalletType(wallet_type)) {
    profile_->GetPrefs()->SetString(prefs::kExternalWalletType, wallet_type);
  }
}

}  // namespace brave_rewards
