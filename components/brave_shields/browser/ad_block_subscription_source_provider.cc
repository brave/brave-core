/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_subscription_source_provider.h"

#include <utility>

#include "base/logging.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_shields/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

namespace brave_shields {

AdBlockSubscriptionSourceProvider::AdBlockSubscriptionSourceProvider(
    PrefService* local_state,
    base::FilePath list_file)
    : list_file_(list_file) {}

AdBlockSubscriptionSourceProvider::~AdBlockSubscriptionSourceProvider() {}

void AdBlockSubscriptionSourceProvider::ReloadListFromDisk() {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::ReadDATFileData, list_file_),
      base::BindOnce(&AdBlockSubscriptionSourceProvider::OnListSourceLoaded,
                     weak_factory_.GetWeakPtr()));
}

void AdBlockSubscriptionSourceProvider::LoadDATBuffer(
    base::OnceCallback<void(bool deserialize, const DATFileDataBuffer& dat_buf)>
        cb) {
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock()},
      base::BindOnce(&brave_component_updater::ReadDATFileData, list_file_),
      base::BindOnce(std::move(cb), false));
}

}  // namespace brave_shields
