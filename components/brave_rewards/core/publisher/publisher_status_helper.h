/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_STATUS_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_STATUS_HELPER_H_

#include <vector>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/rewards_engine_helper.h"

namespace brave_rewards::internal {

class PublisherStatusHelper : public RewardsEngineHelper {
 public:
  ~PublisherStatusHelper() override;

  using RefreshStatusCallback =
      base::OnceCallback<void(std::vector<mojom::PublisherInfoPtr>)>;

  // Refreshes the publisher status for each entry in the specified list.
  void RefreshStatus(std::vector<mojom::PublisherInfoPtr>&& info_list,
                     RefreshStatusCallback callback);

 private:
  friend RewardsEngineContext;

  inline static const char kUserDataKey[] = "publisher-status-helper";

  struct RefreshTaskInfo;

  explicit PublisherStatusHelper(RewardsEngineContext& context);

  void RefreshNext(RefreshTaskInfo task_info);

  void OnPrefixListSearchResult(RefreshTaskInfo task_info, bool exists);

  void OnDatabaseRead(RefreshTaskInfo task_info,
                      mojom::ServerPublisherInfoPtr server_info);

  base::WeakPtrFactory<PublisherStatusHelper> weak_factory_{this};
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_PUBLISHER_STATUS_HELPER_H_
