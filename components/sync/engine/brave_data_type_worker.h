/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_DATA_TYPE_WORKER_H_
#define BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_DATA_TYPE_WORKER_H_

#include "base/feature_list.h"
#include "base/gtest_prod_util.h"
#include "components/sync/base/data_type.h"
#include "components/sync/base/passphrase_enums.h"
#include "components/sync/engine/commit_and_get_updates_types.h"
#include "components/sync/engine/data_type_worker.h"

namespace syncer {

class CancelationSignal;
class Cryptographer;
class NudgeHandler;

namespace features {

BASE_DECLARE_FEATURE(kBraveSyncResetProgressMarker);

}  // namespace features

FORWARD_DECLARE_TEST(BraveDataTypeWorkerTest, ResetProgressMarker);
FORWARD_DECLARE_TEST(BraveDataTypeWorkerTest, ResetProgressMarkerMaxPeriod);
FORWARD_DECLARE_TEST(BraveDataTypeWorkerTest,
                     ResetProgressMarkerDisabledFeature);

class BraveDataTypeWorker : public DataTypeWorker {
 public:
  BraveDataTypeWorker(DataType type,
                      const sync_pb::DataTypeState& initial_state,
                      Cryptographer* cryptographer,
                      bool encryption_enabled,
                      PassphraseType passphrase_type,
                      NudgeHandler* nudge_handler,
                      CancelationSignal* cancelation_signal);
  ~BraveDataTypeWorker() override;
  BraveDataTypeWorker(const BraveDataTypeWorker&) = delete;
  BraveDataTypeWorker& operator=(const BraveDataTypeWorker&) = delete;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveDataTypeWorkerTest, ResetProgressMarker);
  FRIEND_TEST_ALL_PREFIXES(BraveDataTypeWorkerTest,
                           ResetProgressMarkerMaxPeriod);
  FRIEND_TEST_ALL_PREFIXES(BraveDataTypeWorkerTest,
                           ResetProgressMarkerDisabledFeature);

  void OnCommitResponse(
      const CommitResponseDataList& committed_response_list,
      const FailedCommitResponseDataList& error_response_list) override;

  bool IsResetProgressMarkerRequired(
      const FailedCommitResponseDataList& error_response_list);
  void ResetProgressMarker();

  size_t failed_commit_times_ = 0;
  base::Time last_reset_marker_time_;
  static size_t GetFailuresToResetMarkerForTests();
  static base::TimeDelta MinimalTimeBetweenResetForTests();
};

}  // namespace syncer

#endif  // BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_DATA_TYPE_WORKER_H_
