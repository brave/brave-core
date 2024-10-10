/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_referrals/browser/brave_referrals_service.h"

#include <memory>
#include <type_traits>
#include <vector>

#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/task/thread_pool.h"
#include "base/values.h"
#include "brave/brave_domains/service_domains.h"
#include "brave/components/brave_referrals/common/pref_names.h"
#include "brave/components/constants/network_constants.h"
#include "brave/components/constants/pref_names.h"
#include "brave_base/random.h"
#include "build/build_config.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

#if BUILDFLAG(IS_MAC)
#include "base/apple/bundle_locations.h"
#include "brave/components/brave_referrals/browser/file_extended_attribute.h"
#endif  // BUILDFLAG(IS_MAC)

// Perform finalization checks once a day.
const int kFinalizationChecksFrequency = 60 * 60 * 24;

// Report initialization once a day (after initial failure).
const int kReportInitializationFrequency = 60 * 60 * 24;

// Maximum size of the referral server response in bytes.
const int kMaxReferralServerResponseSizeBytes = 1024 * 1024;

// Default promo code, used when no promoCode file exists on first
// run.
constexpr char kDefaultPromoCode[] = "BRV001";

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

#if BUILDFLAG(IS_MAC)
std::string ReadPromoCodeFromXattr() {
  static constexpr char kRefCodeAttr[] = "com.brave.refcode";
  base::FilePath bundle_path = base::apple::OuterBundlePath();
  std::vector<char> value;
  int result_errno =
      brave::GetFileExtendedAttribute(bundle_path, kRefCodeAttr, &value);
  if (result_errno == ENOATTR) {
    VLOG(0) << "Could not get promo code from " << bundle_path
            << ". The extended attribute " << kRefCodeAttr << " was not found.";
  } else if (result_errno != 0) {
    VLOG(0) << "Could not get promo code from " << bundle_path
            << ". An error occurred getting value for attribute "
            << kRefCodeAttr << ". Error code: " << result_errno << ".";
  } else {
    std::string promo_code(value.begin(), value.end());
    base::TrimWhitespaceASCII(promo_code, base::TRIM_ALL, &promo_code);
    if (promo_code.empty()) {
      VLOG(0) << "Promo code value from " << bundle_path
              << "'s extended attribute " << kRefCodeAttr << " is empty.";
    } else {
      DVLOG(1) << "Promo code from " << kRefCodeAttr << ": " << promo_code;
      return promo_code;
    }
  }

  return "";
}
#endif  // BUILDFLAG(IS_MAC)

std::string ReadPromoCode(const base::FilePath& promo_code_file) {
  std::string promo_code;

#if BUILDFLAG(IS_MAC)
  promo_code = ReadPromoCodeFromXattr();
  if (!promo_code.empty()) {
    return promo_code;
  }
#endif  // BUILDFLAG(IS_MAC)

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
    LOG(ERROR) << "Promo code file " << promo_code_file.value() << " is empty";
    return "";
  }

  return promo_code;
}

std::string BuildReferralEndpoint(const std::string& path) {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string referral_server;
  env->GetVar("BRAVE_REFERRALS_SERVER", &referral_server);
  if (referral_server.empty()) {
    auto referral_domain = brave_domains::GetServicesDomain("usage-ping");
    referral_server = base::StrCat(
        {url::kHttpsScheme, url::kStandardSchemeSeparator, referral_domain});
  }

  return referral_server + path;
}

}  // namespace

BraveReferralsService::BraveReferralsService(PrefService* pref_service,
                                             const std::string& api_key,
                                             const std::string& platform)
    : task_runner_(
          base::ThreadPool::CreateSequencedTaskRunner({base::MayBlock()})),
      pref_service_(pref_service),
      api_key_(api_key),
      platform_(platform) {}

BraveReferralsService::~BraveReferralsService() = default;

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
      base::Seconds(
          brave_base::random::Geometric(kFinalizationChecksFrequency)),
      this, &BraveReferralsService::OnFinalizationChecksTimerFired);
  DCHECK(finalization_checks_timer_->IsRunning());

  // Read the promo code from user-data-dir and initialize the referral,
  // retrying if necessary.
  bool has_initialized = pref_service_->GetBoolean(kReferralInitialization);
  // TODO(keur): This can be removed eventually. This prevents existing
  // users without download_ids from initializing.
  bool checked_for_promo_code_file =
      pref_service_->GetBoolean(kReferralCheckedForPromoCodeFile);
  std::string download_id = pref_service_->GetString(kReferralDownloadID);
  if (!checked_for_promo_code_file && !has_initialized && download_id.empty()) {
#if !BUILDFLAG(IS_ANDROID)
    task_runner_->PostTaskAndReplyWithResult(
        FROM_HERE, base::BindOnce(&ReadPromoCode, GetPromoCodeFileName()),
        base::BindOnce(&BraveReferralsService::OnReadPromoCodeComplete,
                       weak_factory_.GetWeakPtr()));

#else
    InitAndroidReferrer();
#endif
  }

  initialized_ = true;

  DCHECK(delegate_);
  delegate_->OnInitialized();
}

void BraveReferralsService::Stop() {
  initialization_timer_.reset();
  finalization_checks_timer_.reset();
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

void BraveReferralsService::OnFinalizationChecksTimerFired() {
  PerformFinalizationChecks();
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
    initialization_timer_->Start(FROM_HERE,
                                 base::Seconds(brave_base::random::Geometric(
                                     kReportInitializationFrequency)),
                                 this, &BraveReferralsService::InitReferral);
    DCHECK(initialization_timer_->IsRunning());
    return;
  }

  auto parsed_json =
      base::JSONReader::ReadAndReturnValueWithError(*response_body);
  if (!parsed_json.has_value() || !parsed_json->is_dict()) {
    LOG(ERROR) << "Failed to parse referral initialization response: "
               << (!parsed_json.has_value() ? parsed_json.error().message
                                            : "not a dictionary");
    return;
  }

  const auto& root = parsed_json->GetDict();
  const auto* download_id = root.FindString("download_id");
  if (!download_id) {
    LOG(ERROR)
        << "Failed to locate download_id in referral initialization response"
        << ", payload: " << *response_body;
    return;
  }
  pref_service_->SetString(kReferralDownloadID, *download_id);

  // We have initialized with the promo server. We can kill the retry timer now.
  pref_service_->SetBoolean(kReferralInitialization, true);
  if (initialization_timer_)
    initialization_timer_.reset();
  if (g_testing_referral_initialized_callback) {
    g_testing_referral_initialized_callback->Run(*download_id);
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

  auto parsed_json =
      base::JSONReader::ReadAndReturnValueWithError(*response_body);
  if (!parsed_json.has_value() || !parsed_json->is_dict()) {
    LOG(ERROR) << "Failed to parse referral finalization check response: "
               << (!parsed_json.has_value() ? parsed_json.error().message
                                            : "not a dictionary");
    return;
  }
  const auto& root = parsed_json->GetDict();
  auto finalized = root.FindBool("finalized");
  if (!finalized.has_value() || !finalized.value()) {
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
#if BUILDFLAG(IS_ANDROID)
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
  DCHECK(delegate_);
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE, delegate_->GetFirstRunSentinelCreationTimeCallback(),
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
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&BraveReferralsService::MaybeDeletePromoCodePref,
                     weak_factory_.GetWeakPtr()));

  // Check for referral finalization, if appropriate.
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
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

  DCHECK(delegate_);
  return delegate_->GetUserDataDirectory().AppendASCII("promoCode");
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
  if (now - first_run_timestamp_ < base::Seconds(check_time))
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

  if (now - timestamp < base::Hours(24))
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
  if (now - first_run_timestamp_ >= base::Seconds(delete_time))
    pref_service_->ClearPref(kReferralPromoCode);
}

std::string BraveReferralsService::BuildReferralInitPayload() const {
  base::Value::Dict root;
  root.Set("api_key", api_key_);
  root.Set("referral_code", promo_code_);
  root.Set("platform", platform_);

  std::string result;
  base::JSONWriter::Write(root, &result);

  return result;
}

std::string BraveReferralsService::BuildReferralFinalizationCheckPayload()
    const {
  base::Value::Dict root;
  root.Set("api_key", api_key_);
  root.Set("download_id", pref_service_->GetString(kReferralDownloadID));
#if BUILDFLAG(IS_ANDROID)
  root.Set("safetynet_status", pref_service_->GetString(kSafetynetStatus));
#endif

  std::string result;
  base::JSONWriter::Write(root, &result);

  return result;
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
  DCHECK(delegate_);
  network::mojom::URLLoaderFactory* loader_factory =
      delegate_->GetURLLoaderFactory();
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

#if BUILDFLAG(IS_ANDROID)
void BraveReferralsService::GetSafetynetStatusResult(
    const bool token_received,
    const std::string& result_string,
    const bool attestation_passed) {
  if (pref_service_->GetString(kSafetynetStatus).empty()) {
    // The device could not support SafetyNet.
    pref_service_->SetString(kSafetynetStatus, "not verified");
  }
  CheckForReferralFinalization();
}
#endif

void BraveReferralsService::CheckForReferralFinalization() {
#if BUILDFLAG(IS_ANDROID)
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
  DCHECK(delegate_);
  network::mojom::URLLoaderFactory* loader_factory =
      delegate_->GetURLLoaderFactory();
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

#if BUILDFLAG(IS_ANDROID)
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
#if BUILDFLAG(IS_ANDROID)
  registry->RegisterTimePref(kReferralAndroidFirstRunTimestamp, base::Time());
  registry->RegisterStringPref(kSafetynetStatus, std::string());
#endif
}

}  // namespace brave
