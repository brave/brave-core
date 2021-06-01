/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_referrals/browser/brave_referrals_service.h"

#include <memory>
#include <utility>

#include "base/callback_helpers.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/system/sys_info.h"
#include "base/task/post_task.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/common/network_constants.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_referrals/common/pref_names.h"
#include "brave_base/random.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/scoped_tabbed_browser_displayer.h"
#include "chrome/common/chrome_paths.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/page_navigator.h"
#include "content/public/common/referrer.h"
#include "extensions/common/url_pattern.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"

#if defined(OS_ANDROID)
#include "chrome/browser/android/service_tab_launcher.h"
#else
#include "chrome/browser/ui/browser.h"
#endif

// Fetch headers from the referral server once a day.
const int kFetchReferralHeadersFrequency = 60 * 60 * 24;

// Perform finalization checks once a day.
const int kFinalizationChecksFrequency = 60 * 60 * 24;

// Report initialization once a day (after initial failure).
const int kReportInitializationFrequency = 60 * 60 * 24;

// Maximum size of the referral server response in bytes.
const int kMaxReferralServerResponseSizeBytes = 1024 * 1024;

// Default promo code, used when no promoCode file exists on first
// run.
const char kDefaultPromoCode[] = "BRV001";

namespace brave {

namespace {

BraveReferralsService::ReferralInitializedCallback*
    g_testing_referral_initialized_callback = nullptr;

base::FilePath g_promo_file_path;

void DeletePromoCodeFile(const base::FilePath& promo_code_file) {
  if (!base::DeleteFile(promo_code_file)) {
    LOG(ERROR) << "Failed to delete referral promo code file "
               << promo_code_file.value();
  }
}

std::string ReadPromoCode(const base::FilePath& promo_code_file) {
  std::string promo_code;

  if (!base::PathExists(promo_code_file)) {
    return kDefaultPromoCode;
  }

  if (!base::ReadFileToString(promo_code_file, &promo_code)) {
    LOG(ERROR) << "Failed to read referral promo code from "
               << promo_code_file.value();
    return "";
  }

  base::TrimWhitespaceASCII(promo_code, base::TRIM_ALL, &promo_code);
  if (promo_code.empty()) {
    LOG(ERROR) << "Promo code file " << promo_code_file.value()
               << " is empty";
    return "";
  }

  return promo_code;
}

std::string BuildReferralEndpoint(const std::string& path) {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string referral_server;
  std::string proto = "https";
  env->GetVar("BRAVE_REFERRALS_SERVER", &referral_server);
  if (referral_server.empty())
    referral_server = kBraveReferralsServer;
  if (env->HasVar("BRAVE_REFERRALS_LOCAL"))
    proto = "http";

  return base::StringPrintf("%s://%s%s", proto.c_str(),
                            referral_server.c_str(),
                            path.c_str());
}

}  // namespace

BraveReferralsService::BraveReferralsService(PrefService* pref_service,
                                             const std::string& api_key,
                                             const std::string& platform)
    : initialized_(false),
      task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()})),
      pref_service_(pref_service),
      api_key_(api_key),
      platform_(platform),
      weak_factory_(this) {
  // Track initial profile creation
  if (g_browser_process->profile_manager()) {
    g_browser_process->profile_manager()->AddObserver(this);
    DCHECK_EQ(0U,
              g_browser_process->profile_manager()->GetLoadedProfiles().size());
  }
}

BraveReferralsService::~BraveReferralsService() {
}

void BraveReferralsService::OnProfileAdded(Profile* profile) {
  if (profile == ProfileManager::GetPrimaryUserProfile()) {
    g_browser_process->profile_manager()->RemoveObserver(this);
    Start();
  }
}

void BraveReferralsService::Start() {
  if (initialized_)
    return;

  // Retrieve first run time.
  GetFirstRunTime();

  // Periodically perform finalization checks.
  DCHECK(!finalization_checks_timer_);
  finalization_checks_timer_ = std::make_unique<base::RepeatingTimer>();
  finalization_checks_timer_->Start(
      FROM_HERE,
      base::TimeDelta::FromSeconds(
          brave_base::random::Geometric(kFinalizationChecksFrequency)),
      this, &BraveReferralsService::OnFinalizationChecksTimerFired);
  DCHECK(finalization_checks_timer_->IsRunning());

  // Fetch the referral headers on startup.
  FetchReferralHeaders();

  // Also, periodically fetch the referral headers.
  DCHECK(!fetch_referral_headers_timer_);
  fetch_referral_headers_timer_ = std::make_unique<base::RepeatingTimer>();
  fetch_referral_headers_timer_->Start(
      FROM_HERE,
      base::TimeDelta::FromSeconds(
        brave_base::random::Geometric(kFetchReferralHeadersFrequency)),
      this, &BraveReferralsService::OnFetchReferralHeadersTimerFired);
  DCHECK(fetch_referral_headers_timer_->IsRunning());

  // Read the promo code from user-data-dir and initialize the referral,
  // retrying if necessary.
  bool has_initialized =
      pref_service_->GetBoolean(kReferralInitialization);
  // TODO(keur): This can be removed eventually. This prevents existing
  // users without download_ids from initializing.
  bool checked_for_promo_code_file =
      pref_service_->GetBoolean(kReferralCheckedForPromoCodeFile);
  std::string download_id = pref_service_->GetString(kReferralDownloadID);
  if (!checked_for_promo_code_file && !has_initialized && download_id.empty()) {
#if !defined(OS_ANDROID)
    task_runner_->PostTaskAndReplyWithResult(
        FROM_HERE, base::BindOnce(&ReadPromoCode, GetPromoCodeFileName()),
        base::BindOnce(&BraveReferralsService::OnReadPromoCodeComplete,
                       weak_factory_.GetWeakPtr()));

#else
    InitAndroidReferrer();
#endif
  }

  initialized_ = true;
}

void BraveReferralsService::Stop() {
  initialization_timer_.reset();
  finalization_checks_timer_.reset();
  fetch_referral_headers_timer_.reset();
  initialized_ = false;
}

// static
void BraveReferralsService::SetReferralInitializedCallbackForTesting(
    ReferralInitializedCallback* referral_initialized_callback) {
  g_testing_referral_initialized_callback = referral_initialized_callback;
}
// static
bool BraveReferralsService::IsDefaultReferralCode(const std::string& code) {
  return code == kDefaultPromoCode;
}

// static
bool BraveReferralsService::GetMatchingReferralHeaders(
    const base::ListValue& referral_headers_list,
    const base::DictionaryValue** request_headers_dict,
    const GURL& url) {
  // If the domain for this request matches one of our target domains,
  // set the associated custom headers.
  for (const auto& headers_value : referral_headers_list) {
    const base::Value* domains_list =
        headers_value.FindKeyOfType("domains", base::Value::Type::LIST);
    if (!domains_list) {
      LOG(WARNING) << "Failed to retrieve 'domains' key from referral headers";
      continue;
    }
    const base::Value* headers_dict =
        headers_value.FindKeyOfType("headers", base::Value::Type::DICTIONARY);
    if (!headers_dict) {
      LOG(WARNING) << "Failed to retrieve 'headers' key from referral headers";
      continue;
    }
    for (const auto& domain_value : domains_list->GetList()) {
      URLPattern url_pattern(URLPattern::SCHEME_HTTPS |
                             URLPattern::SCHEME_HTTP);
      url_pattern.SetScheme("*");
      url_pattern.SetHost(domain_value.GetString());
      url_pattern.SetPath("/*");
      url_pattern.SetMatchSubdomains(true);
      if (!url_pattern.MatchesURL(url))
        continue;
      return headers_dict->GetAsDictionary(request_headers_dict);
    }
  }
  return false;
}

void BraveReferralsService::OnFinalizationChecksTimerFired() {
  PerformFinalizationChecks();
}

void BraveReferralsService::OnFetchReferralHeadersTimerFired() {
  FetchReferralHeaders();
}

void BraveReferralsService::OnReferralHeadersLoadComplete(
    std::unique_ptr<std::string> response_body) {
  int response_code = -1;
  if (referral_headers_loader_->ResponseInfo() &&
      referral_headers_loader_->ResponseInfo()->headers)
    response_code =
        referral_headers_loader_->ResponseInfo()->headers->response_code();
  if (referral_headers_loader_->NetError() != net::OK || response_code < 200 ||
      response_code > 299) {
    const std::string safe_response_body =
        response_body ? *response_body : std::string();
    LOG(ERROR) << "Failed to fetch headers from referral server"
               << ", error: " << referral_headers_loader_->NetError()
               << ", response code: " << response_code
               << ", payload: " << safe_response_body
               << ", url: " << referral_headers_loader_->GetFinalURL().spec();
    return;
  }

  base::JSONReader::ValueWithError root =
      base::JSONReader::ReadAndReturnValueWithError(*response_body);
  if (!root.value || !root.value->is_list()) {
    LOG(ERROR) << "Failed to parse referral headers response: "
               << (!root.value ? root.error_message : "not a list");
    return;
  }
  pref_service_->Set(kReferralHeaders, root.value.value());
}

void BraveReferralsService::OnReferralInitLoadComplete(
    std::unique_ptr<std::string> response_body) {
  int response_code = -1;
  if (referral_init_loader_->ResponseInfo() &&
      referral_init_loader_->ResponseInfo()->headers)
    response_code =
        referral_init_loader_->ResponseInfo()->headers->response_code();
  if (referral_init_loader_->NetError() != net::OK || response_code < 200 ||
      response_code > 299) {
    const std::string safe_response_body =
        response_body ? *response_body : std::string();
    LOG(ERROR) << "Failed to initialize referral"
               << ", error: " << referral_init_loader_->NetError()
               << ", response code: " << response_code
               << ", payload: " << safe_response_body
               << ", url: " << referral_init_loader_->GetFinalURL().spec();
    initialization_timer_ = std::make_unique<base::OneShotTimer>();
    initialization_timer_->Start(
                    FROM_HERE,
                    base::TimeDelta::FromSeconds(
                      brave_base::random::Geometric(
                              kReportInitializationFrequency)),
                    this, &BraveReferralsService::InitReferral);
    DCHECK(initialization_timer_->IsRunning());
    return;
  }

  base::JSONReader::ValueWithError root =
      base::JSONReader::ReadAndReturnValueWithError(*response_body);
  if (!root.value || !root.value->is_dict()) {
    LOG(ERROR) << "Failed to parse referral initialization response: "
               << (!root.value ? root.error_message : "not a dictionary");
    return;
  }
  if (!root.value->FindKey("download_id")) {
    LOG(ERROR)
        << "Failed to locate download_id in referral initialization response"
        << ", payload: " << *response_body;
    return;
  }

  const base::Value* headers = root.value->FindKey("headers");
  if (headers) {
    pref_service_->Set(kReferralHeaders, *headers);
  }

  const base::Value* download_id = root.value->FindKey("download_id");
  pref_service_->SetString(kReferralDownloadID, download_id->GetString());

  // We have initialized with the promo server. We can kill the retry timer now.
  pref_service_->SetBoolean(kReferralInitialization, true);
  if (initialization_timer_)
    initialization_timer_.reset();
  if (g_testing_referral_initialized_callback) {
    g_testing_referral_initialized_callback->Run(download_id->GetString());
  }

  const base::Value* offer_page_url = root.value->FindKey("offer_page_url");
  if (offer_page_url) {
    Profile* last_used_profile = ProfileManager::GetLastUsedProfile();
    GURL gurl(offer_page_url->GetString());
    content::OpenURLParams open_url_params(
        gurl, content::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui::PAGE_TRANSITION_AUTO_TOPLEVEL, false);
    open_url_params.extra_headers = FormatExtraHeaders(headers, gurl);
#if defined(OS_ANDROID)
    ServiceTabLauncher::GetInstance()->LaunchTab(
        last_used_profile, open_url_params,
        base::BindOnce([](content::WebContents*) {}));
#else
    chrome::ScopedTabbedBrowserDisplayer browser_displayer(last_used_profile);
    browser_displayer.browser()->OpenURL(open_url_params);
#endif
  }

  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&DeletePromoCodeFile, GetPromoCodeFileName()));
}

void BraveReferralsService::OnReferralFinalizationCheckLoadComplete(
    std::unique_ptr<std::string> response_body) {
  int response_code = -1;
  if (referral_finalization_check_loader_->ResponseInfo() &&
      referral_finalization_check_loader_->ResponseInfo()->headers)
    response_code = referral_finalization_check_loader_->ResponseInfo()
                        ->headers->response_code();
  if (referral_finalization_check_loader_->NetError() != net::OK ||
      response_code < 200 || response_code > 299) {
    const std::string safe_response_body =
        response_body ? *response_body : std::string();
    LOG(ERROR) << "Failed to perform referral finalization check"
               << ", error: " << referral_finalization_check_loader_->NetError()
               << ", response code: " << response_code
               << ", payload: " << safe_response_body << ", url: "
               << referral_finalization_check_loader_->GetFinalURL().spec();
    return;
  }

  base::JSONReader::ValueWithError root =
      base::JSONReader::ReadAndReturnValueWithError(*response_body);
  if (!root.value) {
    LOG(ERROR) << "Failed to parse referral finalization check response: "
               << root.error_message;
    return;
  }
  const base::Value* finalized = root.value->FindKey("finalized");
  if (!finalized->GetBool()) {
    LOG(ERROR) << "Referral is not ready, please wait at least 30 days";
    return;
  }

  // Now that referral is finalized, discard state so we don't check
  // anymore.
  pref_service_->SetTime(kReferralTimestamp, base::Time::Now());
  pref_service_->ClearPref(kReferralAttemptTimestamp);
  pref_service_->ClearPref(kReferralAttemptCount);
}

void BraveReferralsService::OnReadPromoCodeComplete(
    const std::string& promo_code) {
  promo_code_ = promo_code;
  if (!promo_code_.empty() && !IsDefaultReferralCode(promo_code_)) {
    pref_service_->SetString(kReferralPromoCode, promo_code_);
    DCHECK(!initialization_timer_);
    InitReferral();
  } else {
    // store referral code if it's not empty (ex: it's the default code)
    if (!promo_code_.empty()) {
      pref_service_->SetString(kReferralPromoCode, promo_code_);
    }
    // No referral code or it's the default, no point of reporting it.
    pref_service_->SetBoolean(kReferralInitialization, true);
    if (g_testing_referral_initialized_callback) {
      g_testing_referral_initialized_callback->Run(std::string());
    }
  }
}

void BraveReferralsService::GetFirstRunTime() {
#if defined(OS_ANDROID)
  // Android doesn't use a sentinel to track first run, so we use a
  // preference instead.
  base::Time first_run_timestamp =
      pref_service_->GetTime(kReferralAndroidFirstRunTimestamp);
  if (first_run_timestamp.is_null()) {
    first_run_timestamp = base::Time::Now();
    pref_service_->SetTime(kReferralAndroidFirstRunTimestamp,
                           first_run_timestamp);
  }
  SetFirstRunTime(first_run_timestamp);
#else
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&first_run::GetFirstRunSentinelCreationTime),
      base::BindOnce(&BraveReferralsService::SetFirstRunTime,
                     weak_factory_.GetWeakPtr()));
#endif
}

void BraveReferralsService::SetFirstRunTime(
    const base::Time& first_run_timestamp) {
  first_run_timestamp_ = first_run_timestamp;
  if (first_run_timestamp_.is_null())
    return;
  PerformFinalizationChecks();
}

void BraveReferralsService::PerformFinalizationChecks() {
  // Delete the promo code preference, if appropriate.
  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&BraveReferralsService::MaybeDeletePromoCodePref,
                     weak_factory_.GetWeakPtr()));

  // Check for referral finalization, if appropriate.
  base::PostTask(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&BraveReferralsService::MaybeCheckForReferralFinalization,
                     weak_factory_.GetWeakPtr()));
}

// static
void BraveReferralsService::SetPromoFilePathForTesting(
    const base::FilePath& path) {
  g_promo_file_path = path;
}

base::FilePath BraveReferralsService::GetPromoCodeFileName() const {
  if (!g_promo_file_path.empty())
    return g_promo_file_path;

  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  return user_data_dir.AppendASCII("promoCode");
}

void BraveReferralsService::MaybeCheckForReferralFinalization() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::string download_id = pref_service_->GetString(kReferralDownloadID);
  if (download_id.empty()) {
    return;
  }

  // Only check for referral finalization after 30 days have elapsed
  // since first run.
  uint64_t check_time = 30 * 24 * 60 * 60;
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string check_time_str;
  env->GetVar("BRAVE_REFERRALS_CHECK_TIME", &check_time_str);
  if (!check_time_str.empty())
    base::StringToUint64(check_time_str, &check_time);

  base::Time now = base::Time::Now();
  if (now - first_run_timestamp_ < base::TimeDelta::FromSeconds(check_time))
    return;

  // Only check for referral finalization 30 times, with a 24-hour
  // wait between checks.
  base::Time timestamp = pref_service_->GetTime(kReferralAttemptTimestamp);
  int count = pref_service_->GetInteger(kReferralAttemptCount);
  if (count >= 30) {
    pref_service_->ClearPref(kReferralAttemptTimestamp);
    pref_service_->ClearPref(kReferralAttemptCount);
    pref_service_->ClearPref(kReferralDownloadID);
    return;
  }

  if (now - timestamp < base::TimeDelta::FromHours(24))
    return;

  pref_service_->SetTime(kReferralAttemptTimestamp, now);
  pref_service_->SetInteger(kReferralAttemptCount, count + 1);

  CheckForReferralFinalization();
}

void BraveReferralsService::MaybeDeletePromoCodePref() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  uint64_t delete_time = 90 * 24 * 60 * 60;
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string delete_time_str;
  env->GetVar("BRAVE_REFERRALS_DELETE_TIME", &delete_time_str);
  if (!delete_time_str.empty())
    base::StringToUint64(delete_time_str, &delete_time);

  base::Time now = base::Time::Now();
  if (now - first_run_timestamp_ >= base::TimeDelta::FromSeconds(delete_time))
    pref_service_->ClearPref(kReferralPromoCode);
}

std::string BraveReferralsService::BuildReferralInitPayload() const {
  base::Value root(base::Value::Type::DICTIONARY);
  root.SetKey("api_key", base::Value(api_key_));
  root.SetKey("referral_code", base::Value(promo_code_));
  root.SetKey("platform", base::Value(platform_));

  std::string result;
  base::JSONWriter::Write(root, &result);

  return result;
}

std::string BraveReferralsService::BuildReferralFinalizationCheckPayload()
    const {
  base::Value root(base::Value::Type::DICTIONARY);
  root.SetKey("api_key", base::Value(api_key_));
  root.SetKey("download_id",
              base::Value(pref_service_->GetString(kReferralDownloadID)));
#if defined(OS_ANDROID)
  root.SetKey("safetynet_status",
              base::Value(pref_service_->GetString(kSafetynetStatus)));
#endif

  std::string result;
  base::JSONWriter::Write(root, &result);

  return result;
}

void BraveReferralsService::FetchReferralHeaders() {
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation(
        "brave_referral_headers_fetcher", R"(
        semantics {
          sender:
            "Brave Referrals Service"
          description:
            "Fetches referral headers from Brave."
          trigger:
            "An update timer indicates that it's time to fetch referral headers."
          data: "Brave referral headers."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled by settings."
          policy_exception_justification:
            "Not implemented."
        })");
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url =
      GURL(BuildReferralEndpoint(kBraveReferralsHeadersPath));
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES |
                                 net::LOAD_BYPASS_CACHE |
                                 net::LOAD_DISABLE_CACHE;
  network::mojom::URLLoaderFactory* loader_factory =
      g_browser_process->system_network_context_manager()
          ->GetURLLoaderFactory();
  referral_headers_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  referral_headers_loader_->SetAllowHttpErrorResults(true);
  referral_headers_loader_->SetRetryOptions(
      1, network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
  referral_headers_loader_->DownloadToString(
      loader_factory,
      base::BindOnce(&BraveReferralsService::OnReferralHeadersLoadComplete,
                     base::Unretained(this)),
      kMaxReferralServerResponseSizeBytes);
}

void BraveReferralsService::InitReferral() {
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("brave_referral_initializer", R"(
        semantics {
          sender:
            "Brave Referrals Service"
          description:
            "Validates the current referral offer with Brave, potentially "
            "unlocking special features and/or services."
          trigger:
            "On startup, sends the current referral code to Brave."
          data: "Brave referral metadata."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled by settings."
          policy_exception_justification:
            "Not implemented."
        })");
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->method = "PUT";
  resource_request->url = GURL(BuildReferralEndpoint(kBraveReferralsInitPath));
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES |
                                 net::LOAD_BYPASS_CACHE |
                                 net::LOAD_DISABLE_CACHE;
  network::mojom::URLLoaderFactory* loader_factory =
      g_browser_process->system_network_context_manager()
          ->GetURLLoaderFactory();
  referral_init_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  referral_init_loader_->SetAllowHttpErrorResults(true);
  referral_init_loader_->AttachStringForUpload(BuildReferralInitPayload(),
                                               "application/json");
  referral_init_loader_->SetRetryOptions(
      1, network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
  referral_init_loader_->DownloadToString(
      loader_factory,
      base::BindOnce(&BraveReferralsService::OnReferralInitLoadComplete,
                     base::Unretained(this)),
      kMaxReferralServerResponseSizeBytes);
}

#if defined(OS_ANDROID)
void BraveReferralsService::GetSafetynetStatusResult(
    const bool token_received,
    const std::string& result_string,
    const bool attestation_passed) {
  if (pref_service_->GetString(kSafetynetStatus).empty()) {
    NOTREACHED() << "Failed to get safetynet status";
    pref_service_->SetString(kSafetynetStatus, "not verified");
  }
  CheckForReferralFinalization();
}
#endif

void BraveReferralsService::CheckForReferralFinalization() {
#if defined(OS_ANDROID)
  if (pref_service_->GetString(kSafetynetStatus).empty()) {
    // Get safetynet status before finalization
    safetynet_check::ClientAttestationCallback attest_callback =
        base::BindOnce(&BraveReferralsService::GetSafetynetStatusResult,
                       weak_factory_.GetWeakPtr());
    safetynet_check_runner_.performSafetynetCheck(
        "", std::move(attest_callback), true);
    return;
  }
#endif
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("brave_referral_finalization_checker",
        R"(
        semantics {
          sender:
            "Brave Referrals Service"
          description:
            "Fetches referral finalization data from Brave."
          trigger:
            ""
          data: "Brave referral finalization status."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled by settings."
          policy_exception_justification:
            "Not implemented."
        })");
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->method = "PUT";
  resource_request->url =
      GURL(BuildReferralEndpoint(kBraveReferralsActivityPath));
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES |
                                 net::LOAD_BYPASS_CACHE |
                                 net::LOAD_DISABLE_CACHE;
  network::mojom::URLLoaderFactory* loader_factory =
      g_browser_process->system_network_context_manager()
          ->GetURLLoaderFactory();
  referral_finalization_check_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  referral_finalization_check_loader_->SetAllowHttpErrorResults(true);
  referral_finalization_check_loader_->AttachStringForUpload(
      BuildReferralFinalizationCheckPayload(), "application/json");
  referral_finalization_check_loader_->SetRetryOptions(
      1, network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
  referral_finalization_check_loader_->DownloadToString(
      loader_factory,
      base::BindOnce(
          &BraveReferralsService::OnReferralFinalizationCheckLoadComplete,
          base::Unretained(this)),
      kMaxReferralServerResponseSizeBytes);
}

std::string BraveReferralsService::FormatExtraHeaders(
    const base::Value* referral_headers,
    const GURL& url) {
  if (!referral_headers)
    return std::string();

  const base::ListValue* referral_headers_list = nullptr;
  if (!referral_headers->GetAsList(&referral_headers_list))
    return std::string();

  const base::DictionaryValue* request_headers_dict = nullptr;
  if (!GetMatchingReferralHeaders(*referral_headers_list, &request_headers_dict,
                                  url))
    return std::string();

  std::string extra_headers;
  for (const auto& it : request_headers_dict->DictItems()) {
    extra_headers += base::StringPrintf("%s: %s\r\n", it.first.c_str(),
                                        it.second.GetString().c_str());
  }
  if (!extra_headers.empty())
    extra_headers += "\r\n";

  return extra_headers;
}

#if defined(OS_ANDROID)
void BraveReferralsService::InitAndroidReferrer() {
  android_brave_referrer::InitReferrerCallback init_referrer_callback =
      base::BindOnce(&BraveReferralsService::OnAndroidBraveReferrerReady,
                     weak_factory_.GetWeakPtr());
  android_brave_referrer_.InitReferrer(std::move(init_referrer_callback));
}

void BraveReferralsService::OnAndroidBraveReferrerReady() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&ReadPromoCode, GetPromoCodeFileName()),
      base::BindOnce(&BraveReferralsService::OnReadPromoCodeComplete,
                     weak_factory_.GetWeakPtr()));
}
#endif

///////////////////////////////////////////////////////////////////////////////

void RegisterPrefsForBraveReferralsService(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kReferralCheckedForPromoCodeFile, false);
  registry->RegisterBooleanPref(kReferralInitialization, false);
  registry->RegisterStringPref(kReferralPromoCode, std::string());
  registry->RegisterStringPref(kReferralDownloadID, std::string());
  registry->RegisterTimePref(kReferralTimestamp, base::Time());
  registry->RegisterTimePref(kReferralAttemptTimestamp, base::Time());
  registry->RegisterIntegerPref(kReferralAttemptCount, 0);
  registry->RegisterListPref(kReferralHeaders);
#if defined(OS_ANDROID)
  registry->RegisterTimePref(kReferralAndroidFirstRunTimestamp, base::Time());
  registry->RegisterStringPref(kSafetynetStatus, std::string());
#endif
}

}  // namespace brave
