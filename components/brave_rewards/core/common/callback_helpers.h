/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_CALLBACK_HELPERS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_CALLBACK_HELPERS_H_

#include <utility>

#include "base/functional/callback.h"
#include "base/location.h"
#include "base/task/sequenced_task_runner.h"

namespace brave_rewards::internal {

template <typename Callback, typename... Args>
void DeferCallback(base::Location location, Callback callback, Args&&... args) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      location,
      base::BindOnce(std::move(callback), std::forward<Args>(args)...));
}

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_CALLBACK_HELPERS_H_
