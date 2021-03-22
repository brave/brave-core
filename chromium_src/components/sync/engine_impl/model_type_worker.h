/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_IMPL_MODEL_TYPE_WORKER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_IMPL_MODEL_TYPE_WORKER_H_

#include "base/gtest_prod_util.h"

namespace syncer {

FORWARD_DECLARE_TEST(BraveModelTypeWorkerTest, ResetProgressMarker);
FORWARD_DECLARE_TEST(BraveModelTypeWorkerTest, ResetProgressMarkerMaxPeriod);

}  // namespace syncer

#define BRAVE_MODEL_TYPE_WORKER_H_                                         \
 private:                                                                  \
  friend class BraveModelTypeWorker;                                       \
  friend class BraveModelTypeWorkerTest;                                   \
  FRIEND_TEST_ALL_PREFIXES(BraveModelTypeWorkerTest, ResetProgressMarker); \
  FRIEND_TEST_ALL_PREFIXES(BraveModelTypeWorkerTest,                       \
                           ResetProgressMarkerMaxPeriod);

#define OnCommitResponse virtual OnCommitResponse

#include "../../../../../components/sync/engine_impl/model_type_worker.h"

#undef OnCommitResponse
#undef BRAVE_MODEL_TYPE_WORKER_H_

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_IMPL_MODEL_TYPE_WORKER_H_
