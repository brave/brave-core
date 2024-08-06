/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_MODEL_TYPE_WORKER_H_
#define BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_MODEL_TYPE_WORKER_H_

#include "base/feature_list.h"
#include "base/gtest_prod_util.h"
#include "components/sync/base/model_type.h"
#include "components/sync/base/passphrase_enums.h"
#include "components/sync/engine/commit_and_get_updates_types.h"
#include "components/sync/engine/model_type_worker.h"

namespace syncer {

class CancelationSignal;
class Cryptographer;
class NudgeHandler;

namespace features {

BASE_DECLARE_FEATURE(kBraveSyncResetProgressMarker);

}  // namespace features

FORWARD_DECLARE_TEST(BraveModelTypeWorkerTest, ResetProgressMarker);
FORWARD_DECLARE_TEST(BraveModelTypeWorkerTest, ResetProgressMarkerMaxPeriod);
FORWARD_DECLARE_TEST(BraveModelTypeWorkerTest,
                     ResetProgressMarkerDisabledFeature);

class BraveModelTypeWorker : public ModelTypeWorker {
 public:
  BraveModelTypeWorker(ModelType type,
                       const sync_pb::ModelTypeState& initial_state,
                       Cryptographer* cryptographer,
                       bool encryption_enabled,
                       PassphraseType passphrase_type,
                       NudgeHandler* nudge_handler,
                       CancelationSignal* cancelation_signal);
  ~BraveModelTypeWorker() override;
  BraveModelTypeWorker(const BraveModelTypeWorker&) = delete;
  BraveModelTypeWorker& operator=(const BraveModelTypeWorker&) = delete;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveModelTypeWorkerTest, ResetProgressMarker);
  FRIEND_TEST_ALL_PREFIXES(BraveModelTypeWorkerTest,
                           ResetProgressMarkerMaxPeriod);
  FRIEND_TEST_ALL_PREFIXES(BraveModelTypeWorkerTest,
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

#endif  // BRAVE_COMPONENTS_SYNC_ENGINE_BRAVE_MODEL_TYPE_WORKER_H_
