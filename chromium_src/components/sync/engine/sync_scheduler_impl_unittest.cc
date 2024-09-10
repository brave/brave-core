/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/components/sync/engine/sync_scheduler_impl_unittest.cc"

namespace syncer {

namespace {

void SimulatePollFailedRegularTransientError(DataTypeSet requested_types,
                                             SyncCycle* cycle) {
  cycle->mutable_status_controller()->set_last_download_updates_result(
      SyncerError::ProtocolError(TRANSIENT_ERROR));
}

void SimulatePollFailedNigoryNotReady(DataTypeSet requested_types,
                                      SyncCycle* cycle) {
  cycle->mutable_status_controller()->set_last_download_updates_result(
      SyncerError::ProtocolError(TRANSIENT_ERROR));

  cycle->mutable_status_controller()->set_last_server_error_message(
      kNigoriFolderNotReadyError);
}

}  // namespace

TEST_F(SyncSchedulerImplTest, BraveNoBackoffOnNigoriError) {
  scheduler()->OnReceivedPollIntervalUpdate(base::Milliseconds(10));
  UseMockDelayProvider();  // Will cause test failure if backoff is initiated.
  EXPECT_CALL(*delay(), GetDelay).WillRepeatedly(Return(base::Milliseconds(0)));

  SyncShareTimes times;
  EXPECT_CALL(*syncer(), PollSyncShare)
      .WillOnce(DoAll(Invoke(SimulatePollFailedNigoryNotReady),
                      RecordSyncShare(&times, false)))
      .WillOnce(DoAll(Invoke(SimulatePollFailedRegularTransientError),
                      RecordSyncShare(&times, false)));

  StartSyncScheduler(base::Time());

  // Nigory folder error should not trigger backoff.
  RunLoop();
  EXPECT_FALSE(scheduler()->IsGlobalBackoff());

  // Regular transient error should trigger backoff.
  RunLoop();
  EXPECT_TRUE(scheduler()->IsGlobalBackoff());
}

}  // namespace syncer
