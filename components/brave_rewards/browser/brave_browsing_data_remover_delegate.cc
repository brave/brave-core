/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/brave_browsing_data_remover_delegate.h"  // NOLINT

#include <stdint.h>

#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/barrier_closure.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/task/post_task.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/io_thread.h"
#include "chrome/browser/profiles/profile.h"
#include "brave/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserContext;
using content::BrowserThread;
using brave_rewards::RewardsService;
using brave_rewards::RewardsServiceFactory;

BraveBrowsingDataRemoverDelegate::BraveBrowsingDataRemoverDelegate(
    BrowserContext* browser_context)
    : ChromeBrowsingDataRemoverDelegate(browser_context),
      profile_(Profile::FromBrowserContext(browser_context)) {
}

BraveBrowsingDataRemoverDelegate::~BraveBrowsingDataRemoverDelegate() {}

void BraveBrowsingDataRemoverDelegate::RemoveEmbedderData(
    const base::Time& delete_begin,
    const base::Time& delete_end,
    int remove_mask,
    const content::BrowsingDataFilterBuilder& filter_builder,
    int origin_type_mask,
    base::OnceClosure callback) {

  ChromeBrowsingDataRemoverDelegate::RemoveEmbedderData(
      delete_begin,
      delete_end,
      remove_mask,
      filter_builder,
      origin_type_mask,
      std::move(callback));

  //////////////////////////////////////////////////////////////////////////////
  // REMOVE REWARDS DATA
  RewardsService* rewards_service_ =
      RewardsServiceFactory::GetForProfile(profile_);
  if (rewards_service_) {
    rewards_service_->RemoveData(
        remove_mask, base::DoNothing());
  }
}
