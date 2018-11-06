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
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_constants.h"
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

}

AdsServiceImpl::AdsServiceImpl(Profile* profile) :
    profile_(profile),
    file_task_runner_(base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
    base_path_(profile_->GetPath().AppendASCII("ads_service")) {
  DCHECK(!profile_->IsOffTheRecord());

  if (is_enabled())
    Init();
}

AdsServiceImpl::~AdsServiceImpl() {}

void AdsServiceImpl::Init() {
  DCHECK(is_enabled());

  ads_.reset(ads::Ads::CreateInstance(this));
}

bool AdsServiceImpl::is_enabled() {
  return true;
}

void AdsServiceImpl::Save(const std::string& name,
                          const std::string& value,
                          ads::OnSaveCallback callback) {

}

void AdsServiceImpl::Load(const std::string& name,
                          ads::OnLoadCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&LoadOnFileTaskRunner, base_path_.AppendASCII(name)),
      base::BindOnce(&AdsServiceImpl::OnLoaded,
                     AsWeakPtr(),
                     std::move(callback)));
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

const std::vector<std::string>& AdsServiceImpl::GetLocales() const {
  return l10n_util::GetAvailableLocales();
}

std::string AdsServiceImpl::SetLocale(const std::string& locale) {
  return locale;
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
