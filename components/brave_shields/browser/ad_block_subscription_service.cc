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
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "base/util/timer/wall_clock_timer.h"
#include "brave/common/pref_names.h"
#include "brave/components/adblock_rust_ffi/src/wrapper.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/ad_block_service_helper.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

namespace {
const base::TimeDelta kListUpdateInterval = base::TimeDelta::FromDays(7);
}

namespace brave_shields {

FilterListSubscriptionInfo BuildInfoFromDict(const GURL& list_url,
                                             const base::Value* dict) {
  DCHECK(dict);
  DCHECK(dict->is_dict());

  FilterListSubscriptionInfo info;

  info.list_url = list_url;

  double last_update_attempt = *dict->FindDoubleKey("last_update_attempt");
  info.last_update_attempt = base::Time::FromJsTime(last_update_attempt);

  double last_successful_update_attempt =
      *dict->FindDoubleKey("last_successful_update_attempt");
  info.last_successful_update_attempt =
      base::Time::FromJsTime(last_successful_update_attempt);

  info.enabled = *dict->FindBoolKey("enabled");

  return info;
}

// Constructor for a new subscription
// |refresh_callback| will not be called; the new list download should be
// initiated by the caller. |refresh_callback| will only initiate a
// low-priority background download, whereas it should be immediate when
// initiated by a user adding a new subscription.
AdBlockSubscriptionService::AdBlockSubscriptionService(
    const GURL& list_url,
    AdBlockSubscriptionService::RefreshSubscriptionCallback refresh_callback,
    brave_component_updater::BraveComponent::Delegate* delegate)
    : AdBlockBaseService(delegate),
      refresh_callback_(refresh_callback),
      list_url_(list_url),
      enabled_(true) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
}

// Constructor from cached information
AdBlockSubscriptionService::AdBlockSubscriptionService(
    const FilterListSubscriptionInfo& cached_info,
    AdBlockSubscriptionService::RefreshSubscriptionCallback refresh_callback,
    brave_component_updater::BraveComponent::Delegate* delegate)
    : AdBlockBaseService(delegate),
      refresh_callback_(refresh_callback),
      list_url_(cached_info.list_url),
      enabled_(cached_info.enabled),
      last_update_attempt_(cached_info.last_update_attempt),
      last_successful_update_attempt_(
          cached_info.last_successful_update_attempt) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  GetTaskRunner()->PostTask(
      FROM_HERE, base::BindOnce(&AdBlockSubscriptionService::ReloadFilters,
                                base::Unretained(this)));

  const base::Time next_update = last_update_attempt_ + kListUpdateInterval;
  const base::TimeDelta until_next_update = next_update - base::Time::Now();
  // Schedule the next update for the list - either immediately, or in the
  // future with a delayed task
  if (until_next_update <= base::TimeDelta::FromSeconds(0)) {
    content::GetUIThreadTaskRunner({base::TaskPriority::BEST_EFFORT})
        ->PostTask(FROM_HERE, refresh_callback_);
  } else {
    update_timer_.Start(FROM_HERE, next_update, refresh_callback_);
  }
}

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

void AdBlockSubscriptionService::ReloadFilters() {
  DCHECK(GetTaskRunner()->RunsTasksInCurrentSequence());

  base::FilePath list_location =
      DirForCustomSubscription(list_url_).AppendASCII(
          kCustomSubscriptionListText);

  std::string filters;
  if (base::ReadFileToString(list_location, &filters)) {
    ad_block_client_.reset(new adblock::Engine(filters.c_str()));
  }
}

void AdBlockSubscriptionService::OnSuccessfulDownload() {
  DCHECK(GetTaskRunner()->RunsTasksInCurrentSequence());

  last_update_attempt_ = base::Time::Now();
  last_successful_update_attempt_ = last_update_attempt_;

  ReloadFilters();

  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(&AdBlockSubscriptionService::ScheduleRefreshOnUIThread,
                     base::Unretained(this),
                     last_update_attempt_ + kListUpdateInterval));
}

void AdBlockSubscriptionService::ScheduleRefreshOnUIThread(
    base::Time next_download_time) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  update_timer_.Start(FROM_HERE, next_download_time, refresh_callback_);
}

void AdBlockSubscriptionService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {}

///////////////////////////////////////////////////////////////////////////////

std::unique_ptr<AdBlockSubscriptionService> AdBlockSubscriptionServiceFactory(
    const GURL& list_url,
    AdBlockSubscriptionService::RefreshSubscriptionCallback refresh_callback,
    brave_component_updater::BraveComponent::Delegate* delegate) {
  return std::make_unique<AdBlockSubscriptionService>(
      list_url, refresh_callback, delegate);
}

std::unique_ptr<AdBlockSubscriptionService> AdBlockSubscriptionServiceFactory(
    const FilterListSubscriptionInfo& cached_info,
    AdBlockSubscriptionService::RefreshSubscriptionCallback refresh_callback,
    brave_component_updater::BraveComponent::Delegate* delegate) {
  return std::make_unique<AdBlockSubscriptionService>(
      cached_info, refresh_callback, delegate);
}

}  // namespace brave_shields
