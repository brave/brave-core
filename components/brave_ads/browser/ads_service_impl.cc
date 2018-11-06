/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/ads_service_impl.h"

#include "base/files/file_util.h"
#include "base/guid.h"
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

//read comment about file pathes at src\base\files\file_path.h
#if defined(OS_WIN)
const base::FilePath::StringType kUserModelPath(L"user_model");
#else
const base::FilePath::StringType kUserModelPath("user_model");
#endif

std::string LoadUserModelOnFileTaskRunner(
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
    user_model_path_(profile_->GetPath().Append(kUserModelPath)) {
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

void AdsServiceImpl::LoadUserModel(ads::LoadUserModelCallback callback) {
  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&LoadUserModelOnFileTaskRunner, user_model_path_),
      base::BindOnce(&AdsServiceImpl::OnUserModelLoaded,
                     AsWeakPtr(),
                     std::move(callback)));
}

void AdsServiceImpl::OnUserModelLoaded(
    const ads::LoadUserModelCallback& callback,
    const std::string& user_model) {
  if (user_model.empty())
    callback(ads::Result::FAILED, user_model);
  else
    callback(ads::Result::SUCCESS, user_model);
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

}  // namespace brave_ads
