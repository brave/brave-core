/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/publisher/publisher_status_helper.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <utility>

#include "base/memory/raw_ref.h"
#include "base/time/time.h"
#include "brave/components/brave_rewards/core/common/legacy_callback_helpers.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/publisher/server_publisher_fetcher.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {

namespace {

struct PublisherStatusData {
  mojom::PublisherStatus status;
  uint64_t updated_at;
};

using PublisherStatusMap = std::map<std::string, PublisherStatusData>;

}  // namespace

struct PublisherStatusHelper::RefreshTaskInfo {
  RefreshTaskInfo(std::vector<mojom::PublisherInfoPtr> info_list,
                  RefreshStatusCallback callback)
      : map(std::make_unique<PublisherStatusMap>()),
        list(std::move(info_list)),
        callback(std::move(callback)) {
    for (const auto& info : list) {
      map->emplace(info->id,
                   PublisherStatusData{.status = info->status,
                                       .updated_at = info->status_updated_at});
    }
    current = map->begin();
  }

  RefreshTaskInfo(const RefreshTaskInfo&) = delete;
  RefreshTaskInfo& operator=(const RefreshTaskInfo&) = delete;

  RefreshTaskInfo(RefreshTaskInfo&&) = default;
  RefreshTaskInfo& operator=(RefreshTaskInfo&&) = default;

  std::unique_ptr<PublisherStatusMap> map;
  PublisherStatusMap::iterator current;
  std::vector<mojom::PublisherInfoPtr> list;
  RefreshStatusCallback callback;
};

PublisherStatusHelper::PublisherStatusHelper(RewardsEngineContext& context)
    : RewardsEngineHelper(context) {}

PublisherStatusHelper::~PublisherStatusHelper() = default;

void PublisherStatusHelper::RefreshStatus(
    std::vector<mojom::PublisherInfoPtr>&& info_list,
    RefreshStatusCallback callback) {
  RefreshTaskInfo task_info(std::move(info_list), std::move(callback));
  RefreshNext(std::move(task_info));
}

void PublisherStatusHelper::RefreshNext(RefreshTaskInfo task_info) {
  // Find the next map element that has an expired status.
  task_info.current = std::find_if(
      task_info.current, task_info.map->end(), [this](auto& key_value) {
        mojom::ServerPublisherInfo server_info;
        server_info.status = key_value.second.status;
        server_info.updated_at = key_value.second.updated_at;
        return GetHelper<ServerPublisherFetcher>().IsExpired(server_info);
      });

  // If no more expired elements are found, update the status values in the list
  // and execute the callback.
  if (task_info.current == task_info.map->end()) {
    for (const auto& info : task_info.list) {
      info->status = task_info.map->at(info->id).status;
    }
    std::move(task_info.callback).Run(std::move(task_info.list));
    return;
  }

  // Look for publisher key in hash index.
  context().GetEngineImpl().database()->SearchPublisherPrefixList(
      task_info.current->first,
      ToLegacyCallback(
          base::BindOnce(&PublisherStatusHelper::OnPrefixListSearchResult,
                         weak_factory_.GetWeakPtr(), std::move(task_info))));
}

void PublisherStatusHelper::OnPrefixListSearchResult(RefreshTaskInfo task_info,
                                                     bool exists) {
  // If the publisher key does not exist in the hash index look for next expired
  // entry.
  if (!exists) {
    ++task_info.current;
    RefreshNext(std::move(task_info));
    return;
  }

  // Fetch current publisher info.
  GetHelper<ServerPublisherFetcher>().Fetch(
      task_info.current->first,
      base::BindOnce(&PublisherStatusHelper::OnDatabaseRead,
                     weak_factory_.GetWeakPtr(), std::move(task_info)));
}

void PublisherStatusHelper::OnDatabaseRead(
    RefreshTaskInfo task_info,
    mojom::ServerPublisherInfoPtr server_info) {
  // Update status map and continue looking for expired entries.
  task_info.current->second.status = server_info->status;
  ++task_info.current;
  RefreshNext(std::move(task_info));
}

}  // namespace brave_rewards::internal
