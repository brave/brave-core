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

#include "base/time/time.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"

namespace brave_rewards::internal {

namespace {

struct PublisherStatusData {
  mojom::PublisherStatus status;
  uint64_t updated_at;
};

using PublisherStatusMap = std::map<std::string, PublisherStatusData>;

struct RefreshTaskInfo {
  RefreshTaskInfo(PublisherStatusMap&& status_map,
                  std::function<void(PublisherStatusMap)> callback)
      : map(std::move(status_map)), current(map.begin()), callback(callback) {}

  PublisherStatusMap map;
  PublisherStatusMap::iterator current;
  std::function<void(PublisherStatusMap)> callback;
};

void RefreshNext(std::shared_ptr<RefreshTaskInfo> task_info) {
  DCHECK(task_info);

  // Find the first map element that has an expired status.
  task_info->current = std::find_if(
      task_info->current, task_info->map.end(), [](auto& key_value) {
        mojom::ServerPublisherInfo server_info;
        server_info.status = key_value.second.status;
        server_info.updated_at = key_value.second.updated_at;
        return ledger().publisher()->ShouldFetchServerPublisherInfo(
            &server_info);
      });

  // Execute the callback if no more expired elements are found.
  if (task_info->current == task_info->map.end()) {
    task_info->callback(std::move(task_info->map));
    return;
  }

  // Look for publisher key in hash index.
  auto& key = task_info->current->first;
  ledger().database()->SearchPublisherPrefixList(key, [task_info](bool exists) {
    // If the publisher key does not exist in the hash index look for
    // next expired entry.
    if (!exists) {
      ++task_info->current;
      RefreshNext(task_info);
      return;
    }
    // Fetch current publisher info.
    auto& key = task_info->current->first;
    ledger().publisher()->GetServerPublisherInfo(
        key, [task_info](mojom::ServerPublisherInfoPtr server_info) {
          // Update status map and continue looking for expired entries.
          task_info->current->second.status = server_info->status;
          ++task_info->current;
          RefreshNext(task_info);
        });
  });
}

void RefreshPublisherStatusMap(
    PublisherStatusMap&& status_map,
    std::function<void(PublisherStatusMap)> callback) {
  RefreshNext(
      std::make_shared<RefreshTaskInfo>(std::move(status_map), callback));
}

}  // namespace

namespace publisher {

void RefreshPublisherStatus(std::vector<mojom::PublisherInfoPtr>&& info_list,
                            GetRecurringTipsCallback callback) {
  PublisherStatusMap map;
  for (const auto& info : info_list) {
    map[info->id] = {info->status, info->status_updated_at};
  }

  auto shared_list = std::make_shared<std::vector<mojom::PublisherInfoPtr>>(
      std::move(info_list));

  RefreshPublisherStatusMap(std::move(map), [shared_list, callback](auto map) {
    for (const auto& info : *shared_list) {
      info->status = map[info->id].status;
    }
    callback(std::move(*shared_list));
  });
}

}  // namespace publisher
}  // namespace brave_rewards::internal
