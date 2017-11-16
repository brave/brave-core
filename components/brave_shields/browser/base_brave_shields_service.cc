/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/base_brave_shields_service.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/brave_shields/browser/dat_file_web_request.h"

namespace brave_shields {

BaseBraveShieldsService::BaseBraveShieldsService(
    const std::string &file_name,
    const GURL &url) :
      file_name_(file_name),
      url_(url),
    initialized_(false) {
}

BaseBraveShieldsService::~BaseBraveShieldsService() {
}

void BaseBraveShieldsService::DownloadDATFile() {
  web_request_.reset(new DATFileWebRequest(
    file_name_,
    url_,
    base::Bind(&BaseBraveShieldsService::DATFileResponse,
      base::Unretained(this))));
  web_request_->Init();
  web_request_->Start();
}

void BaseBraveShieldsService::DATFileResponse(bool success) {
  std::lock_guard<std::mutex> guard(init_mutex_);
  if (!success) {
    LOG(ERROR) << "Could not download DAT file";
    return;
  }
  InitShields();
}

void BaseBraveShieldsService::InitShields() {
  if (Init()) {
    std::lock_guard<std::mutex> guard(initialized_mutex_);
    initialized_ = true;
  }
}

bool BaseBraveShieldsService::Start() {
  base::ThreadRestrictions::AssertIOAllowed();
  std::lock_guard<std::mutex> guard(init_mutex_);
  if (initialized_) {
    return true;
  }

  DownloadDATFile();
  return false;
}

void BaseBraveShieldsService::Stop() {
  std::lock_guard<std::mutex> guard(initialized_mutex_);
  Cleanup();
  initialized_ = false;
}

}  // namespace brave_shields
