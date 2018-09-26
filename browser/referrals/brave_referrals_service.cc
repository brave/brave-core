/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/referrals/brave_referrals_service.h"

#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/path_service.h"
#include "base/rand_util.h"
#include "base/strings/string_util.h"
#include "base/sys_info.h"
#include "base/task/post_task.h"
#include "base/values.h"
#include "brave/common/network_constants.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/scoped_tabbed_browser_displayer.h"
#include "chrome/common/chrome_paths.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"

// Fetch headers from the referral server once a day.
const int kFetchReferralHeadersFrequency = 60 * 60 * 24;

// Maximum size of the referral server response in bytes.
const int kMaxReferralServerResponseSizeBytes = 1024 * 1024;

namespace {

std::string GetPlatformIdentifier() {
#if defined(OS_WIN)
  if (base::SysInfo::OperatingSystemArchitecture() == "x86")
    return "winia32";
  else
    return "winx64";
#elif defined(OS_MACOSX)
  return "osx";
#elif defined(OS_LINUX)
  return "linux";
#else
  return std::string();
#endif
}

std::string BuildReferralEndpoint(const std::string& path) {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string referral_server;
  env->GetVar("BRAVE_REFERRALS_SERVER", &referral_server);
  if (referral_server.empty())
    referral_server = kBraveReferralsServer;
  return base::StringPrintf("https://%s%s", referral_server.c_str(),
                            path.c_str());
}

} // namespace

namespace brave {

BraveReferralsService::BraveReferralsService(PrefService* pref_service)
    : initialized_(false),
      task_runner_(
          base::CreateSequencedTaskRunnerWithTraits({base::MayBlock()})),
      pref_service_(pref_service),
      weak_factory_(this) {
}

BraveReferralsService::~BraveReferralsService() {
}

void BraveReferralsService::Start() {
  if (initialized_)
    return;

  // Retrieve first run sentinel creation time.
  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&BraveReferralsService::GetFirstRunTime,
                                    base::Unretained(this)));

  // Periodically fetch referral headers.
  DCHECK(!fetch_referral_headers_timer_);
  fetch_referral_headers_timer_ = std::make_unique<base::RepeatingTimer>();
  fetch_referral_headers_timer_->Start(
      FROM_HERE,
      base::TimeDelta::FromSeconds(kFetchReferralHeadersFrequency +
                                   base::RandInt(0, 60 * 10)),
      this, &BraveReferralsService::OnFetchReferralHeadersTimerFired);
  DCHECK(fetch_referral_headers_timer_->IsRunning());

  // On first run, read the promo code from user-data-dir and
  // initialize the referral.
  std::string download_id = pref_service_->GetString(kReferralDownloadID);
  if (download_id.empty() && first_run::IsChromeFirstRun())
    task_runner_->PostTaskAndReply(
        FROM_HERE,
        base::Bind(&BraveReferralsService::PerformFirstRunTasks,
                   base::Unretained(this)),
        base::Bind(&BraveReferralsService::OnFirstRunTasksComplete,
                   weak_factory_.GetWeakPtr()));
  else
    FetchReferralHeaders();

  initialized_ = true;
}

void BraveReferralsService::Stop() {
  fetch_referral_headers_timer_.reset();
  initialized_ = false;
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

  std::unique_ptr<base::Value> root = base::JSONReader().ReadToValue(*response_body);
  if (!root || !root->is_list()) {
    LOG(ERROR) << "Failed to parse referral headers response";
    return;
  }
  pref_service_->Set(kReferralHeaders, *root);
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
    return;
  }

  std::unique_ptr<base::Value> root = base::JSONReader().ReadToValue(*response_body);
  if (!root || !root->is_dict()) {
    LOG(ERROR) << "Failed to parse referral initialization response";
    return;
  }
  if (!root->FindKey("download_id")) {
    LOG(ERROR) << "Failed to locate download_id in referral initialization response"
               << ", payload: " << *response_body;
    return;
  }

  const base::Value* offer_page_url = root->FindKey("offer_page_url");
  if (offer_page_url) {
    chrome::ScopedTabbedBrowserDisplayer browser_displayer(
        ProfileManager::GetLastUsedProfile());
    browser_displayer.browser()->OpenURL(content::OpenURLParams(
        GURL(offer_page_url->GetString()), content::Referrer(),
        WindowOpenDisposition::NEW_FOREGROUND_TAB,
        ui::PAGE_TRANSITION_AUTO_TOPLEVEL, false));
  }

  if (root->FindKey("headers")) {
    const base::Value* headers = root->FindKey("headers");
    pref_service_->Set(kReferralHeaders, *headers);
  }

  const base::Value* download_id = root->FindKey("download_id");
  pref_service_->SetString(kReferralDownloadID, download_id->GetString());

  const base::Value* referral_code = root->FindKey("referral_code");
  pref_service_->SetString(kReferralPromoCode, referral_code->GetString());

  task_runner_->PostTask(FROM_HERE,
                         base::Bind(&BraveReferralsService::DeletePromoCodeFile,
                                    base::Unretained(this)));
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

  std::unique_ptr<base::Value> root = base::JSONReader().ReadToValue(*response_body);
  if (!root || !root->is_list()) {
    LOG(ERROR) << "Failed to parse referral finalization check response";
    return;
  }
  const base::Value* finalized = root->FindKey("finalized");
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

void BraveReferralsService::OnFirstRunTasksComplete() {
  if (!promo_code_.empty())
    InitReferral();
}

void BraveReferralsService::PerformFirstRunTasks() {
  ReadPromoCode();
}

void BraveReferralsService::GetFirstRunTime() {
  first_run_timestamp_ = first_run::GetFirstRunSentinelCreationTime();
  if (first_run_timestamp_.is_null())
    return;

  // Delete the promo code preference, if appropriate.
  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&BraveReferralsService::MaybeDeletePromoCodePref,
                     base::Unretained(this)));

  // Check for referral finalization, if appropriate.
  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::UI},
      base::BindOnce(&BraveReferralsService::MaybeCheckForReferralFinalization,
                     base::Unretained(this)));
}

base::FilePath BraveReferralsService::GetPromoCodeFileName() const {
  base::FilePath user_data_dir;
  base::PathService::Get(chrome::DIR_USER_DATA, &user_data_dir);
  return user_data_dir.AppendASCII("promoCode");
}

bool BraveReferralsService::ReadPromoCode() {
  base::FilePath promo_code_file = GetPromoCodeFileName();
  if (!base::PathExists(promo_code_file)) {
    return false;
  }
  if (!base::ReadFileToString(promo_code_file, &promo_code_)) {
    LOG(ERROR) << "Failed to read referral promo code from "
               << promo_code_file.value().c_str();
    return false;
  }
  base::TrimWhitespaceASCII(promo_code_, base::TRIM_ALL, &promo_code_);
  if (promo_code_.empty()) {
    LOG(ERROR) << "Promo code file " << promo_code_file.value().c_str()
               << " is empty";
    return false;
  }
  return true;
}

void BraveReferralsService::DeletePromoCodeFile() const {
  base::FilePath promo_code_file = GetPromoCodeFileName();
  if (!base::DeleteFile(promo_code_file, false)) {
    LOG(ERROR) << "Failed to delete referral promo code file "
               << promo_code_file.value().c_str();
    return;
  }
}

void BraveReferralsService::MaybeCheckForReferralFinalization() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::string download_id = pref_service_->GetString(kReferralDownloadID);
  if (download_id.empty()) {
    return;
  }

  // Only check for referral finalization after 30 days have elapsed
  // since first run.
  base::Time now = base::Time::Now();
  if (now - first_run_timestamp_ < base::TimeDelta::FromDays(30))
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
  base::Time now = base::Time::Now();
  if (now - first_run_timestamp_ >= base::TimeDelta::FromDays(90))
    pref_service_->ClearPref(kReferralPromoCode);
}

std::string BraveReferralsService::BuildReferralInitPayload() const {
  std::string api_key = BRAVE_REFERRALS_API_KEY;
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (env->HasVar("BRAVE_REFERRALS_API_KEY"))
    env->GetVar("BRAVE_REFERRALS_API_KEY", &api_key);

  base::Value root(base::Value::Type::DICTIONARY);
  root.SetKey("api_key", base::Value(api_key));
  root.SetKey("referral_code", base::Value(promo_code_));
  root.SetKey("platform", base::Value(GetPlatformIdentifier()));

  std::string result;
  base::JSONWriter::Write(root, &result);

  return result;
}

std::string BraveReferralsService::BuildReferralFinalizationCheckPayload() const {
  std::string api_key = BRAVE_REFERRALS_API_KEY;
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  if (env->HasVar("BRAVE_REFERRALS_API_KEY"))
    env->GetVar("BRAVE_REFERRALS_API_KEY", &api_key);

  base::Value root(base::Value::Type::DICTIONARY);
  root.SetKey("api_key", base::Value(api_key));
  root.SetKey("download_id",
              base::Value(pref_service_->GetString(kReferralDownloadID)));

  std::string result;
  base::JSONWriter::Write(root, &result);

  return result;
}

void BraveReferralsService::FetchReferralHeaders() {
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("brave_referral_headers_fetcher", R"(
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
  resource_request->load_flags =
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES |
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
      net::LOAD_DO_NOT_SEND_AUTH_DATA;
  network::mojom::URLLoaderFactory* loader_factory =
      g_browser_process->system_network_context_manager()
          ->GetURLLoaderFactory();
  referral_headers_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  referral_headers_loader_->SetAllowHttpErrorResults(true);
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
  resource_request->load_flags =
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES |
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
      net::LOAD_DO_NOT_SEND_AUTH_DATA;
  network::mojom::URLLoaderFactory* loader_factory =
      g_browser_process->system_network_context_manager()
          ->GetURLLoaderFactory();
  referral_init_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  referral_init_loader_->SetAllowHttpErrorResults(true);
  referral_init_loader_->AttachStringForUpload(BuildReferralInitPayload(),
                                               "application/json");
  referral_init_loader_->DownloadToString(
      loader_factory,
      base::BindOnce(&BraveReferralsService::OnReferralInitLoadComplete,
                     base::Unretained(this)),
      kMaxReferralServerResponseSizeBytes);
}

void BraveReferralsService::CheckForReferralFinalization() {
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("brave_referral_finalization_checker", R"(
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
  resource_request->load_flags =
      net::LOAD_DO_NOT_SEND_COOKIES | net::LOAD_DO_NOT_SAVE_COOKIES |
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
      net::LOAD_DO_NOT_SEND_AUTH_DATA;
  network::mojom::URLLoaderFactory* loader_factory =
      g_browser_process->system_network_context_manager()
          ->GetURLLoaderFactory();
  referral_finalization_check_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  referral_finalization_check_loader_->SetAllowHttpErrorResults(true);
  referral_finalization_check_loader_->AttachStringForUpload(
      BuildReferralFinalizationCheckPayload(), "application/json");
  referral_finalization_check_loader_->DownloadToString(
      loader_factory,
      base::BindOnce(
          &BraveReferralsService::OnReferralFinalizationCheckLoadComplete,
          base::Unretained(this)),
      kMaxReferralServerResponseSizeBytes);
}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<BraveReferralsService> BraveReferralsServiceFactory(PrefService* pref_service) {
  return std::make_unique<BraveReferralsService>(pref_service);
}

void RegisterPrefsForBraveReferralsService(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(kReferralPromoCode, std::string());
  registry->RegisterStringPref(kReferralDownloadID, std::string());
  registry->RegisterStringPref(kReferralTimestamp, std::string());
  registry->RegisterTimePref(kReferralAttemptTimestamp, base::Time());
  registry->RegisterIntegerPref(kReferralAttemptCount, 0);
  registry->RegisterListPref(kReferralHeaders);
}

}  // namespace brave
