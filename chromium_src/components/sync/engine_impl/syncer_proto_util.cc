/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/sync/engine_impl/syncer_proto_util.h"
#define AddRequiredFieldsToClientToServerMessage AddRequiredFieldsToClientToServerMessage_ChromiumImpl  // NOLINT
#define PostClientToServerMessage PostClientToServerMessage_ChromiumImpl
#include "../../../../../components/sync/engine_impl/syncer_proto_util.cc"  // NOLINT
#undef PostClientToServerMessage
#undef AddRequiredFieldsToClientToServerMessage

namespace syncer {
namespace {
}   // namespace

// static
void SyncerProtoUtil::AddRequiredFieldsToClientToServerMessage(
    const SyncCycle* cycle,
    sync_pb::ClientToServerMessage* msg) {}

// static
SyncerError SyncerProtoUtil::PostClientToServerMessage(
    const ClientToServerMessage& msg,
    ClientToServerResponse* response,
    SyncCycle* cycle,
    ModelTypeSet* partial_failure_data_types) {
  return SyncerError(SyncerError::SYNCER_OK);
}

}   // namespace syncer
