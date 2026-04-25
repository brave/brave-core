/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GLOBAL_STATE_GLOBAL_STATE_HOLDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GLOBAL_STATE_GLOBAL_STATE_HOLDER_H_

namespace brave_ads {

class GlobalState;

class GlobalStateHolder final {
 public:
  explicit GlobalStateHolder(GlobalState* global_state);

  GlobalStateHolder(const GlobalStateHolder& other) = delete;
  GlobalStateHolder& operator=(const GlobalStateHolder& other) = delete;

  GlobalStateHolder(GlobalStateHolder&& other) noexcept = delete;
  GlobalStateHolder& operator=(GlobalStateHolder&& other) noexcept = delete;

  ~GlobalStateHolder();

  static GlobalState* GetGlobalState();
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_GLOBAL_STATE_GLOBAL_STATE_HOLDER_H_
