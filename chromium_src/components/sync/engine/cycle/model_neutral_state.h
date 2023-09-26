/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_CYCLE_MODEL_NEUTRAL_STATE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_CYCLE_MODEL_NEUTRAL_STATE_H_

#define last_download_updates_result     \
  Unused();                              \
  std::string last_server_error_message; \
  SyncerError last_download_updates_result

#include "src/components/sync/engine/cycle/model_neutral_state.h"  // IWYU pragma: export

#undef last_download_updates_result

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_CYCLE_MODEL_NEUTRAL_STATE_H_
