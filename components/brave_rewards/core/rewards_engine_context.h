/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_CONTEXT_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_CONTEXT_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/supports_user_data.h"

namespace brave_rewards::internal {

class RewardsEngineImpl;

class RewardsEngineContext : private base::SupportsUserData {
 public:
  explicit RewardsEngineContext(RewardsEngineImpl& engine_impl);
  ~RewardsEngineContext() override;

  RewardsEngineContext(const RewardsEngineContext&) = delete;
  RewardsEngineContext& operator=(const RewardsEngineContext&) = delete;

  RewardsEngineImpl& GetEngineImpl() const { return engine_impl_.get(); }

  template <typename T>
  T& GetHelper() const {
    const void* key = &T::kUserDataKey;
    T* helper = static_cast<T*>(GetUserData(key));
    CHECK(helper) << "Rewards engine helper " << T::kUserDataKey
                  << " has not been created";

    return *helper;
  }

 private:
  void AddHelpers();

  template <typename T, typename... Args>
  void AddHelper(Args&&... args) {
    DCHECK(!GetUserData(&T::kUserDataKey))
        << "Rewards engine helper " << T::kUserDataKey
        << " has already been created";
    T* helper = new T(*this, std::forward<Args>(args)...);
    SetUserData(&T::kUserDataKey, std::unique_ptr<T>(helper));
    helper_keys_.push_back(helper);
  }

  const raw_ref<RewardsEngineImpl> engine_impl_;
  std::vector<const void*> helper_keys_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_REWARDS_ENGINE_CONTEXT_H_
