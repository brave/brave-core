/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_service_impl.h"

#include <stdint.h>

#include <algorithm>
#include <functional>
#include <limits>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/containers/flat_map.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/i18n/time_formatting.h"
#include "base/json/json_reader.h"
#include "base/json/json_string_value_serializer.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/time/time.h"
#include "bat/ledger/global_constants.h"
#include "bat/ledger/ledger.h"
#include "bat/ledger/ledger_database.h"
#include "bat/ledger/mojom_structs.h"
#include "brave/base/containers/utils.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/ui/webui/brave_rewards_source.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/components/brave_ads/browser/buildflags/buildflags.h"
#include "brave/components/brave_ads/common/pref_names.h"
#include "brave/components/brave_rewards/browser/android_util.h"
#include "brave/components/brave_rewards/browser/file_util.h"
#include "brave/components/brave_rewards/browser/logging.h"
#include "brave/components/brave_rewards/browser/logging_util.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_p3a.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/browser/static_values.h"
#include "brave/components/brave_rewards/browser/switches.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/services/bat_ledger/public/cpp/ledger_client_mojo_bridge.h"
#include "chrome/browser/bitmap_fetcher/bitmap_fetcher_service_factory.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/country_codes/country_codes.h"
#include "components/favicon/core/favicon_service.h"
#include "components/favicon_base/favicon_types.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/common/service_manager_connection.h"
#include "net/base/escape.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/base/url_util.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/service_manager/public/cpp/connector.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/image/image.h"
#include "url/gurl.h"
#include "url/url_canon_stdstring.h"
#include "url/url_util.h"

#if defined(BRAVE_CHROMIUM_BUILD)
#include "brave/components/brave_rewards/resources/grit/brave_rewards_resources.h"
#include "components/grit/brave_components_resources.h"
#else
#include "components/grit/components_resources.h"
#endif

#if BUILDFLAG(ENABLE_GREASELION)
#include "brave/components/greaselion/browser/greaselion_service.h"
#endif

using net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES;
using std::placeholders::_1;
using std::placeholders::_2;

namespace brave_rewards {

static const unsigned int kRetriesCountOnNetworkChange = 1;

namespace {

const int kDiagnosticLogMaxVerboseLevel = 6;
const int kTailDiagnosticLogToNumLines = 20000;
const int kDiagnosticLogMaxFileSize = 10 * (1024 * 1024);
const char pref_prefix[] = "brave.rewards";

std::string URLMethodToRequestType(ledger::type::UrlMethod method) {
  switch (method) {
    case ledger::type::UrlMethod::GET:
      return "GET";
    case ledger::type::UrlMethod::POST:
      return "POST";
    case ledger::type::UrlMethod::PUT:
      return "PUT";
    case ledger::type::UrlMethod::PATCH:
      return "PATCH";
    default:
      NOTREACHED();
      return "GET";
  }
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

  const auto dict = base::DictionaryValue::From(std::move(value));
  if (!dict) {
    VLOG(0) << "Corrupted ledger state.";
    return result;
  }

  ExtractAndLogP3AStats(*dict);
  result.second = std::move(*dict);

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

ledger::type::InlineTipsPlatforms ConvertInlineTipStringToPlatform(
    const std::string& key) {
  if (key == "reddit") {
    return ledger::type::InlineTipsPlatforms::REDDIT;
  }
  if (key == "twitter") {
    return ledger::type::InlineTipsPlatforms::TWITTER;
  }
  if (key == "github") {
    return ledger::type::InlineTipsPlatforms::GITHUB;
  }

  NOTREACHED();
  return ledger::type::InlineTipsPlatforms::TWITTER;
}

bool ProcessPublisher(const GURL& url) {
  // we should always process publisher on desktop
  #if !defined(OS_ANDROID)
    return true;
  #endif

  const std::vector<GURL> excluded = {
      GURL("https://twitter.com")
  };

  for (const auto& domain : excluded) {
    if (net::registry_controlled_domains::SameDomainOrHost(
        url,
        domain,
        net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
      return false;
    }
  }

  return true;
}

std::string GetPrefPath(const std::string& name) {
  return base::StringPrintf("%s.%s", pref_prefix, name.c_str());
}

}  // namespace

bool IsMediaLink(const GURL& url,
                 const GURL& first_party_url,
                 const GURL& referrer) {
  return ledger::Ledger::IsMediaLink(url.spec(),
                                     first_party_url.spec(),
                                     referrer.spec());
}


// read comment about file pathes at src\base\files\file_path.h
#if defined(OS_WIN)
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
      file_task_runner_(base::CreateSequencedTaskRunner(
          {base::ThreadPool(), base::MayBlock(),
           base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
      diagnostic_log_path_(profile_->GetPath().Append(kDiagnosticLogPath)),
      ledger_state_path_(profile_->GetPath().Append(kLedger_state)),
      publisher_state_path_(profile_->GetPath().Append(kPublisher_state)),
      publisher_info_db_path_(profile->GetPath().Append(kPublisher_info_db)),
      publisher_list_path_(profile->GetPath().Append(kPublishers_list)),
      notification_service_(new RewardsNotificationServiceImpl(profile)),
      next_timer_id_(0) {
  // Set up the rewards data source
  content::URLDataSource::Add(profile_,
                              std::make_unique<BraveRewardsSource>(profile_));
  ready_ = std::make_unique<base::OneShotEvent>();
}

RewardsServiceImpl::~RewardsServiceImpl() {
  if (ledger_database_) {
    file_task_runner_->DeleteSoon(FROM_HERE, ledger_database_.release());
  }
  StopNotificationTimers();
}

void RewardsServiceImpl::ConnectionClosed() {
  if (!profile_->GetPrefs()->GetBoolean(prefs::kEnabled)) {
    BLOG(1, "Rewards not enabled");
    return;
  }

  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(FROM_HERE,
      base::BindOnce(&RewardsServiceImpl::StartLedger, AsWeakPtr()),
      base::TimeDelta::FromSeconds(1));
}

bool RewardsServiceImpl::IsInitialized() {
  return Connected() && ready_->is_signaled();
}

void RewardsServiceImpl::Init(
    std::unique_ptr<RewardsServiceObserver> extension_observer,
    std::unique_ptr<RewardsServicePrivateObserver> private_observer,
    std::unique_ptr<RewardsNotificationServiceObserver> notification_observer) {
  notification_service_->Init(std::move(notification_observer));
  AddObserver(notification_service_.get());

  if (extension_observer) {
    extension_observer_ = std::move(extension_observer);
    AddObserver(extension_observer_.get());
  }

  if (private_observer) {
    private_observer_ = std::move(private_observer);
    private_observers_.AddObserver(private_observer_.get());
  }

  if (profile_->GetPrefs()->GetBoolean(prefs::kEnabled)) {
    StartLedger();
  }
}

void RewardsServiceImpl::StartLedger() {
  if (Connected()) {
    BLOG(1, "Ledger process is already running");
    return;
  }

  ledger_database_.reset(
      ledger::LedgerDatabase::CreateInstance(publisher_info_db_path_));

  BLOG(1, "Starting ledger process");

  auto* connection = content::ServiceManagerConnection::GetForProcess();
  if (!connection) {
    BLOG(1, "Connection not found");
    return;
  }

  if (!bat_ledger_service_.is_bound()) {
    connection->GetConnector()->Connect(
        bat_ledger::mojom::kServiceName,
        bat_ledger_service_.BindNewPipeAndPassReceiver());

    bat_ledger_service_.set_disconnect_handler(
      base::Bind(&RewardsServiceImpl::ConnectionClosed, AsWeakPtr()));
  }

  ledger::type::Environment environment = ledger::type::Environment::STAGING;
  // Environment
  #if defined(OFFICIAL_BUILD) && defined(OS_ANDROID)
    environment = GetServerEnvironmentForAndroid();
  #elif defined(OFFICIAL_BUILD)
    environment = ledger::type::Environment::PRODUCTION;
  #endif
  SetEnvironment(environment);

  SetDebug(false);

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  if (command_line.HasSwitch(switches::kRewards)) {
    std::string options = command_line.GetSwitchValueASCII(switches::kRewards);

    if (!options.empty()) {
      HandleFlags(options);
    }
  }

  bat_ledger_service_->Create(
      bat_ledger_client_receiver_.BindNewEndpointAndPassRemote(),
      bat_ledger_.BindNewEndpointAndPassReceiver(),
      base::BindOnce(&RewardsServiceImpl::OnCreate, AsWeakPtr()));
}

void RewardsServiceImpl::OnCreate() {
  if (!Connected()) {
    return;
  }

  PrepareLedgerEnvForTesting();

  auto callback = base::BindOnce(&RewardsServiceImpl::OnWalletInitialized,
      AsWeakPtr());

  bat_ledger_->Initialize(false, std::move(callback));
}

void RewardsServiceImpl::OnResult(
    ledger::ResultCallback callback,
    const ledger::type::Result result) {
  callback(result);
}

void RewardsServiceImpl::MaybeShowBackupNotification(uint64_t boot_stamp) {
  PrefService* pref_service = profile_->GetPrefs();
  const base::TimeDelta interval = pref_service->GetTimeDelta(
      prefs::kBackupNotificationInterval);

  // Don't display notification if it has already been shown or if
  // the balance is zero.
  if (interval.is_zero() ||
      !pref_service->GetBoolean(prefs::kUserHasFunded)) {
    return;
  }

  auto clear_backup_interval = [pref_service]() {
    pref_service->SetTimeDelta(
        prefs::kBackupNotificationInterval,
        base::TimeDelta());
  };

  // Don't display notification if a backup has already succeeded.
  if (pref_service->GetBoolean(prefs::kBackupSucceeded)) {
    clear_backup_interval();
    return;
  }

  // Don't display notification if user has a verified wallet.
  for (auto& pair : GetExternalWallets()) {
    DCHECK(pair.second);
    switch (pair.second->status) {
      case ledger::type::WalletStatus::VERIFIED:
      case ledger::type::WalletStatus::DISCONNECTED_VERIFIED:
        clear_backup_interval();
        return;
      default:
        break;
    }
  }

  const base::Time boot_time = base::Time::FromDoubleT(boot_stamp);
  const base::TimeDelta elapsed = base::Time::Now() - boot_time;
  if (elapsed > interval) {
    clear_backup_interval();
    notification_service_->AddNotification(
        RewardsNotificationService::REWARDS_NOTIFICATION_BACKUP_WALLET,
        RewardsNotificationService::RewardsNotificationArgs(),
        "rewards_notification_backup_wallet");
  }
}

void RewardsServiceImpl::MaybeShowAddFundsNotification(
    uint64_t reconcile_stamp) {
  // Show add funds notification if reconciliation will occur in the
  // next 3 days and balance is too low.
  base::Time now = base::Time::Now();
  if (reconcile_stamp - now.ToDoubleT() <
      3 * base::Time::kHoursPerDay * base::Time::kSecondsPerHour) {
    if (ShouldShowNotificationAddFunds()) {
      MaybeShowNotificationAddFunds();
    }
  }
}

void RewardsServiceImpl::OnCreateWallet(
    CreateWalletCallback callback,
    ledger::type::Result result) {
  OnWalletInitialized(result);
  std::move(callback).Run(result);
}

void RewardsServiceImpl::AddPrivateObserver(
    RewardsServicePrivateObserver* observer) {
  private_observers_.AddObserver(observer);
}

void RewardsServiceImpl::RemovePrivateObserver(
    RewardsServicePrivateObserver* observer) {
  private_observers_.RemoveObserver(observer);
}

void RewardsServiceImpl::CreateWallet(CreateWalletCallback callback) {
  if (!ready_->is_signaled()) {
    StartLedger();
    ready_->Post(
        FROM_HERE,
        base::BindOnce(&brave_rewards::RewardsService::CreateWallet,
            AsWeakPtr(),
            std::move(callback)));
    return;
  }

  if (!Connected()) {
    return;
  }

  auto on_create = base::BindOnce(&RewardsServiceImpl::OnCreateWallet,
      AsWeakPtr(),
      std::move(callback));

#if !defined(OS_ANDROID)
  bat_ledger_->CreateWallet(std::move(on_create));
#else
  safetynet_check::ClientAttestationCallback attest_callback =
      base::BindOnce(&RewardsServiceImpl::CreateWalletAttestationResult,
          AsWeakPtr(),
          std::move(on_create));
  safetynet_check_runner_.performSafetynetCheck("",
      std::move(attest_callback), true);
#endif
}

#if defined(OS_ANDROID)
void RewardsServiceImpl::CreateWalletAttestationResult(
    bat_ledger::mojom::BatLedger::CreateWalletCallback callback,
    const bool token_received,
    const std::string& result_string,
    const bool attestation_passed) {
  if (!Connected()) {
    return;
  }

  if (!token_received) {
    BLOG(0, "CreateWalletAttestationResult error: " << result_string);
    OnWalletInitialized(ledger::type::Result::LEDGER_ERROR);
    return;
  }
  if (!attestation_passed) {
    OnWalletInitialized(ledger::type::Result::SAFETYNET_ATTESTATION_FAILED);
    return;
  }
  bat_ledger_->CreateWallet(std::move(callback));
}
#endif

void RewardsServiceImpl::GetPublisherInfoList(
    uint32_t start,
    uint32_t limit,
    uint64_t min_visit_time,
    uint64_t reconcile_stamp,
    bool allow_non_verified,
    uint32_t min_visits,
    const GetPublisherInfoListCallback& callback) {
  if (!Connected()) {
    return;
  }

  auto filter = ledger::type::ActivityInfoFilter::New();
  filter->min_duration = min_visit_time;
  auto pair =
      ledger::type::ActivityInfoFilterOrderPair::New("ai.percent", false);
  filter->order_by.push_back(std::move(pair));
  filter->reconcile_stamp = reconcile_stamp;
  filter->excluded = ledger::type::ExcludeFilter::FILTER_ALL_EXCEPT_EXCLUDED;
  filter->percent = 1;
  filter->non_verified = allow_non_verified;
  filter->min_visits = min_visits;

  bat_ledger_->GetActivityInfoList(
      start,
      limit,
      std::move(filter),
      base::BindOnce(&RewardsServiceImpl::OnGetPublisherInfoList,
                     AsWeakPtr(),
                     callback));
}

void RewardsServiceImpl::GetExcludedList(
    const GetPublisherInfoListCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetExcludedList(base::BindOnce(
      &RewardsServiceImpl::OnGetPublisherInfoList,
      AsWeakPtr(),
      callback));
}

void RewardsServiceImpl::OnGetPublisherInfoList(
    const GetPublisherInfoListCallback& callback,
    ledger::type::PublisherInfoList list) {
  callback.Run(std::move(list));
}

void RewardsServiceImpl::OnLoad(SessionID tab_id, const GURL& url) {
  if (!Connected()) {
    return;
  }

  if (!ProcessPublisher(url)) {
    return;
  }

  auto origin = url.GetOrigin();
  const std::string baseDomain =
      GetDomainAndRegistry(origin.host(), INCLUDE_PRIVATE_REGISTRIES);

  if (baseDomain == "")
    return;

  const std::string publisher_url = origin.scheme() + "://" + baseDomain + "/";

  ledger::type::VisitDataPtr data = ledger::type::VisitData::New();
  data->tld = data->name = baseDomain;
  data->domain = origin.host(),
  data->path = url.path();
  data->tab_id = tab_id.id();
  data->url = publisher_url;
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

void RewardsServiceImpl::OnPostData(SessionID tab_id,
                                    const GURL& url,
                                    const GURL& first_party_url,
                                    const GURL& referrer,
                                    const std::string& post_data) {
  if (!Connected()) {
    return;
  }

  if (!ProcessPublisher(url)) {
    return;
  }

  std::string output;
  url::RawCanonOutputW<1024> canonOutput;
  url::DecodeURLEscapeSequences(post_data.c_str(),
                                post_data.length(),
                                url::DecodeURLMode::kUTF8OrIsomorphic,
                                &canonOutput);
  output = base::UTF16ToUTF8(base::StringPiece16(canonOutput.data(),
                                                 canonOutput.length()));

  if (output.empty())
    return;

  ledger::type::VisitDataPtr data = ledger::type::VisitData::New();
  data->path = url.spec(),
  data->tab_id = tab_id.id();

  bat_ledger_->OnPostData(url.spec(),
                          first_party_url.spec(),
                          referrer.spec(),
                          output,
                          std::move(data));
}

void RewardsServiceImpl::OnXHRLoad(SessionID tab_id,
                                   const GURL& url,
                                   const GURL& first_party_url,
                                   const GURL& referrer) {
  if (!Connected()) {
    return;
  }

  if (!ProcessPublisher(url)) {
    return;
  }

  std::map<std::string, std::string> parts;

  for (net::QueryIterator it(url); !it.IsAtEnd(); it.Advance()) {
    parts[it.GetKey()] = it.GetUnescapedValue();
  }

  ledger::type::VisitDataPtr data = ledger::type::VisitData::New();
  data->path = url.spec();
  data->tab_id = tab_id.id();

  bat_ledger_->OnXHRLoad(tab_id.id(),
                         url.spec(),
                         base::MapToFlatMap(parts),
                         first_party_url.spec(),
                         referrer.spec(),
                         std::move(data));
}

void RewardsServiceImpl::OnRestorePublishers(
    const ledger::type::Result result) {
  if (result != ledger::type::Result::LEDGER_OK) {
    return;
  }

  for (auto& observer : observers_) {
    observer.OnExcludedSitesChanged(
      this, "-1", static_cast<int>(ledger::type::PublisherExclude::ALL));
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
  return net::EscapeQueryParamValue(value, false);
}

void RewardsServiceImpl::Shutdown() {
  RemoveObserver(notification_service_.get());

  if (extension_observer_) {
    RemoveObserver(extension_observer_.get());
  }

  if (private_observer_) {
    private_observers_.RemoveObserver(private_observer_.get());
  }

  BitmapFetcherService* image_service =
      BitmapFetcherServiceFactory::GetForBrowserContext(profile_);
  if (image_service) {
    for (auto mapping : current_media_fetchers_) {
      image_service->CancelRequest(mapping.second);
    }
  }

  for (auto* const loader : url_loaders_) {
    delete loader;
  }
  url_loaders_.clear();

  bat_ledger_.reset();
  RewardsService::Shutdown();
}

void RewardsServiceImpl::OnWalletInitialized(ledger::type::Result result) {
  if (result == ledger::type::Result::WALLET_CREATED ||
      result == ledger::type::Result::LEDGER_OK) {
    is_wallet_initialized_ = true;
  }

  if (!ready_->is_signaled()) {
    ready_->Signal();
  }

  if (is_wallet_initialized_) {
    StartNotificationTimers(true);
  }

  if (result == ledger::type::Result::WALLET_CREATED) {
    // Record P3A:
    RecordWalletBalanceP3A(true, true, 0);
#if BUILDFLAG(BRAVE_ADS_ENABLED)
    const bool ads_enabled =
        profile_->GetPrefs()->GetBoolean(brave_ads::prefs::kEnabled);
    RecordAdsState(ads_enabled ? AdsP3AState::kAdsEnabled
                               : AdsP3AState::kAdsDisabled);
#endif
  }

  EnableGreaseLion(profile_->GetPrefs()->GetBoolean(
      prefs::kEnabled));

  for (auto& observer : observers_) {
    observer.OnWalletInitialized(this, result);
  }
}

void RewardsServiceImpl::OnGetAutoContributeProperties(
    const GetAutoContributePropertiesCallback& callback,
    ledger::type::AutoContributePropertiesPtr properties) {
  if (!properties) {
    callback.Run(nullptr);
    return;
  }

  callback.Run(std::move(properties));
}

void RewardsServiceImpl::OnGetRewardsInternalsInfo(
    GetRewardsInternalsInfoCallback callback,
    ledger::type::RewardsInternalsInfoPtr info) {
  std::move(callback).Run(std::move(info));
}

void RewardsServiceImpl::GetAutoContributeProperties(
    const GetAutoContributePropertiesCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetAutoContributeProperties(base::BindOnce(
        &RewardsServiceImpl::OnGetAutoContributeProperties,
        AsWeakPtr(),
        callback));
}

void RewardsServiceImpl::OnReconcileComplete(
    const ledger::type::Result result,
    ledger::type::ContributionInfoPtr contribution) {
  if (result == ledger::type::Result::LEDGER_OK &&
      contribution->type == ledger::type::RewardsType::RECURRING_TIP) {
    MaybeShowNotificationTipsPaid();
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
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&LoadStateOnFileTaskRunner, ledger_state_path_),
      base::BindOnce(&RewardsServiceImpl::OnLedgerStateLoaded,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnLedgerStateLoaded(
    ledger::client::OnLoadCallback callback,
    std::pair<std::string, base::Value> state) {
  if (!Connected()) {
    return;
  }

  if (state.second.is_dict()) {
    // Record stats.
    RecordBackendP3AStats();
    MaybeRecordInitialAdsP3AState(profile_->GetPrefs());
  }
  if (state.first.empty()) {
    RecordNoWalletCreatedForAllMetrics();
  }

  // Run callbacks.
  const std::string& data = state.first;
  callback(data.empty() ? ledger::type::Result::NO_LEDGER_STATE
                        : ledger::type::Result::LEDGER_OK,
                        data);

  bat_ledger_->GetRewardsMainEnabled(
      base::BindOnce(&RewardsServiceImpl::StartNotificationTimers,
        AsWeakPtr()));
}

void RewardsServiceImpl::LoadPublisherState(
    ledger::client::OnLoadCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&LoadOnFileTaskRunner, publisher_state_path_),
      base::BindOnce(&RewardsServiceImpl::OnPublisherStateLoaded,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnPublisherStateLoaded(
    ledger::client::OnLoadCallback callback,
    const std::string& data) {
  if (!Connected()) {
    return;
  }

  callback(
      data.empty() ? ledger::type::Result::NO_PUBLISHER_STATE
                   : ledger::type::Result::LEDGER_OK,
      data);
}

void RewardsServiceImpl::LoadURL(
    ledger::type::UrlRequestPtr request,
    ledger::client::LoadURLCallback callback) {
  if (!request || request->url.empty()) {
    ledger::type::UrlResponse response;
    response.status_code = net::HTTP_BAD_REQUEST;
    callback(response);
    return;
  }

  GURL parsed_url(request->url);
  if (!parsed_url.is_valid()) {
    ledger::type::UrlResponse response;
    response.url = request->url;
    response.status_code = net::HTTP_BAD_REQUEST;
    callback(response);
    return;
  }

  if (test_response_callback_) {
    std::string test_response;
    std::map<std::string, std::string> test_headers;
    int response_status_code = net::HTTP_OK;
    test_response_callback_.Run(
        request->url,
        static_cast<int>(request->method),
        &response_status_code,
        &test_response,
        &test_headers);

    ledger::type::UrlResponse response;
    response.url = request->url;
    response.status_code = response_status_code;
    response.body = test_response;
    response.headers = base::MapToFlatMap(test_headers);
    callback(response);
    return;
  }

  auto net_request = std::make_unique<network::ResourceRequest>();
  net_request->url = parsed_url;
  net_request->method = URLMethodToRequestType(request->method);

  // Loading Twitter requires credentials
  if (net_request->url.DomainIs("twitter.com")) {
    net_request->credentials_mode = network::mojom::CredentialsMode::kInclude;

#if defined(OS_ANDROID)
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

  network::SimpleURLLoader* loader = network::SimpleURLLoader::Create(
      std::move(net_request),
      GetNetworkTrafficAnnotationTagForURLLoad()).release();
  loader->SetAllowHttpErrorResults(true);
  url_loaders_.insert(loader);
  loader->SetRetryOptions(kRetriesCountOnNetworkChange,
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);

  if (!request->content.empty()) {
    loader->AttachStringForUpload(request->content, request->content_type);
  }

  loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      content::BrowserContext::GetDefaultStoragePartition(profile_)
          ->GetURLLoaderFactoryForBrowserProcess().get(),
      base::BindOnce(&RewardsServiceImpl::OnURLLoaderComplete,
                     base::Unretained(this),
                     loader,
                     callback));
}

void RewardsServiceImpl::OnURLLoaderComplete(
    network::SimpleURLLoader* loader,
    ledger::client::LoadURLCallback callback,
    std::unique_ptr<std::string> response_body) {

  DCHECK(url_loaders_.find(loader) != url_loaders_.end());
  url_loaders_.erase(loader);

  if (!Connected()) {
    return;
  }

  std::unique_ptr<network::SimpleURLLoader> scoped_loader(loader);

  ledger::type::UrlResponse response;
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

  std::map<std::string, std::string> headers;
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

  callback(response);
}

void RewardsServiceImpl::OnGetRewardsParameters(
    GetRewardsParametersCallback callback,
    ledger::type::RewardsParametersPtr parameters) {
  std::move(callback).Run(std::move(parameters));
}

void RewardsServiceImpl::GetRewardsParameters(
    GetRewardsParametersCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetRewardsParameters(
      base::BindOnce(&RewardsServiceImpl::OnGetRewardsParameters,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnFetchPromotions(
    const ledger::type::Result result,
    ledger::type::PromotionList promotions) {
  for (auto& observer : observers_) {
    ledger::type::PromotionList promotions_clone;
    for (auto& promotion : promotions) {
      promotions_clone.push_back(promotion->Clone());
    }
    observer.OnFetchPromotions(this, result, std::move(promotions_clone));
  }
}

void RewardsServiceImpl::FetchPromotions() {
  if (!Connected()) {
    return;
  }

  bat_ledger_->FetchPromotions(base::BindOnce(
      &RewardsServiceImpl::OnFetchPromotions,
      AsWeakPtr()));
}

void ParseCaptchaResponse(
    const std::string& response,
    std::string* image,
    std::string* id,
    std::string* hint) {
  base::Optional<base::Value> value = base::JSONReader::Read(response);
  if (!value || !value->is_dict()) {
    return;
  }

  base::DictionaryValue* dictionary = nullptr;
  if (!value->GetAsDictionary(&dictionary)) {
    return;
  }

  auto* captcha_image = dictionary->FindKey("captchaImage");
  if (!captcha_image || !captcha_image->is_string()) {
    return;
  }

  auto* captcha_hint = dictionary->FindKey("hint");
  if (!captcha_hint || !captcha_hint->is_string()) {
    return;
  }

  auto* captcha_id = dictionary->FindKey("captchaId");
  if (!captcha_id || !captcha_id->is_string()) {
    return;
  }

  *image = captcha_image->GetString();
  *hint = captcha_hint->GetString();
  *id = captcha_id->GetString();
}

void RewardsServiceImpl::OnClaimPromotion(
    ClaimPromotionCallback callback,
    const ledger::type::Result result,
    const std::string& response) {
  std::string image;
  std::string hint;
  std::string id;

  if (result != ledger::type::Result::LEDGER_OK) {
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

void RewardsServiceImpl::AttestationAndroid(
    const std::string& promotion_id,
    AttestPromotionCallback callback,
    const ledger::type::Result result,
    const std::string& nonce) {
  if (result != ledger::type::Result::LEDGER_OK) {
    std::move(callback).Run(result, nullptr);
    return;
  }

  if (nonce.empty()) {
    std::move(callback).Run(ledger::type::Result::LEDGER_ERROR, nullptr);
    return;
  }

  #if defined(OS_ANDROID)
    auto attest_callback =
        base::BindOnce(&RewardsServiceImpl::OnAttestationAndroid,
            AsWeakPtr(),
            promotion_id,
            std::move(callback),
            nonce);
    safetynet_check_runner_.performSafetynetCheck(
        nonce,
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
  if (!Connected()) {
    return;
  }

  if (!token_received) {
    std::move(callback).Run(ledger::type::Result::LEDGER_ERROR, nullptr);
  }

  base::Value solution(base::Value::Type::DICTIONARY);
  solution.SetStringKey("nonce", nonce);
  solution.SetStringKey("token", token);

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
    return;
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
    return;
  }

  auto claim_callback = base::BindOnce(&RewardsServiceImpl::AttestationAndroid,
      AsWeakPtr(),
      promotion_id,
      std::move(callback));

  bat_ledger_->ClaimPromotion(promotion_id, "", std::move(claim_callback));
}

void RewardsServiceImpl::GetWalletPassphrase(
    const GetWalletPassphraseCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetWalletPassphrase(callback);
}

void RewardsServiceImpl::RecoverWallet(const std::string& passPhrase) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->RecoverWallet(passPhrase, base::BindOnce(
      &RewardsServiceImpl::OnRecoverWallet,
      AsWeakPtr()));
}

void RewardsServiceImpl::OnRecoverWallet(const ledger::type::Result result) {
  for (auto& observer : observers_) {
    observer.OnRecoverWallet(this, result);
  }
}

void RewardsServiceImpl::AttestPromotion(
    const std::string& promotion_id,
    const std::string& solution,
    AttestPromotionCallback callback) {
  if (!Connected()) {
    return;
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
    const ledger::type::Result result,
    ledger::type::PromotionPtr promotion) {
  if (result != ledger::type::Result::LEDGER_OK) {
    std::move(callback).Run(result, nullptr);
    return;
  }

  for (auto& observer : observers_) {
    observer.OnPromotionFinished(this, result, promotion->Clone());
  }

  if (promotion->type == ledger::type::PromotionType::ADS) {
    auto* ads_service = brave_ads::AdsServiceFactory::GetForProfile(profile_);
    if (ads_service) {
      ads_service->UpdateAdRewards(/*should_reconcile*/ true);
    }
  }

  std::move(callback).Run(result, std::move(promotion));
}

void RewardsServiceImpl::GetReconcileStamp(
    const GetReconcileStampCallback& callback)  {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetReconcileStamp(callback);
}

void RewardsServiceImpl::SetRewardsMainEnabled(bool enabled) {
  if (enabled) {
    StartLedger();

    if (!ready_->is_signaled()) {
      ready_->Post(
          FROM_HERE,
          base::Bind(&brave_rewards::RewardsService::SetRewardsMainEnabled,
              base::Unretained(this),
              enabled));
      return;
    }
  }

  if (!Connected()) {
    return;
  }

  bat_ledger_->SetRewardsMainEnabled(enabled);
  TriggerOnRewardsMainEnabled(enabled);

  if (!enabled) {
    RecordRewardsDisabledForSomeMetrics();
    StopLedger(base::DoNothing());
  }
}

void RewardsServiceImpl::EnableGreaseLion(const bool enabled) {
  #if BUILDFLAG(ENABLE_GREASELION)
    if (greaselion_service_) {
      greaselion_service_->SetFeatureEnabled(greaselion::REWARDS, enabled);
      greaselion_service_->SetFeatureEnabled(greaselion::TWITTER_TIPS, enabled);
    }
  #endif
}

void RewardsServiceImpl::StopLedger(StopLedgerCallback callback) {
  BLOG(1, "Shutting down ledger process");
  if (!Connected()) {
    BLOG(1, "Ledger process not running");
    OnStopLedger(std::move(callback), ledger::type::Result::LEDGER_OK);
    return;
  }

  bat_ledger_->Shutdown(base::BindOnce(
      &RewardsServiceImpl::OnStopLedger,
      AsWeakPtr(),
      std::move(callback)));
}

void RewardsServiceImpl::OnStopLedger(
    StopLedgerCallback callback,
    const ledger::type::Result result) {
  BLOG_IF(
      1,
      result != ledger::type::Result::LEDGER_OK,
      "Ledger process was not shut down successfully");
  Reset();
  BLOG(1, "Successfully shutdown ledger");
  std::move(callback).Run(result);
}

void RewardsServiceImpl::OnStopLedgerForCompleteReset(
    SuccessCallback callback,
    const ledger::type::Result result) {
  profile_->GetPrefs()->ClearPrefsWithPrefixSilently(pref_prefix);

  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(),
      FROM_HERE,
      base::BindOnce(
          &RewardsServiceImpl::ResetOnFilesTaskRunner,
          base::Unretained(this)),
      base::BindOnce(
          &RewardsServiceImpl::OnCompleteReset,
          AsWeakPtr(),
          std::move(callback)));
}

bool RewardsServiceImpl::ResetOnFilesTaskRunner() {
  // Close any open files before deleting them (required on Windows)
  diagnostic_log_.Close();

  const std::vector<base::FilePath> paths = {
    ledger_state_path_,
    publisher_state_path_,
    publisher_info_db_path_,
    diagnostic_log_path_,
    publisher_list_path_,
  };

  bool res = true;
  for (size_t i = 0; i < paths.size(); i++) {
    if (!base::DeleteFileRecursively(paths[i])) {
      res = false;
    }
  }

  return res;
}

void RewardsServiceImpl::Reset() {
  for (auto* const url_loader : url_loaders_) {
    delete url_loader;
  }
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
  is_wallet_initialized_ = false;
  ready_ = std::make_unique<base::OneShotEvent>();
  bool success =
      file_task_runner_->DeleteSoon(FROM_HERE, ledger_database_.release());
  BLOG_IF(1, !success, "Database was not released");
  BLOG(1, "Successfully reset rewards service");
}

void RewardsServiceImpl::GetRewardsMainEnabled(
    const GetRewardsMainEnabledCallback& callback) const {
  if (!Connected()) {
    std::move(callback).Run(
        profile_->GetPrefs()->GetBoolean(prefs::kEnabled));
    return;
  }

  bat_ledger_->GetRewardsMainEnabled(callback);
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

void RewardsServiceImpl::ClearState(const std::string& name) {
  profile_->GetPrefs()->ClearPref(GetPrefPath(name));
}

bool RewardsServiceImpl::GetBooleanOption(const std::string& name) const {
  DCHECK(!name.empty());

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
    const GetPublisherMinVisitTimeCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetPublisherMinVisitTime(callback);
}

void RewardsServiceImpl::SetPublisherMinVisitTime(
    int duration_in_seconds) const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetPublisherMinVisitTime(duration_in_seconds);
}

void RewardsServiceImpl::GetPublisherMinVisits(
    const GetPublisherMinVisitsCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetPublisherMinVisits(callback);
}

void RewardsServiceImpl::SetPublisherMinVisits(int visits) const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetPublisherMinVisits(visits);
}

void RewardsServiceImpl::GetPublisherAllowNonVerified(
    const GetPublisherAllowNonVerifiedCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetPublisherAllowNonVerified(callback);
}

void RewardsServiceImpl::SetPublisherAllowNonVerified(bool allow) const {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetPublisherAllowNonVerified(allow);
}

void RewardsServiceImpl::GetPublisherAllowVideos(
    const GetPublisherAllowVideosCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetPublisherAllowVideos(callback);
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
    return;
  }

  bat_ledger_->GetAutoContributeEnabled(std::move(callback));
}

void RewardsServiceImpl::SetAutoContributeEnabled(bool enabled) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SetAutoContributeEnabled(enabled);

  if (!enabled) {
    // Just record the disabled state.
    RecordAutoContributionsState(
        AutoContributionsP3AState::kWalletCreatedAutoContributeOff, 0);
  } else {
    // Need to query the DB for actual counts.
    RecordBackendP3AStats();
  }
}

void RewardsServiceImpl::OnAdsEnabled(
    bool ads_enabled) {
  for (auto& observer : observers_)
    observer.OnAdsEnabled(this, ads_enabled);
}

void RewardsServiceImpl::TriggerOnRewardsMainEnabled(
    bool rewards_main_enabled) {
  for (auto& observer : observers_)
    observer.OnRewardsMainEnabled(this, rewards_main_enabled);
}

void RewardsServiceImpl::OnGetBalanceReport(
    GetBalanceReportCallback callback,
    const ledger::type::Result result,
    ledger::type::BalanceReportInfoPtr report) {
  std::move(callback).Run(result, std::move(report));
}

void RewardsServiceImpl::GetBalanceReport(
    const uint32_t month,
    const uint32_t year,
    GetBalanceReportCallback callback) {
  if (!Connected()) {
    std::move(callback).Run(ledger::type::Result::LEDGER_OK, nullptr);
    return;
  }

  bat_ledger_->GetBalanceReport(
      static_cast<ledger::type::ActivityMonth>(month),
      year,
      base::BindOnce(&RewardsServiceImpl::OnGetBalanceReport,
        AsWeakPtr(),
        std::move(callback)));
}

void RewardsServiceImpl::IsWalletCreated(
    const IsWalletCreatedCallback& callback) {
  if (!Connected()) {
    const auto stamp =
        profile_->GetPrefs()->GetUint64(prefs::kCreationStamp);
    callback.Run(stamp != 0u);
    return;
  }

  bat_ledger_->IsWalletCreated(callback);
}

void RewardsServiceImpl::GetPublisherActivityFromUrl(
    uint64_t windowId,
    const std::string& url,
    const std::string& favicon_url,
    const std::string& publisher_blob) {
  GURL parsed_url(url);

  if (!parsed_url.is_valid() || !ProcessPublisher(parsed_url)) {
    return;
  }

  auto origin = parsed_url.GetOrigin();
  std::string baseDomain =
      GetDomainAndRegistry(origin.host(), INCLUDE_PRIVATE_REGISTRIES);

  if (baseDomain == "") {
    ledger::type::PublisherInfoPtr info;
    OnPanelPublisherInfo(
        ledger::type::Result::NOT_FOUND,
        std::move(info),
        windowId);
    return;
  }

  if (!Connected()) {
    return;
  }

  ledger::type::VisitDataPtr visit_data = ledger::type::VisitData::New();
  visit_data->domain = visit_data->name = baseDomain;
  visit_data->path = parsed_url.PathForRequest();
  visit_data->url = origin.spec();
  visit_data->favicon_url = favicon_url;

  bat_ledger_->GetPublisherActivityFromUrl(
        windowId,
        std::move(visit_data),
        publisher_blob);
}

void RewardsServiceImpl::OnPanelPublisherInfo(
    const ledger::type::Result result,
    ledger::type::PublisherInfoPtr info,
    uint64_t windowId) {
  if (result != ledger::type::Result::LEDGER_OK &&
      result != ledger::type::Result::NOT_FOUND) {
    return;
  }

  for (auto& observer : private_observers_)
    observer.OnPanelPublisherInfo(this,
                                  result,
                                  info.get(),
                                  windowId);
}

void RewardsServiceImpl::GetAutoContributionAmount(
    const GetAutoContributionAmountCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetAutoContributionAmount(callback);
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
    BLOG(1, "Already fetching favicon: " << url);
    return;
  }

  BitmapFetcherService* image_service =
      BitmapFetcherServiceFactory::GetForBrowserContext(profile_);
  if (image_service) {
    current_media_fetchers_[url] = image_service->RequestImage(
        parsedUrl,
        base::Bind(&RewardsServiceImpl::OnFetchFavIconCompleted,
                   base::Unretained(this), callback, favicon_key, parsedUrl),
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
    ledger::client::FetchIconCallback callback, bool success) {
  if (!Connected()) {
    return;
  }

  callback(success, favicon_url);
}

void RewardsServiceImpl::GetPublisherBanner(
    const std::string& publisher_id,
    GetPublisherBannerCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetPublisherBanner(publisher_id,
      base::BindOnce(&RewardsServiceImpl::OnPublisherBanner,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnPublisherBanner(
    GetPublisherBannerCallback callback,
    ledger::type::PublisherBannerPtr banner) {
  std::move(callback).Run(std::move(banner));
}

void RewardsServiceImpl::OnSaveRecurringTip(
    SaveRecurringTipCallback callback,
    const ledger::type::Result result) {
  bool success = result == ledger::type::Result::LEDGER_OK;

  for (auto& observer : observers_) {
    observer.OnRecurringTipSaved(this, success);
  }

  std::move(callback).Run(success);
}

void RewardsServiceImpl::SaveRecurringTip(
    const std::string& publisher_key,
    const double amount,
    SaveRecurringTipCallback callback) {
  if (!Connected()) {
    return;
  }

  ledger::type::RecurringTipPtr info = ledger::type::RecurringTip::New();
  info->publisher_key = publisher_key;
  info->amount = amount;
  info->created_at = GetCurrentTimestamp();

  bat_ledger_->SaveRecurringTip(
      std::move(info),
      base::BindOnce(&RewardsServiceImpl::OnSaveRecurringTip,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnMediaInlineInfoSaved(
    SaveMediaInfoCallback callback,
    const ledger::type::Result result,
    ledger::type::PublisherInfoPtr publisher) {
  std::move(callback).Run(std::move(publisher));
}

void RewardsServiceImpl::SaveInlineMediaInfo(
    const std::string& media_type,
    const std::map<std::string, std::string>& args,
    SaveMediaInfoCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->SaveMediaInfo(
      media_type,
      base::MapToFlatMap(args),
      base::BindOnce(&RewardsServiceImpl::OnMediaInlineInfoSaved,
                    AsWeakPtr(),
                    std::move(callback)));
}

void RewardsServiceImpl::UpdateMediaDuration(
    const std::string& publisher_key,
    uint64_t duration) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->UpdateMediaDuration(publisher_key, duration);
}

void RewardsServiceImpl::GetPublisherInfo(
    const std::string& publisher_key,
    GetPublisherInfoCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetPublisherInfo(
      publisher_key,
      base::BindOnce(&RewardsServiceImpl::OnPublisherInfo,
          AsWeakPtr(),
          std::move(callback)));
}

void RewardsServiceImpl::OnPublisherInfo(
    GetPublisherInfoCallback callback,
    const ledger::type::Result result,
    ledger::type::PublisherInfoPtr info) {
  const auto result_converted = static_cast<int>(result);
  if (result != ledger::type::Result::LEDGER_OK) {
    std::move(callback).Run(result_converted, nullptr);
    return;
  }

  brave_rewards::PublisherInfo rewards_publisher_info =
      PublisherInfoToRewardsPublisherInfo(*info);

  auto rewards_publisher_info_ptr =
      std::make_unique<brave_rewards::PublisherInfo>(rewards_publisher_info);
  std::move(callback).Run(
      result_converted,
      std::move(rewards_publisher_info_ptr));
}

void RewardsServiceImpl::GetPublisherPanelInfo(
    const std::string& publisher_key,
    GetPublisherInfoCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetPublisherPanelInfo(
      publisher_key,
      base::BindOnce(&RewardsServiceImpl::OnPublisherPanelInfo,
          AsWeakPtr(),
          std::move(callback)));
}

void RewardsServiceImpl::OnPublisherPanelInfo(
    GetPublisherInfoCallback callback,
    const ledger::type::Result result,
    ledger::type::PublisherInfoPtr info) {
  const auto result_converted = static_cast<int>(result);
  if (result != ledger::type::Result::LEDGER_OK) {
    std::move(callback).Run(result_converted, nullptr);
    return;
  }

  brave_rewards::PublisherInfo rewards_publisher_info =
      PublisherInfoToRewardsPublisherInfo(*info);

  auto rewards_publisher_info_ptr =
      std::make_unique<brave_rewards::PublisherInfo>(rewards_publisher_info);
  std::move(callback).Run(
      result_converted,
      std::move(rewards_publisher_info_ptr));
}

void RewardsServiceImpl::SavePublisherInfo(
    const uint64_t window_id,
    std::unique_ptr<brave_rewards::PublisherInfo> publisher_info,
    SavePublisherInfoCallback callback) {
  if (!Connected()) {
    return;
  }

  auto ledger_publisher_info = ledger::PublisherInfo::New();
  ledger_publisher_info->id = publisher_info->id;
  ledger_publisher_info->duration = publisher_info->duration;
  ledger_publisher_info->score = publisher_info->score;
  ledger_publisher_info->visits = publisher_info->visits;
  ledger_publisher_info->percent = publisher_info->percent;
  ledger_publisher_info->weight = publisher_info->weight;
  ledger_publisher_info->excluded =
      static_cast<ledger::PublisherExclude>(publisher_info->excluded);
  ledger_publisher_info->reconcile_stamp = publisher_info->reconcile_stamp;
  ledger_publisher_info->status =
      static_cast<ledger::PublisherStatus>(publisher_info->status);
  ledger_publisher_info->status_updated_at = publisher_info->status_updated_at;
  ledger_publisher_info->name = publisher_info->name;
  ledger_publisher_info->url = publisher_info->url;
  ledger_publisher_info->provider = publisher_info->provider;
  ledger_publisher_info->favicon_url = publisher_info->favicon_url;

  bat_ledger_->SavePublisherInfo(
      window_id,
      std::move(ledger_publisher_info),
      base::BindOnce(&RewardsServiceImpl::OnSavePublisherInfo,
          AsWeakPtr(),
          std::move(callback)));
}

void RewardsServiceImpl::OnSavePublisherInfo(
    SavePublisherInfoCallback callback,
    const ledger::type::Result result) {
  const auto result_converted = static_cast<int>(result);
  std::move(callback).Run(result_converted);
}

void RewardsServiceImpl::OnGetRecurringTips(
    GetRecurringTipsCallback callback,
    ledger::type::PublisherInfoList list) {
  std::move(callback).Run(std::move(list));
}

void RewardsServiceImpl::GetRecurringTips(
    GetRecurringTipsCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetRecurringTips(
      base::BindOnce(&RewardsServiceImpl::OnGetRecurringTips,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnGetOneTimeTips(
    GetRecurringTipsCallback callback,
    ledger::type::PublisherInfoList list) {
  std::move(callback).Run(std::move(list));
}

void RewardsServiceImpl::GetOneTimeTips(GetOneTimeTipsCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetOneTimeTips(
      base::BindOnce(&RewardsServiceImpl::OnGetOneTimeTips,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnRecurringTip(const ledger::type::Result result) {
  bool success = result == ledger::type::Result::LEDGER_OK;
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
    base::Bind(&RewardsServiceImpl::OnRecurringTip,
               AsWeakPtr()));
}

void RewardsServiceImpl::OnSetPublisherExclude(
    const std::string& publisher_key,
    const bool exclude,
    const ledger::type::Result result) {
  if (result != ledger::type::Result::LEDGER_OK) {
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

  ledger::type::PublisherExclude status =
      exclude
      ? ledger::type::PublisherExclude::EXCLUDED
      : ledger::type::PublisherExclude::INCLUDED;

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

void RewardsServiceImpl::StartNotificationTimers(bool main_enabled) {
  if (!main_enabled) return;

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

  bat_ledger_->GetCreationStamp(
      base::BindOnce(&RewardsServiceImpl::MaybeShowBackupNotification,
        AsWeakPtr()));
  GetReconcileStamp(
      base::Bind(&RewardsServiceImpl::MaybeShowAddFundsNotification,
        AsWeakPtr()));
  FetchPromotions();
}

void RewardsServiceImpl::MaybeShowNotificationAddFunds() {
  if (!Connected()) {
    return;
  }

  bat_ledger_->HasSufficientBalanceToReconcile(
      base::BindOnce(&RewardsServiceImpl::ShowNotificationAddFunds,
        AsWeakPtr()));
}

void RewardsServiceImpl::MaybeShowNotificationAddFundsForTesting(
    base::OnceCallback<void(bool)> callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->HasSufficientBalanceToReconcile(
      base::BindOnce(
          &RewardsServiceImpl::OnMaybeShowNotificationAddFundsForTesting,
          AsWeakPtr(),
          std::move(callback)));
}

void RewardsServiceImpl::OnMaybeShowNotificationAddFundsForTesting(
    base::OnceCallback<void(bool)> callback,
    const bool sufficient) {
  ShowNotificationAddFunds(sufficient);
  std::move(callback).Run(sufficient);
}

bool RewardsServiceImpl::ShouldShowNotificationAddFunds() const {
  base::Time next_time =
      profile_->GetPrefs()->GetTime(prefs::kAddFundsNotification);
  return (next_time.is_null() || base::Time::Now() > next_time);
}

void RewardsServiceImpl::ShowNotificationAddFunds(bool sufficient) {
  if (sufficient) return;

  base::Time next_time = base::Time::Now() + base::TimeDelta::FromDays(3);
  profile_->GetPrefs()->SetTime(prefs::kAddFundsNotification, next_time);
  RewardsNotificationService::RewardsNotificationArgs args;
  notification_service_->AddNotification(
      RewardsNotificationService::REWARDS_NOTIFICATION_INSUFFICIENT_FUNDS, args,
      "rewards_notification_insufficient_funds");
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

bool RewardsServiceImpl::MaybeTailDiagnosticLog(
    const int num_lines) {
  if (!diagnostic_log_.IsValid()) {
    return false;
  }

  static bool first_run = true;

  const int64_t length = diagnostic_log_.GetLength();
  if (length == -1) {
    return false;
  }

  if (!first_run && length <= kDiagnosticLogMaxFileSize) {
    return true;
  }

  first_run = false;

  if (!TailFile(&diagnostic_log_, num_lines)) {
    return false;
  }

  return true;
}

void RewardsServiceImpl::DiagnosticLog(
    const std::string& file,
    const int line,
    const int verbose_level,
    const std::string& message) {
  if (ledger_for_testing_ || !should_persist_logs_) {
    return;
  }

  if (resetting_rewards_) {
    return;
  }

  if (verbose_level > kDiagnosticLogMaxVerboseLevel) {
    return;
  }

  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&RewardsServiceImpl::WriteToDiagnosticLogOnFileTaskRunner,
          base::Unretained(this),
          diagnostic_log_path_,
          kTailDiagnosticLogToNumLines,
          file,
          line,
          verbose_level,
          message),
      base::BindOnce(&RewardsServiceImpl::OnWriteToLogOnFileTaskRunner,
          AsWeakPtr()));
}

bool RewardsServiceImpl::WriteToDiagnosticLogOnFileTaskRunner(
    const base::FilePath& log_path,
    const int num_lines,
    const std::string& file,
    const int line,
    const int verbose_level,
    const std::string& message) {
  if (!InitializeLog(&diagnostic_log_, log_path)) {
    VLOG(0) << "Failed to initialize diagnostic log: "
        << GetLastFileError(&diagnostic_log_);

    return false;
  }

  const base::Time time = base::Time::Now();

  const std::string log_entry =
      FriendlyFormatLogEntry(time, file, line, verbose_level, message);

  if (!WriteToLog(&diagnostic_log_, log_entry)) {
    VLOG(0) << "Failed to write to diagnostic log: "
        << GetLastFileError(&diagnostic_log_);

    return false;
  }

  if (!MaybeTailDiagnosticLog(num_lines)) {
    VLOG(0) << "Failed to vacuum diagnostic log";

    return false;
  }

  return true;
}

void RewardsServiceImpl::OnWriteToLogOnFileTaskRunner(
    const bool success) {
  DCHECK(success);
}

void RewardsServiceImpl::LoadDiagnosticLog(
      const int num_lines,
      LoadDiagnosticLogCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&RewardsServiceImpl::LoadDiagnosticLogOnFileTaskRunner,
          base::Unretained(this),
          diagnostic_log_path_,
          num_lines),
      base::BindOnce(&RewardsServiceImpl::OnLoadDiagnosticLogOnFileTaskRunner,
          AsWeakPtr(),
          std::move(callback)));
}

std::string RewardsServiceImpl::LoadDiagnosticLogOnFileTaskRunner(
    const base::FilePath& path,
    const int num_lines) {
  if (!base::PathExists(path)) {
    return "";
  }

  std::string value;
  if (!TailFileAsString(&diagnostic_log_, num_lines, &value)) {
    return base::StringPrintf("ERROR: %s",
        GetLastFileError(&diagnostic_log_).c_str());
  }

  return value;
}

void RewardsServiceImpl::OnLoadDiagnosticLogOnFileTaskRunner(
    LoadDiagnosticLogCallback callback,
    const std::string& value) {
  std::move(callback).Run(value);
}

void RewardsServiceImpl::ClearDiagnosticLog(
    ClearDiagnosticLogCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&RewardsServiceImpl::ClearDiagnosticLogOnFileTaskRunner,
          base::Unretained(this),
          diagnostic_log_path_),
      base::BindOnce(&RewardsServiceImpl::OnClearDiagnosticLogOnFileTaskRunner,
          AsWeakPtr(),
          std::move(callback)));
}

bool RewardsServiceImpl::ClearDiagnosticLogOnFileTaskRunner(
    const base::FilePath& path) {
  if (!base::PathExists(path)) {
    return true;
  }

  diagnostic_log_.Close();

  return base::DeleteFile(path, false);
}

void RewardsServiceImpl::OnClearDiagnosticLogOnFileTaskRunner(
    ClearDiagnosticLogCallback callback,
    const bool success) {
  std::move(callback).Run(success);
}

void RewardsServiceImpl::Log(
    const char* file,
    const int line,
    const int verbose_level,
    const std::string& message) {
  DCHECK(file);

  DiagnosticLog(file, line, verbose_level, message);

  const int vlog_level = ::logging::GetVlogLevelHelper(file, strlen(file));
  if (verbose_level <= vlog_level) {
    ::logging::LogMessage(file, line, -verbose_level).stream() << message;
  }
}

// static
void RewardsServiceImpl::HandleFlags(const std::string& options) {
  std::vector<std::string> flags = base::SplitString(
      options, ",", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  for (const auto& flag : flags) {
    if (flag.empty()) {
      continue;
    }

    std::vector<std::string> values = base::SplitString(
      flag, "=", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

    if (values.size() != 2) {
      continue;
    }

    std::string name = base::ToLowerASCII(values[0]);
    std::string value = values[1];

    if (value.empty()) {
      continue;
    }

    if (name == "staging") {
      ledger::type::Environment environment;
      std::string lower = base::ToLowerASCII(value);

      if (lower == "true" || lower == "1") {
        environment = ledger::type::Environment::STAGING;
      } else {
        environment = ledger::type::Environment::PRODUCTION;
      }

      SetEnvironment(environment);
      continue;
    }

    if (name == "debug") {
      bool is_debug;
      std::string lower = base::ToLowerASCII(value);

      if (lower == "true" || lower == "1") {
        is_debug = true;
      } else {
        is_debug = false;
      }

      SetDebug(is_debug);
      continue;
    }

    if (name == "reconcile-interval") {
      int reconcile_int;
      bool success = base::StringToInt(value, &reconcile_int);

      if (success && reconcile_int > 0) {
        SetReconcileInterval(reconcile_int);
      }

      continue;
    }

    if (name == "short-retries") {
      std::string lower = base::ToLowerASCII(value);
      bool short_retries;

      if (lower == "true" || lower == "1") {
        short_retries = true;
      } else {
        short_retries = false;
      }

      SetShortRetries(short_retries);
    }

    if (name == "uphold-token") {
      std::string token = base::ToLowerASCII(value);

      auto uphold = ledger::type::ExternalWallet::New();
      uphold->token = token;
      uphold->address = "c5fd7219-6586-4fe1-b947-0cbd25040ca8";
      uphold->status = ledger::type::WalletStatus::VERIFIED;
      uphold->one_time_string = "";
      uphold->user_name = "Brave Test";
      SaveExternalWallet(ledger::constant::kWalletUphold, std::move(uphold));
      continue;
    }

    if (name == "development") {
      ledger::type::Environment environment;
      std::string lower = base::ToLowerASCII(value);

      if (lower == "true" || lower == "1") {
        environment = ledger::type::Environment::DEVELOPMENT;
        SetEnvironment(environment);
      }

      continue;
    }

    if (name == "persist-logs") {
      const std::string lower = base::ToLowerASCII(value);

      if (lower == "true" || lower == "1") {
        should_persist_logs_ = true;
      } else {
        should_persist_logs_ = false;
      }
    }
  }
}

void RewardsServiceImpl::SetBackupCompleted() {
  profile_->GetPrefs()->SetBoolean(prefs::kBackupSucceeded, true);
}

void RewardsServiceImpl::GetRewardsInternalsInfo(
    GetRewardsInternalsInfoCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetRewardsInternalsInfo(
      base::BindOnce(&RewardsServiceImpl::OnGetRewardsInternalsInfo,
                     AsWeakPtr(), std::move(callback)));
}

void RewardsServiceImpl::OnTip(
    const std::string& publisher_key,
    const double amount,
    const bool recurring,
    ledger::type::PublisherInfoPtr publisher) {
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
    const ledger::type::Result result) {
  if (result != ledger::type::Result::LEDGER_OK) {
    return;
  }

  OnTip(publisher_key, amount, recurring);
}

void RewardsServiceImpl::OnTip(
    const std::string& publisher_key,
    const double amount,
    const bool recurring) {
  if (!Connected()) {
    return;
  }

  if (recurring) {
    SaveRecurringTip(publisher_key, amount, base::DoNothing());
    return;
  }

  bat_ledger_->OneTimeTip(publisher_key, amount, base::DoNothing());
}

bool RewardsServiceImpl::Connected() const {
  return bat_ledger_.is_bound();
}

void RewardsServiceImpl::SetLedgerEnvForTesting() {
  ledger_for_testing_ = true;
}

void RewardsServiceImpl::PrepareLedgerEnvForTesting() {
  if (!ledger_for_testing_) {
    return;
  }

  bat_ledger_service_->SetTesting();

  profile_->GetPrefs()->SetInteger(prefs::kMinVisitTime, 1);
  SetShortRetries(true);

  // this is needed because we are using braveledger_request_util::buildURL
  // directly in RewardsBrowserTest
  #if defined(OFFICIAL_BUILD)
  ledger::_environment = ledger::type::Environment::PRODUCTION;
  #else
  ledger::_environment = ledger::type::Environment::STAGING;
  #endif
}

void RewardsServiceImpl::StartMonthlyContributionForTest() {
  if (!Connected()) {
    return;
  }

  bat_ledger_->StartMonthlyContribution();
}

void RewardsServiceImpl::CheckInsufficientFundsForTesting() {
  MaybeShowNotificationAddFunds();
}

ledger::type::TransferFeeList RewardsServiceImpl::GetTransferFeesForTesting(
    const std::string& wallet_type) {
  return GetTransferFees(wallet_type);
}

void RewardsServiceImpl::GetEnvironment(
    const GetEnvironmentCallback& callback) {
  bat_ledger_service_->GetEnvironment(callback);
}

void RewardsServiceImpl::GetDebug(const GetDebugCallback& callback) {
  bat_ledger_service_->GetDebug(callback);
}

void RewardsServiceImpl::GetReconcileInterval(
    GetReconcileIntervalCallback callback) {
  bat_ledger_service_->GetReconcileInterval(std::move(callback));
}

void RewardsServiceImpl::GetShortRetries(
    const GetShortRetriesCallback& callback) {
  bat_ledger_service_->GetShortRetries(callback);
}

void RewardsServiceImpl::SetEnvironment(ledger::type::Environment environment) {
  bat_ledger_service_->SetEnvironment(environment);
}

void RewardsServiceImpl::SetDebug(bool debug) {
  bat_ledger_service_->SetDebug(debug);
}

void RewardsServiceImpl::SetReconcileInterval(const int32_t interval) {
  bat_ledger_service_->SetReconcileInterval(interval);
}

void RewardsServiceImpl::SetShortRetries(bool short_retries) {
  bat_ledger_service_->SetShortRetries(short_retries);
}

void RewardsServiceImpl::GetPendingContributionsTotal(
    const GetPendingContributionsTotalCallback& callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetPendingContributionsTotal(std::move(callback));
}

void RewardsServiceImpl::PublisherListNormalized(
    ledger::type::PublisherInfoList list) {
  for (auto& observer : observers_) {
    ledger::type::PublisherInfoList new_list;
    for (const auto& publisher : list) {
      if (publisher->percent >= 1) {
        new_list.push_back(publisher->Clone());
      }
    }
    observer.OnPublisherListNormalized(this, std::move(new_list));
  }
}

void RewardsServiceImpl::RefreshPublisher(
    const std::string& publisher_key,
    RefreshPublisherCallback callback) {
  if (!Connected()) {
    std::move(callback).Run(ledger::type::PublisherStatus::NOT_VERIFIED, "");
    return;
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
    ledger::type::PublisherStatus status) {
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
    return;
  }

  const auto platform = ConvertInlineTipStringToPlatform(key);
  bat_ledger_->GetInlineTippingPlatformEnabled(
      platform,
      base::BindOnce(&RewardsServiceImpl::OnInlineTipSetting,
          AsWeakPtr(),
          std::move(callback)));
}

void RewardsServiceImpl::OnInlineTipSetting(
    GetInlineTippingPlatformEnabledCallback callback,
    bool enabled) {
  std::move(callback).Run(enabled);
}

void RewardsServiceImpl::GetShareURL(
      const std::string& type,
      const std::map<std::string, std::string>& args,
      GetShareURLCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetShareURL(
      type,
      base::MapToFlatMap(args),
      base::BindOnce(&RewardsServiceImpl::OnShareURL,
          AsWeakPtr(),
          std::move(callback)));
}

void RewardsServiceImpl::OnShareURL(
    GetShareURLCallback callback,
    const std::string& url) {
  std::move(callback).Run(url);
}

void RewardsServiceImpl::OnGetPendingContributions(
    GetPendingContributionsCallback callback,
    ledger::type::PendingContributionInfoList list) {
  std::move(callback).Run(std::move(list));
}

void RewardsServiceImpl::GetPendingContributions(
    GetPendingContributionsCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetPendingContributions(
      base::BindOnce(&RewardsServiceImpl::OnGetPendingContributions,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnPendingContributionRemoved(
  const ledger::type::Result result) {
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
  const ledger::type::Result result) {
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
      ledger::type::Result result,
      const std::string& publisher_key,
      const std::string& publisher_name) {
  switch (result) {
    case ledger::type::Result::PENDING_NOT_ENOUGH_FUNDS:
    {
      RewardsNotificationService::RewardsNotificationArgs args;
      notification_service_->AddNotification(
          RewardsNotificationService::
          REWARDS_NOTIFICATION_PENDING_NOT_ENOUGH_FUNDS,
          args,
          "rewards_notification_not_enough_funds");
      break;
    }
    case ledger::type::Result::PENDING_PUBLISHER_REMOVED:
    {
      for (auto& observer : observers_) {
        observer.OnPendingContributionRemoved(
            this,
            ledger::type::Result::LEDGER_OK);
      }
      break;
    }
    case ledger::type::Result::VERIFIED_PUBLISHER:
    {
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

void RewardsServiceImpl::OnFetchBalance(
    FetchBalanceCallback callback,
    const ledger::type::Result result,
    ledger::type::BalancePtr balance) {
  if (balance) {
    if (balance->total > 0) {
      profile_->GetPrefs()->SetBoolean(prefs::kUserHasFunded, true);
    }

    // Record stats.
    double balance_minus_grant = CalcWalletBalanceForP3A(
        balance->wallets,
        balance->user_funds);
    RecordWalletBalanceP3A(
        true,
        true,
        static_cast<size_t>(balance_minus_grant));
  }

  std::move(callback).Run(result, std::move(balance));
}

void RewardsServiceImpl::FetchBalance(FetchBalanceCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->FetchBalance(
      base::BindOnce(&RewardsServiceImpl::OnFetchBalance,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::SaveExternalWallet(
    const std::string& wallet_type,
    ledger::type::ExternalWalletPtr wallet) {
  auto* wallets =
      profile_->GetPrefs()->GetDictionary(prefs::kExternalWallets);

  base::Value new_wallets(wallets->Clone());

  auto* old_wallet = new_wallets.FindDictKey(wallet_type);
  base::Value new_wallet(base::Value::Type::DICTIONARY);
  if (old_wallet) {
    new_wallet = old_wallet->Clone();
  }

  new_wallet.SetStringKey("token", wallet->token);
  new_wallet.SetStringKey("address", wallet->address);
  new_wallet.SetIntKey("status", static_cast<int>(wallet->status));
  new_wallet.SetStringKey("one_time_string", wallet->one_time_string);
  new_wallet.SetStringKey("user_name", wallet->user_name);
  new_wallet.SetStringKey("verify_url", wallet->verify_url);
  new_wallet.SetStringKey("add_url", wallet->add_url);
  new_wallet.SetStringKey("withdraw_url", wallet->withdraw_url);
  new_wallet.SetStringKey("account_url", wallet->account_url);
  new_wallet.SetStringKey("login_url", wallet->login_url);

  new_wallets.SetKey(wallet_type, std::move(new_wallet));

  profile_->GetPrefs()->Set(prefs::kExternalWallets, new_wallets);
}

std::map<std::string, ledger::type::ExternalWalletPtr>
RewardsServiceImpl::GetExternalWallets() {
  std::map<std::string, ledger::type::ExternalWalletPtr> wallets;

  auto* dict =
      profile_->GetPrefs()->GetDictionary(prefs::kExternalWallets);

  for (const auto& it : dict->DictItems()) {
    ledger::type::ExternalWalletPtr wallet =
        ledger::type::ExternalWallet::New();

    wallet->type = it.first;

    auto* token = it.second.FindStringKey("token");
    if (token) {
      wallet->token = *token;
    }

    auto* address = it.second.FindStringKey("address");
    if (address) {
      wallet->address = *address;
    }

    auto* one_time_string = it.second.FindStringKey("one_time_string");
    if (one_time_string) {
      wallet->one_time_string = *one_time_string;
    }

    auto status = it.second.FindIntKey("status");
    if (status) {
      wallet->status = static_cast<ledger::type::WalletStatus>(*status);
    }

    auto* user_name = it.second.FindStringKey("user_name");
    if (user_name) {
      wallet->user_name = *user_name;
    }

    auto* verify_url = it.second.FindStringKey("verify_url");
    if (verify_url) {
      wallet->verify_url = *verify_url;
    }

    auto* add_url = it.second.FindStringKey("add_url");
    if (add_url) {
      wallet->add_url = *add_url;
    }

    auto* withdraw_url = it.second.FindStringKey("withdraw_url");
    if (withdraw_url) {
      wallet->withdraw_url = *withdraw_url;
    }

    auto* account_url = it.second.FindStringKey("account_url");
    if (account_url) {
      wallet->account_url = *account_url;
    }

    auto* login_url = it.second.FindStringKey("login_url");
    if (login_url) {
      wallet->login_url = *login_url;
    }

    wallets.insert(std::make_pair(it.first, std::move(wallet)));
  }

  return wallets;
}

void RewardsServiceImpl::OnGetExternalWallet(
    const std::string& wallet_type,
    GetExternalWalletCallback callback,
    const ledger::type::Result result,
    ledger::type::ExternalWalletPtr wallet) {
  std::move(callback).Run(result, std::move(wallet));
}

void RewardsServiceImpl::GetExternalWallet(
    const std::string& wallet_type,
    GetExternalWalletCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetExternalWallet(wallet_type,
      base::BindOnce(&RewardsServiceImpl::OnGetExternalWallet,
                     AsWeakPtr(),
                     wallet_type,
                     std::move(callback)));
}

void RewardsServiceImpl::OnExternalWalletAuthorization(
    const std::string& wallet_type,
    ExternalWalletAuthorizationCallback callback,
    const ledger::type::Result result,
    const base::flat_map<std::string, std::string>& args) {
  std::move(callback).Run(result, base::FlatMapToMap(args));
}

void RewardsServiceImpl::ExternalWalletAuthorization(
      const std::string& wallet_type,
      const std::map<std::string, std::string>& args,
      ExternalWalletAuthorizationCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->ExternalWalletAuthorization(
      wallet_type,
      base::MapToFlatMap(args),
      base::BindOnce(&RewardsServiceImpl::OnExternalWalletAuthorization,
                     AsWeakPtr(),
                     wallet_type,
                     std::move(callback)));
}

void RewardsServiceImpl::OnProcessExternalWalletAuthorization(
    const std::string& wallet_type,
    const std::string& action,
    ProcessRewardsPageUrlCallback callback,
    const ledger::type::Result result,
    const std::map<std::string, std::string>& args) {
  std::move(callback).Run(result, wallet_type, action, args);
}

void RewardsServiceImpl::ProcessRewardsPageUrl(
    const std::string& path,
    const std::string& query,
    ProcessRewardsPageUrlCallback callback) {
  auto path_items = base::SplitString(
      path,
      "/",
      base::TRIM_WHITESPACE,
      base::SPLIT_WANT_NONEMPTY);

  if (path_items.size() < 2) {
    std::move(callback).Run(ledger::type::Result::LEDGER_ERROR, "", "", {});
    return;
  }

  const std::string action = path_items.at(1);
  const std::string wallet_type = path_items.at(0);

  std::map<std::string, std::string> query_map;

  const auto url = GURL("brave:/" + path + query);
  for (net::QueryIterator it(url); !it.IsAtEnd(); it.Advance()) {
    query_map[it.GetKey()] = it.GetUnescapedValue();
  }

  if (action == "authorization") {
    if (wallet_type == ledger::constant::kWalletUphold) {
      ExternalWalletAuthorization(
          wallet_type,
          query_map,
          base::BindOnce(
              &RewardsServiceImpl::OnProcessExternalWalletAuthorization,
              AsWeakPtr(),
              wallet_type,
              action,
              std::move(callback)));
      return;
    }
  }

  std::move(callback).Run(
      ledger::type::Result::LEDGER_ERROR,
      wallet_type,
      action,
      {});
}

void RewardsServiceImpl::OnDisconnectWallet(
    const std::string& wallet_type,
    const ledger::type::Result result) {
  for (auto& observer : observers_) {
    observer.OnDisconnectWallet(this, result, wallet_type);
  }
}

void RewardsServiceImpl::DisconnectWallet(const std::string& wallet_type) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->DisconnectWallet(
      wallet_type,
      base::BindOnce(&RewardsServiceImpl::OnDisconnectWallet,
                     AsWeakPtr(),
                     wallet_type));
}

void RewardsServiceImpl::ShowNotification(
      const std::string& type,
      const std::vector<std::string>& args,
      ledger::ResultCallback callback) {
  if (type.empty()) {
    callback(ledger::type::Result::LEDGER_ERROR);
    return;
  }

  RewardsNotificationService::RewardsNotificationArgs notification_args;
  notification_args.push_back(type);
  notification_args.insert(notification_args.end(), args.begin(), args.end());
  notification_service_->AddNotification(
      RewardsNotificationService::REWARDS_NOTIFICATION_GENERAL_LEDGER,
      notification_args,
      "rewards_notification_general_ledger_" + type);
    callback(ledger::type::Result::LEDGER_OK);
}

void RewardsServiceImpl::SetTransferFee(
    const std::string& wallet_type,
    ledger::type::TransferFeePtr transfer_fee) {
  if (!transfer_fee) {
    return;
  }

  base::Value fee(base::Value::Type::DICTIONARY);
  fee.SetStringKey("id", transfer_fee->id);
  fee.SetDoubleKey("amount", transfer_fee->amount);

  auto* external_wallets =
      profile_->GetPrefs()->GetDictionary(prefs::kExternalWallets);

  base::Value new_external_wallets(external_wallets->Clone());

  auto* wallet = new_external_wallets.FindDictKey(wallet_type);
  base::Value new_wallet(base::Value::Type::DICTIONARY);
  if (wallet) {
    new_wallet = wallet->Clone();
  }

  auto* fees = wallet->FindDictKey("transfer_fees");
  base::Value new_fees(base::Value::Type::DICTIONARY);
  if (fees) {
    new_fees = fees->Clone();
  }
  new_fees.SetKey(transfer_fee->id, std::move(fee));

  new_wallet.SetKey("transfer_fees", std::move(new_fees));
  new_external_wallets.SetKey(wallet_type, std::move(new_wallet));

  profile_->GetPrefs()->Set(
      prefs::kExternalWallets,
      new_external_wallets);
}

ledger::type::TransferFeeList RewardsServiceImpl::GetTransferFees(
    const std::string& wallet_type) {
  ledger::type::TransferFeeList fees;

  auto* external_wallets =
      profile_->GetPrefs()->GetDictionary(prefs::kExternalWallets);

  auto* wallet_key = external_wallets->FindKey(wallet_type);

  const base::DictionaryValue* wallet;
  if (!wallet_key || !wallet_key->GetAsDictionary(&wallet)) {
    return fees;
  }

  auto* fees_key = wallet->FindKey("transfer_fees");
  const base::DictionaryValue* fee_dict;
  if (!fees_key || !fees_key->GetAsDictionary(&fee_dict)) {
    return fees;
  }

  for (auto& item : *fee_dict) {
    const base::DictionaryValue* fee_dict;
    if (!item.second || !item.second->GetAsDictionary(&fee_dict)) {
      continue;
    }

    auto fee = ledger::type::TransferFee::New();

    auto* id = fee_dict->FindKey("id");
    if (!id || !id->is_string()) {
      continue;
    }
    fee->id = id->GetString();

    auto* amount = fee_dict->FindKey("amount");
    if (!amount || !amount->is_double()) {
      continue;
    }
    fee->amount = amount->GetDouble();

    fees.insert(std::make_pair(fee->id, std::move(fee)));
  }

  return fees;
}

void RewardsServiceImpl::RemoveTransferFee(
    const std::string& wallet_type,
    const std::string& id) {
  auto* external_wallets =
      profile_->GetPrefs()->GetDictionary(prefs::kExternalWallets);

  base::Value new_external_wallets(external_wallets->Clone());

  const auto path = base::StringPrintf(
      "%s.transfer_fees.%s",
      wallet_type.c_str(),
      id.c_str());

  bool success = new_external_wallets.RemovePath(path);
  if (!success) {
    return;
  }

  profile_->GetPrefs()->Set(
      prefs::kExternalWallets,
      new_external_wallets);
}

bool RewardsServiceImpl::OnlyAnonWallet() {
  const int32_t current_country =
      country_codes::GetCountryIDFromPrefs(profile_->GetPrefs());

  for (const auto& country : kOnlyAnonWalletCountries) {
    if (country.length() != 2) {
      continue;
    }

    const int id = country_codes::CountryCharsToCountryID(
        country.at(0), country.at(1));

    if (id == current_country) {
      return true;
    }
  }

  return false;
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
    ledger::type::PublisherInfoList list) {
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
    ledger::type::ContributionInfoList list) {
  int auto_contributions = 0;
  int tips = 0;
  int queued_recurring = 0;

  for (auto& contribution : list) {
    switch (contribution->type) {
    case ledger::type::RewardsType::AUTO_CONTRIBUTE: {
      auto_contributions += 1;
      break;
    }
    case ledger::type::RewardsType::ONE_TIME_TIP: {
      tips += 1;
      break;
    }
    case ledger::type::RewardsType::RECURRING_TIP: {
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

  RecordTipsState(true, true, tips, queued_recurring);

  GetAutoContributeEnabled(base::BindOnce(
    &RewardsServiceImpl::OnRecordBackendP3AStatsAC,
    AsWeakPtr(),
    auto_contributions));
}

void RewardsServiceImpl::OnRecordBackendP3AStatsAC(
    const int auto_contributions,
    bool ac_enabled) {
  const auto auto_contributions_state = ac_enabled ?
        AutoContributionsP3AState::kAutoContributeOn :
        AutoContributionsP3AState::kWalletCreatedAutoContributeOff;
  RecordAutoContributionsState(auto_contributions_state, auto_contributions);
}

#if defined(OS_ANDROID)
ledger::type::Environment RewardsServiceImpl::GetServerEnvironmentForAndroid() {
  auto result = ledger::type::Environment::PRODUCTION;
  bool use_staging = false;
  if (profile_ && profile_->GetPrefs()) {
    use_staging =
        profile_->GetPrefs()->GetBoolean(prefs::kUseRewardsStagingServer);
  }

  if (use_staging) {
    result = ledger::type::Environment::STAGING;
  }

  return result;
}
#endif

ledger::type::ClientInfoPtr GetDesktopClientInfo() {
  auto info = ledger::type::ClientInfo::New();
  info->platform = ledger::type::Platform::DESKTOP;
  #if defined(OS_MACOSX)
    info->os = ledger::type::OperatingSystem::MACOS;
  #elif defined(OS_WIN)
    info->os = ledger::type::OperatingSystem::WINDOWS;
  #elif defined(OS_LINUX)
    info->os = ledger::type::OperatingSystem::LINUX;
  #else
    info->os = ledger::type::OperatingSystem::UNDEFINED;
  #endif

  return info;
}

ledger::type::ClientInfoPtr RewardsServiceImpl::GetClientInfo() {
  #if defined(OS_ANDROID)
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

void RewardsServiceImpl::GetAnonWalletStatus(
    GetAnonWalletStatusCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetAnonWalletStatus(
      base::BindOnce(&RewardsServiceImpl::OnGetAnonWalletStatus,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnGetAnonWalletStatus(
    GetAnonWalletStatusCallback callback,
    const ledger::type::Result result) {
  std::move(callback).Run(result);
}

void RewardsServiceImpl::GetMonthlyReport(
    const uint32_t month,
    const uint32_t year,
    GetMonthlyReportCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetMonthlyReport(
      static_cast<ledger::type::ActivityMonth>(month),
      year,
      base::BindOnce(&RewardsServiceImpl::OnGetMonthlyReport,
          AsWeakPtr(),
          std::move(callback)));
}

void RewardsServiceImpl::OnGetMonthlyReport(
    GetMonthlyReportCallback callback,
    const ledger::type::Result result,
    ledger::type::MonthlyReportInfoPtr report) {
  std::move(callback).Run(std::move(report));
}

void RewardsServiceImpl::ReconcileStampReset() {
  for (auto& observer : observers_) {
    observer.ReconcileStampReset();
  }
}

ledger::type::DBCommandResponsePtr RunDBTransactionOnFileTaskRunner(
    ledger::type::DBTransactionPtr transaction,
    ledger::LedgerDatabase* database) {
  auto response = ledger::type::DBCommandResponse::New();
  if (!database) {
    response->status = ledger::type::DBCommandResponse::Status::RESPONSE_ERROR;
  } else {
    database->RunTransaction(std::move(transaction), response.get());
  }

  return response;
}

void RewardsServiceImpl::RunDBTransaction(
    ledger::type::DBTransactionPtr transaction,
    ledger::client::RunDBTransactionCallback callback) {
  DCHECK(ledger_database_);
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(),
      FROM_HERE,
      base::BindOnce(&RunDBTransactionOnFileTaskRunner,
          base::Passed(std::move(transaction)),
          ledger_database_.get()),
      base::BindOnce(&RewardsServiceImpl::OnRunDBTransaction,
          AsWeakPtr(),
          std::move(callback)));
}

void RewardsServiceImpl::OnRunDBTransaction(
    ledger::client::RunDBTransactionCallback callback,
    ledger::type::DBCommandResponsePtr response) {
  callback(std::move(response));
}

void RewardsServiceImpl::GetCreateScript(
    ledger::client::GetCreateScriptCallback callback) {
  callback("", 0);
}

void RewardsServiceImpl::PendingContributionSaved(
    const ledger::type::Result result) {
  for (auto& observer : observers_) {
    observer.OnPendingContributionSaved(this, result);
  }
}

bool RewardsServiceImpl::IsWalletInitialized() {
  return is_wallet_initialized_;
}

void RewardsServiceImpl::ForTestingSetTestResponseCallback(
    GetTestResponseCallback callback) {
  test_response_callback_ = callback;
}

void RewardsServiceImpl::GetAllMonthlyReportIds(
      GetAllMonthlyReportIdsCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetAllMonthlyReportIds(
      base::BindOnce(&RewardsServiceImpl::OnGetAllMonthlyReportIds,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnGetAllMonthlyReportIds(
    GetAllMonthlyReportIdsCallback callback,
    const std::vector<std::string>& ids) {
  std::move(callback).Run(ids);
}

void RewardsServiceImpl::GetAllContributions(
    GetAllContributionsCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetAllContributions(
      base::BindOnce(&RewardsServiceImpl::OnGetAllContributions,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnGetAllContributions(
    GetAllContributionsCallback callback,
    ledger::type::ContributionInfoList contributions) {
  std::move(callback).Run(std::move(contributions));
}

void RewardsServiceImpl::GetAllPromotions(GetAllPromotionsCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetAllPromotions(
      base::BindOnce(&RewardsServiceImpl::OnGetAllPromotions,
                     AsWeakPtr(),
                     std::move(callback)));
}

void RewardsServiceImpl::OnGetAllPromotions(
    GetAllPromotionsCallback callback,
    base::flat_map<std::string, ledger::type::PromotionPtr> promotions) {
  ledger::type::PromotionList list;
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

  auto* ads_service = brave_ads::AdsServiceFactory::GetForProfile(profile_);
  if (ads_service) {
    ads_service->ResetAllState(/* should_shutdown */ true);
  }

  StopNotificationTimers();
  notification_service_->DeleteAllNotifications(true);

  auto stop_callback =
      base::BindOnce(
          &RewardsServiceImpl::OnStopLedgerForCompleteReset,
          AsWeakPtr(),
          std::move(callback));
  StopLedger(std::move(stop_callback));
}

void RewardsServiceImpl::OnCompleteReset(
    SuccessCallback callback,
    const bool success) {
  resetting_rewards_ = false;

  for (auto& observer : observers_) {
    observer.OnCompleteReset(success);
  }

  std::move(callback).Run(true);
}

void RewardsServiceImpl::WalletDisconnected(const std::string& wallet_type) {
  OnDisconnectWallet(wallet_type, ledger::type::Result::LEDGER_OK);
}

void RewardsServiceImpl::DeleteLog(ledger::ResultCallback callback) {
  diagnostic_log_.Close();
  base::PostTaskAndReplyWithResult(
      file_task_runner_.get(),
      FROM_HERE,
      base::BindOnce(
          &RewardsServiceImpl::DeleteLogTaskRunner,
          base::Unretained(this)),
      base::BindOnce(
          &RewardsServiceImpl::OnDeleteLog,
          AsWeakPtr(),
          std::move(callback)));
}

bool RewardsServiceImpl::DeleteLogTaskRunner() {
  return base::DeleteFile(diagnostic_log_path_);
}

void RewardsServiceImpl::OnDeleteLog(
    ledger::ResultCallback callback,
    const bool success) {
  const auto result = success
      ? ledger::type::Result::LEDGER_OK
      : ledger::type::Result::LEDGER_ERROR;
  callback(result);
}

void RewardsServiceImpl::GetEventLogs(GetEventLogsCallback callback) {
  if (!Connected()) {
    return;
  }

  bat_ledger_->GetEventLogs(
      base::BindOnce(&RewardsServiceImpl::OnGetEventLogs,
          AsWeakPtr(),
          std::move(callback)));
}

void RewardsServiceImpl::OnGetEventLogs(
    GetEventLogsCallback callback,
    ledger::type::EventLogs logs) {
  std::move(callback).Run(std::move(logs));
}

}  // namespace brave_rewards
