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

namespace brave_shields {

BaseBraveShieldsService::BaseBraveShieldsService() :
    initialized_(false) {
}

BaseBraveShieldsService::~BaseBraveShieldsService() {
}

bool BaseBraveShieldsService::IsInitialized() const {
  return initialized_;
}

void BaseBraveShieldsService::InitShields() {
  if (Init()) {
    std::lock_guard<std::mutex> guard(initialized_mutex_);
    initialized_ = true;
  }
}

bool BaseBraveShieldsService::Start() {
  std::lock_guard<std::mutex> guard(init_mutex_);
  if (initialized_) {
    return true;
  }

  InitShields();
  return false;
}

void BaseBraveShieldsService::Stop() {
  std::lock_guard<std::mutex> guard(initialized_mutex_);
  Cleanup();
  initialized_ = false;
}

bool BaseBraveShieldsService::ShouldStartRequest(const GURL& url,
    content::ResourceType resource_type,
    const std::string& tab_host) {
  return true;
}

}  // namespace brave_shields
