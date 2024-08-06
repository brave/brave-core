/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/functional/bind.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/time/time_override.h"
#include "brave/components/sync/engine/brave_data_type_worker.h"
#include "components/sync/engine/cancelation_signal.h"
#include "components/sync/nigori/cryptographer_impl.h"
#include "components/sync/protocol/sync.pb.h"
#include "components/sync/test/fake_cryptographer.h"
#include "components/sync/test/mock_data_type_processor.h"
#include "components/sync/test/mock_nudge_handler.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::subtle::ScopedTimeClockOverrides;
using base::subtle::TimeNowIgnoringOverride;
using sync_pb::CommitResponse_ResponseType;
using sync_pb::CommitResponse_ResponseType_CONFLICT;
using sync_pb::CommitResponse_ResponseType_TRANSIENT_ERROR;
using sync_pb::ModelTypeState;

namespace syncer {

class BraveDataTypeWorkerTest : public ::testing::Test {
 protected:
  explicit BraveDataTypeWorkerTest(ModelType model_type = PREFERENCES)
      : model_type_(model_type) {}

  ~BraveDataTypeWorkerTest() override = default;

  void NormalInitialize() {
    ModelTypeState initial_state;
    initial_state.mutable_progress_marker()->set_data_type_id(
        GetSpecificsFieldNumberFromModelType(model_type_));
    initial_state.mutable_progress_marker()->set_token(
        "some_saved_progress_token");

    initial_state.set_initial_sync_state(
        sync_pb::ModelTypeState_InitialSyncState_INITIAL_SYNC_DONE);

    InitializeWithState(model_type_, initial_state);

    nudge_handler()->ClearCounters();
  }

  void InitializeWithState(const ModelType type, const ModelTypeState& state) {
    DCHECK(!worker());

    auto processor = std::make_unique<MockDataTypeProcessor>();
    processor->SetDisconnectCallback(base::BindOnce(
        &BraveDataTypeWorkerTest::DisconnectProcessor, base::Unretained(this)));

    worker_ = std::make_unique<BraveDataTypeWorker>(
        type, state, &cryptographer_, /*encryption_enabled=*/false,
        PassphraseType::kImplicitPassphrase, &mock_nudge_handler_,
        &cancelation_signal_);
    worker_->ConnectSync(std::move(processor));
  }

  // Callback when processor got disconnected with sync.
  void DisconnectProcessor() {
    DCHECK(!is_processor_disconnected_);
    is_processor_disconnected_ = true;
  }

  BraveDataTypeWorker* worker() { return worker_.get(); }
  MockNudgeHandler* nudge_handler() { return &mock_nudge_handler_; }

  bool IsProgressMarkerEmpty() {
    return worker()->model_type_state_.progress_marker().token().empty();
  }

  void FillProgressMarker() {
    worker()->model_type_state_.mutable_progress_marker()->set_token("TOKEN1");
  }

 private:
  base::test::SingleThreadTaskEnvironment task_environment;
  const ModelType model_type_;
  FakeCryptographer cryptographer_;
  CancelationSignal cancelation_signal_;
  std::unique_ptr<BraveDataTypeWorker> worker_;
  MockNudgeHandler mock_nudge_handler_;
  bool is_processor_disconnected_ = false;
};

namespace {

base::TimeDelta g_overridden_time_delta;
base::Time g_overridden_now;

std::unique_ptr<ScopedTimeClockOverrides> OverrideForTimeDelta(
    base::TimeDelta overridden_time_delta,
    const base::Time& now = TimeNowIgnoringOverride()) {
  g_overridden_time_delta = overridden_time_delta;
  g_overridden_now = now;
  return std::make_unique<ScopedTimeClockOverrides>(
      []() { return g_overridden_now + g_overridden_time_delta; }, nullptr,
      nullptr);
}

base::TimeDelta g_minimal_time_between_reset_marker;

std::unique_ptr<ScopedTimeClockOverrides> AdvanceTimeToAllowResetMarker() {
  DCHECK(!g_minimal_time_between_reset_marker.is_zero());
  static base::TimeDelta override_total_delta;
  override_total_delta += g_minimal_time_between_reset_marker;
  return OverrideForTimeDelta(override_total_delta);
}

FailedCommitResponseDataList MakeErrorResponseList(
    CommitResponse_ResponseType err_code) {
  FailedCommitResponseData data;
  data.response_type = err_code;
  return FailedCommitResponseDataList({data});
}

}  // namespace

TEST_F(BraveDataTypeWorkerTest, ResetProgressMarker) {
  g_minimal_time_between_reset_marker =
      BraveDataTypeWorker::MinimalTimeBetweenResetForTests();

  NormalInitialize();

  CommitResponse_ResponseType err_codes[] = {
      CommitResponse_ResponseType_CONFLICT,
      CommitResponse_ResponseType_TRANSIENT_ERROR};

  std::unique_ptr<ScopedTimeClockOverrides> time_override;
  for (const auto& err : err_codes) {
    {
      auto local_time_override = AdvanceTimeToAllowResetMarker();
      // Cleanup failures counter and setup progress marker
      worker()->OnCommitResponse(CommitResponseDataList(),
                                 FailedCommitResponseDataList());
      FillProgressMarker();
    }

    for (size_t i = 0;
         i < BraveDataTypeWorker::GetFailuresToResetMarkerForTests() - 1; ++i) {
      auto local_time_override = AdvanceTimeToAllowResetMarker();
      worker()->OnCommitResponse(CommitResponseDataList(),
                                 MakeErrorResponseList(err));
      EXPECT_FALSE(IsProgressMarkerEmpty());
    }

    auto local_time_override = AdvanceTimeToAllowResetMarker();
    worker()->OnCommitResponse(CommitResponseDataList(),
                               MakeErrorResponseList(err));
    EXPECT_TRUE(IsProgressMarkerEmpty());
  }
}

TEST_F(BraveDataTypeWorkerTest, ResetProgressMarkerMaxPeriod) {
  NormalInitialize();
  auto error_response_list =
      MakeErrorResponseList(CommitResponse_ResponseType_CONFLICT);

  for (size_t i = 0;
       i < BraveDataTypeWorker::GetFailuresToResetMarkerForTests() - 1; ++i) {
    worker()->OnCommitResponse(CommitResponseDataList(), error_response_list);
    EXPECT_FALSE(IsProgressMarkerEmpty());
  }

  worker()->OnCommitResponse(CommitResponseDataList(), error_response_list);
  EXPECT_TRUE(IsProgressMarkerEmpty());

  // Doing the same, expecting reset marker will not happened because the
  // allowed period not yet passed

  // Cleanup failures counter and setup progress marker
  worker()->OnCommitResponse(CommitResponseDataList(),
                             FailedCommitResponseDataList());
  FillProgressMarker();

  for (size_t i = 0;
       i < BraveDataTypeWorker::GetFailuresToResetMarkerForTests() - 1; ++i) {
    worker()->OnCommitResponse(CommitResponseDataList(), error_response_list);
    EXPECT_FALSE(IsProgressMarkerEmpty());
  }

  // Expect reset progress marker types not happened
  worker()->OnCommitResponse(CommitResponseDataList(), error_response_list);
  EXPECT_FALSE(IsProgressMarkerEmpty());
}

TEST_F(BraveDataTypeWorkerTest, ResetProgressMarkerDisabledFeature) {
  base::test::ScopedFeatureList feature_list;
  feature_list.InitAndDisableFeature(features::kBraveSyncResetProgressMarker);

  NormalInitialize();
  auto error_response_list =
      MakeErrorResponseList(CommitResponse_ResponseType_CONFLICT);

  for (size_t i = 0;
       i < BraveDataTypeWorker::GetFailuresToResetMarkerForTests() - 1; ++i) {
    worker()->OnCommitResponse(CommitResponseDataList(), error_response_list);
    EXPECT_FALSE(IsProgressMarkerEmpty());
  }

  // Expect reset progress marker types does not happened, because
  // we have disabled feature
  worker()->OnCommitResponse(CommitResponseDataList(), error_response_list);
  EXPECT_FALSE(IsProgressMarkerEmpty());
}

}  // namespace syncer
