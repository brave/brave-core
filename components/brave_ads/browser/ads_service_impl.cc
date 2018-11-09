/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_service_impl.h"

#include "base/files/file_util.h"
#include "base/guid.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/task_runner_util.h"
#include "base/task/post_task.h"
#include "bat/ads/ads.h"
#include "bat/ads/url_session.h"
#include "brave/components/brave_ads/browser/bundle_state_database.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_constants.h"
#include "content/public/browser/browser_thread.h"
#include "ui/base/l10n/l10n_util.h"


namespace brave_ads {

namespace {

std::string LoadOnFileTaskRunner(
    const base::FilePath& path) {
  std::string data;
  bool success = base::ReadFileToString(path, &data);

  // Make sure the file isn't empty.
  if (!success || data.empty()) {
    LOG(ERROR) << "Failed to read file: " << path.MaybeAsASCII();
    return std::string();
  }
  return data;
}

std::vector<ads::AdInfo> GetAdsForCategoryOnFileTaskRunner(
    const std::string category,
    BundleStateDatabase* backend) {
  std::vector<ads::AdInfo> ads;
  if (!backend)
    return ads;

  backend->GetAdsForCategory(category, ads);

  return ads;
}

bool SaveBundleStateOnFileTaskRunner(
    std::unique_ptr<ads::BUNDLE_STATE> bundle_state,
    BundleStateDatabase* backend) {
  if (backend && backend->SaveBundleState(*bundle_state))
    return true;

  return false;
}

}

AdsServiceImpl::AdsServiceImpl(Profile* profile) :
    profile_(profile),
    file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
    base_path_(profile_->GetPath().AppendASCII("ads_service")),
    next_timer_id_(0),
    bundle_state_backend_(
        new BundleStateDatabase(base_path_.AppendASCII("bundle_state"))) {
  DCHECK(!profile_->IsOffTheRecord());

  if (is_enabled())
    Init();
}

AdsServiceImpl::~AdsServiceImpl() {
  file_task_runner_->DeleteSoon(FROM_HERE, bundle_state_backend_.release());
}

void AdsServiceImpl::Init() {
  DCHECK(is_enabled());

  ads_.reset(ads::Ads::CreateInstance(this));
}

bool AdsServiceImpl::is_enabled() const {
  return true;
}

bool AdsServiceImpl::IsAdsEnabled() const {
  return is_enabled();
}

uint64_t AdsServiceImpl::GetAdsPerHour() const {
  // TODO(bridiver) - implement this
  return 100;
}

uint64_t AdsServiceImpl::GetAdsPerDay() const {
  // TODO(bridiver) - implement this
  return 100;
}

void AdsServiceImpl::Save(const std::string& name,
                          const std::string& value,
                          ads::OnSaveCallback callback) {
  // TODO(bridiver) - implement
}

void AdsServiceImpl::Load(const std::string& name,
                          ads::OnLoadCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&LoadOnFileTaskRunner, base_path_.AppendASCII(name)),
      base::BindOnce(&AdsServiceImpl::OnLoaded,
                     AsWeakPtr(),
                     std::move(callback)));
}

const std::string AdsServiceImpl::Load(const std::string& name) {
  // TODO(bridiver) - this Load method needs to be refactored in bat-native-ads
  // because we can't have synchronous IO operations
  NOTREACHED();
  return "{}";
}

void AdsServiceImpl::SaveBundleState(
    std::unique_ptr<ads::BUNDLE_STATE> bundle_state,
    ads::OnSaveCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&SaveBundleStateOnFileTaskRunner,
                    base::Passed(std::move(bundle_state)),
                    bundle_state_backend_.get()),
      base::BindOnce(&AdsServiceImpl::OnSaveBundleState,
                     AsWeakPtr(),
                     callback));
}

void AdsServiceImpl::OnSaveBundleState(const ads::OnSaveCallback& callback,
                                       bool success) {
  callback(success ? ads::Result::SUCCESS : ads::Result::FAILED);
}

void AdsServiceImpl::OnLoaded(
    const ads::OnLoadCallback& callback,
    const std::string& value) {
  if (value.empty())
    callback(ads::Result::FAILED, value);
  else
    callback(ads::Result::SUCCESS, value);
}

void AdsServiceImpl::Reset(const std::string& name,
                           ads::OnResetCallback callback) {
  // TODO(bridiver) - implement
}

void AdsServiceImpl::GetAdsForCategory(
      const std::string& category,
      ads::OnGetAdsForCategoryCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&GetAdsForCategoryOnFileTaskRunner,
                    category,
                    bundle_state_backend_.get()),
      base::BindOnce(&AdsServiceImpl::OnGetAdsForCategory,
                     AsWeakPtr(),
                     std::move(callback),
                     category));
}

void AdsServiceImpl::OnGetAdsForCategory(
    const ads::OnGetAdsForCategoryCallback& callback,
    const std::string& category,
    const std::vector<ads::AdInfo>& ads) {
  callback(ads.empty() ? ads::Result::FAILED : ads::Result::SUCCESS,
      category,
      ads);
}


void AdsServiceImpl::GetAdsForSampleCategory(
    ads::OnGetAdsForCategoryCallback callback) {
}

const ads::ClientInfo AdsServiceImpl::GetClientInfo() const {
  // TODO(bridiver) - these eventually get used in a catalog request
  // and seem unecessary. Seems like it would be better to get the catalog from
  // S3 and we should actually be filtering this stuff out from the headers when
  // making requests as well
  ads::ClientInfo client_info;
  // this doesn't seem necessary
  client_info.application_version = "";
  // client_info.application_version = chrome::kChromeVersion;

  // this doesn't seem necessary
  client_info.platform = "";
  // client_info.platform = base::OperatingSystemName();

  // this is definitely a privacy issue
  client_info.platform_version = "";

  return client_info;
}

const std::string AdsServiceImpl::GenerateUUID() const {
  return base::GenerateGUID();
}

const std::string AdsServiceImpl::GetSSID() const {
  return "";
}

const std::vector<std::string> AdsServiceImpl::GetLocales() const {
  // TODO(bridiver) - re-implement this
  return l10n_util::GetAvailableLocales();
}

const std::string AdsServiceImpl::GetAdsLocale() const {
  // TODO(bridiver) - implement this
  return "";
}

std::unique_ptr<ads::URLSession> AdsServiceImpl::URLSessionTask(
      const std::string& url,
      const std::vector<std::string>& headers,
      const std::string& content,
      const std::string& content_type,
      const ads::URLSession::Method& method,
      ads::URLSessionCallbackHandlerCallback callback) {
  return nullptr;
}

bool AdsServiceImpl::GetUrlComponents(
      const std::string& url,
      ads::UrlComponents* components) const {
  GURL gurl(url);

  if (!gurl.is_valid())
    return false;

  components->url = gurl.spec();
  if (gurl.has_scheme())
    components->scheme = gurl.scheme();

  if (gurl.has_username())
    components->user = gurl.username();

  if (gurl.has_host())
    components->hostname = gurl.host();

  if (gurl.has_port())
    components->port = gurl.port();

  if (gurl.has_query())
    components->query = gurl.query();

  if (gurl.has_ref())
    components->fragment = gurl.ref();

  return true;
}

uint32_t AdsServiceImpl::SetTimer(const uint64_t& time_offset) {
  if (next_timer_id_ == std::numeric_limits<uint32_t>::max())
    next_timer_id_ = 1;
  else
    ++next_timer_id_;

  timers_[next_timer_id_] = std::make_unique<base::OneShotTimer>();
  timers_[next_timer_id_]->Start(FROM_HERE,
      base::TimeDelta::FromSeconds(time_offset),
      base::BindOnce(
          &AdsServiceImpl::OnTimer, AsWeakPtr(), next_timer_id_));

  return next_timer_id_;
}

void AdsServiceImpl::KillTimer(uint32_t timer_id) {
  if (timers_.find(timer_id) == timers_.end())
    return;

  timers_[timer_id]->Stop();
  timers_.erase(timer_id);
}

void AdsServiceImpl::OnTimer(uint32_t timer_id) {
  timers_.erase(timer_id);
  ads_->OnTimer(timer_id);
}

std::ostream& AdsServiceImpl::Log(const char* file,
                                  int line,
                                  const ads::LogLevel log_level) const {
  switch(log_level) {
    case ads::LogLevel::INFO:
      return logging::LogMessage(file, line, logging::LOG_INFO).stream();
      break;
    case ads::LogLevel::WARNING:
      return logging::LogMessage(file, line, logging::LOG_WARNING).stream();
      break;
    default:
      return logging::LogMessage(file, line, logging::LOG_ERROR).stream();
  }
}

}  // namespace brave_ads
