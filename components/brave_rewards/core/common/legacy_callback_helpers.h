/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_LEGACY_CALLBACK_HELPERS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_LEGACY_CALLBACK_HELPERS_H_

#include <functional>
#include <memory>
#include <utility>

#include "base/functional/callback.h"

namespace brave_rewards::internal {

// Converts a `OnceCallback` into an equivalent `std::function`. This adapter
// should only be used to interface with legacy code that uses `std::function`
// for callbacks. Use `OnceCallback` for all new code.
template <typename... Args>
std::function<void(Args...)> ToLegacyCallback(
    base::OnceCallback<void(Args...)> callback) {
  return [shared_callback = std::make_shared<decltype(callback)>(
              std::move(callback))](Args... args) {
    std::move(*shared_callback).Run(std::forward<Args>(args)...);
  };
}

template <typename... Args>
base::OnceCallback<void(Args...)> WrapLegacyCallback(
    std::function<void(Args...)> callback) {
  return base::BindOnce(
      [](decltype(callback) wrapped, Args... args) {
        wrapped(std::forward<Args>(args)...);
      },
      std::move(callback));
}

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_LEGACY_CALLBACK_HELPERS_H_
