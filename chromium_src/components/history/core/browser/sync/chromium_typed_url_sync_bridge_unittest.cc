/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <vector>

#include "base/test/scoped_feature_list.h"
#include "brave/components/brave_sync/features.h"
#include "brave/components/history/core/browser/sync/brave_typed_url_sync_bridge.h"
#include "components/history/core/browser/sync/typed_url_sync_bridge.h"
#include "components/sync/model/model_type_sync_bridge.h"

#define BRAVE_TEST_MEMBERS_DECLARE \
  base::test::ScopedFeatureList scoped_feature_list_;

#define BRAVE_TEST_MEMBERS_INIT          \
  scoped_feature_list_.InitWithFeatures( \
      {}, {brave_sync::features::kBraveSyncSendAllHistory});

#define TypedURLSyncBridge BraveTypedURLSyncBridge

#include "src/components/history/core/browser/sync/typed_url_sync_bridge_unittest.cc"

#undef TypedURLSyncBridge

#undef BRAVE_TEST_MEMBERS_INIT
#undef BRAVE_TEST_MEMBERS_DECLARE
