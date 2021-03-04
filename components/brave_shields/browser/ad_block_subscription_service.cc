/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_subscription_service.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/common/pref_names.h"
//#include "brave/components/adblock_rust_ffi/src/wrapper.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "components/prefs/pref_service.h"

namespace brave_shields {

// Constructor for a new subscription
AdBlockSubscriptionService::AdBlockSubscriptionService(
    const GURL& list_url,
    brave_component_updater::BraveComponent::Delegate* delegate)
    : AdBlockBaseService(delegate), list_url_(list_url), enabled_(true) {}

// Constructor from cached information
AdBlockSubscriptionService::AdBlockSubscriptionService(
    const FilterListSubscriptionInfo& cached_info,
    brave_component_updater::BraveComponent::Delegate* delegate)
    : AdBlockBaseService(delegate),
      list_url_(cached_info.list_url),
      enabled_(cached_info.enabled),
      last_update_attempt_(cached_info.last_update_attempt),
      last_successful_update_attempt_(
          cached_info.last_successful_update_attempt) {}

AdBlockSubscriptionService::~AdBlockSubscriptionService() {}

FilterListSubscriptionInfo AdBlockSubscriptionService::GetInfo() const {
  return FilterListSubscriptionInfo{
      .list_url = list_url_,
      .last_update_attempt = last_update_attempt_,
      .last_successful_update_attempt = last_successful_update_attempt_,
      .enabled = enabled_,
  };
}

void AdBlockSubscriptionService::SetEnabled(bool enabled) {
  enabled_ = enabled;
}

bool AdBlockSubscriptionService::Init() {
  AdBlockBaseService::Init();

  return true;
}

void AdBlockSubscriptionService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<AdBlockSubscriptionService> AdBlockSubscriptionServiceFactory(
    const GURL& list_url,
    brave_component_updater::BraveComponent::Delegate* delegate) {
  return std::make_unique<AdBlockSubscriptionService>(list_url, delegate);
}

std::unique_ptr<AdBlockSubscriptionService> AdBlockSubscriptionServiceFactory(
    const FilterListSubscriptionInfo& cached_info,
    brave_component_updater::BraveComponent::Delegate* delegate) {
  return std::make_unique<AdBlockSubscriptionService>(cached_info, delegate);
}

}  // namespace brave_shields
