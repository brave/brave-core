/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_GLUE_SYNC_ENGINE_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_GLUE_SYNC_ENGINE_IMPL_H_

#define BRAVE_SYNC_ENGINE_IMPL_H \
  friend SyncEngineHost* BraveGetSyncEngineHost(SyncEngineImpl*);

#include "../../../../../../components/sync/driver/glue/sync_engine_impl.h"  // NOLINT
#undef BRAVE_SYNC_ENGINE_IMPL_H
#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_DRIVER_GLUE_SYNC_ENGINE_IMPL_H_
