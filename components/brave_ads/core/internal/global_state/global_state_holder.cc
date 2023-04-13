/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/global_state/global_state_holder.h"

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

namespace brave_ads {

namespace {
GlobalState* g_global_state_instance = nullptr;
}  // namespace

GlobalStateHolder::GlobalStateHolder(GlobalState* global_state) {
  DCHECK(!g_global_state_instance);
  g_global_state_instance = global_state;
}

GlobalStateHolder::~GlobalStateHolder() {
  DCHECK(g_global_state_instance);
  g_global_state_instance = nullptr;
}

// static
GlobalState* GlobalStateHolder::GetGlobalState() {
  return g_global_state_instance;
}

}  // namespace brave_ads
