/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/engine_impl/brave_model_type_worker.h"

#include <utility>

#include "base/logging.h"
#include "components/sync/engine/model_type_processor.h"

namespace syncer {

BraveModelTypeWorker::BraveModelTypeWorker(
    ModelType type,
    const sync_pb::ModelTypeState& initial_state,
    bool trigger_initial_sync,
    std::unique_ptr<Cryptographer> cryptographer,
    PassphraseType passphrase_type,
    NudgeHandler* nudge_handler,
    std::unique_ptr<ModelTypeProcessor> model_type_processor,
    CancelationSignal* cancelation_signal)
    : ModelTypeWorker(type,
                      initial_state,
                      trigger_initial_sync,
                      std::move(cryptographer),
                      passphrase_type,
                      nudge_handler,
                      std::move(model_type_processor),
                      cancelation_signal) {}

BraveModelTypeWorker::~BraveModelTypeWorker() {}

void BraveModelTypeWorker::OnCommitResponse(
    const CommitResponseDataList& committed_response_list,
    const FailedCommitResponseDataList& error_response_list) {
  ModelTypeWorker::OnCommitResponse(committed_response_list,
                                    error_response_list);
}

}  // namespace syncer
