/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_DATA_TYPE_WORKER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_DATA_TYPE_WORKER_H_

#include "base/gtest_prod_util.h"

namespace syncer {

FORWARD_DECLARE_TEST(BraveDataTypeWorkerTest, ResetProgressMarker);
FORWARD_DECLARE_TEST(BraveDataTypeWorkerTest, ResetProgressMarkerMaxPeriod);

}  // namespace syncer

#define BRAVE_MODEL_TYPE_WORKER_H_                                        \
 private:                                                                 \
  friend class BraveDataTypeWorker;                                       \
  friend class BraveDataTypeWorkerTest;                                   \
  FRIEND_TEST_ALL_PREFIXES(BraveDataTypeWorkerTest, ResetProgressMarker); \
  FRIEND_TEST_ALL_PREFIXES(BraveDataTypeWorkerTest,                       \
                           ResetProgressMarkerMaxPeriod);

#define OnCommitResponse virtual OnCommitResponse

#include "src/components/sync/engine/data_type_worker.h"  // IWYU pragma: export

#undef OnCommitResponse
#undef BRAVE_MODEL_TYPE_WORKER_H_

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_DATA_TYPE_WORKER_H_
