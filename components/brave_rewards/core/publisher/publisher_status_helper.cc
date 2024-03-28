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
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"

namespace brave_rewards::internal {

namespace {

struct PublisherStatusData {
  mojom::PublisherStatus status;
  uint64_t updated_at;
};

using PublisherStatusMap = std::map<std::string, PublisherStatusData>;

struct RefreshTaskInfo {
  RefreshTaskInfo(RewardsEngine& engine,
                  PublisherStatusMap&& status_map,
                  base::OnceCallback<void(PublisherStatusMap)> callback)
      : engine(engine.GetWeakPtr()),
        map(std::move(status_map)),
        current(map.begin()),
        callback(std::move(callback)) {}

  base::WeakPtr<RewardsEngine> engine;
  PublisherStatusMap map;
  PublisherStatusMap::iterator current;
  base::OnceCallback<void(PublisherStatusMap)> callback;
};

void RefreshNext(std::unique_ptr<RefreshTaskInfo> task_info) {
  // Find the first map element that has an expired status.
  task_info->current = std::find_if(
      task_info->current, task_info->map.end(), [&task_info](auto& key_value) {
        if (!task_info->engine) {
          return false;
        }
        mojom::ServerPublisherInfo server_info;
        server_info.status = key_value.second.status;
        server_info.updated_at = key_value.second.updated_at;
        return task_info->engine->publisher()->ShouldFetchServerPublisherInfo(
            &server_info);
      });

  // Execute the callback if no more expired elements are found.
  if (task_info->current == task_info->map.end()) {
    std::move(task_info->callback).Run(std::move(task_info->map));
    return;
  }

  auto on_prefix_searched = [](std::unique_ptr<RefreshTaskInfo> task_info,
                               bool exists) {
    // If the publisher key does not exist in the hash index look
    // for next expired entry.
    if (!exists || !task_info->engine) {
      ++task_info->current;
      RefreshNext(std::move(task_info));
      return;
    }

    auto on_db_read = [](std::unique_ptr<RefreshTaskInfo> task_info,
                         mojom::ServerPublisherInfoPtr server_info) {
      // Update status map and continue looking for
      // expired entries.
      task_info->current->second.status = server_info->status;
      ++task_info->current;
      RefreshNext(std::move(task_info));
    };

    // Fetch current publisher info.
    auto& key = task_info->current->first;
    task_info->engine->publisher()->GetServerPublisherInfo(
        key, base::BindOnce(on_db_read, std::move(task_info)));
  };

  // Look for publisher key in hash index.
  auto& key = task_info->current->first;
  task_info->engine->database()->SearchPublisherPrefixList(
      key, base::BindOnce(on_prefix_searched, std::move(task_info)));
}

void RefreshPublisherStatusMap(
    RewardsEngine& engine,
    PublisherStatusMap&& status_map,
    base::OnceCallback<void(PublisherStatusMap)> callback) {
  RefreshNext(std::make_unique<RefreshTaskInfo>(engine, std::move(status_map),
                                                std::move(callback)));
}

}  // namespace

namespace publisher {

void RefreshPublisherStatus(RewardsEngine& engine,
                            std::vector<mojom::PublisherInfoPtr>&& info_list,
                            RefreshPublisherStatusCallback callback) {
  PublisherStatusMap map;
  for (const auto& info : info_list) {
    map[info->id] = {info->status, info->status_updated_at};
  }

  auto on_refreshed = [](std::vector<mojom::PublisherInfoPtr> list,
                         RefreshPublisherStatusCallback callback,
                         PublisherStatusMap map) {
    for (const auto& info : list) {
      info->status = map[info->id].status;
    }
    std::move(callback).Run(std::move(list));
  };

  RefreshPublisherStatusMap(
      engine, std::move(map),
      base::BindOnce(on_refreshed, std::move(info_list), std::move(callback)));
}

}  // namespace publisher
}  // namespace brave_rewards::internal
