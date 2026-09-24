/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_DATA_TYPE_WORKER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_DATA_TYPE_WORKER_H_

#include "base/gtest_prod_util.h"

// These forward declarations for the Brave test fixtures must precede the
// upstream header so its (plaster-inserted) FRIEND_TEST_ALL_PREFIXES entries
// resolve. The `virtual` and `friend` changes themselves live in the plaster
// at brave/rewrite/components/sync/engine/data_type_worker.h.yaml.
namespace syncer {

FORWARD_DECLARE_TEST(BraveDataTypeWorkerTest, ResetProgressMarker);
FORWARD_DECLARE_TEST(BraveDataTypeWorkerTest, ResetProgressMarkerMaxPeriod);

}  // namespace syncer

#include <components/sync/engine/data_type_worker.h>  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_DATA_TYPE_WORKER_H_
