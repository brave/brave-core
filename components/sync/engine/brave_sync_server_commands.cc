/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sync/engine/brave_sync_server_commands.h"

#include <utility>

#include "base/functional/callback.h"
#include "base/logging.h"
#include "base/sequence_checker.h"
#include "components/sync/engine/sync_protocol_error.h"
#include "components/sync/engine/syncer_proto_util.h"
#include "components/sync/protocol/sync.pb.h"

namespace syncer {

namespace {

void InitClearServerDataContext(SyncCycle* cycle,
                                sync_pb::ClientToServerMessage* message) {
  message->set_share(cycle->context()->account_name());
  message->set_message_contents(
      sync_pb::ClientToServerMessage::CLEAR_SERVER_DATA);
}

}  // namespace

void BraveSyncServerCommands::PermanentlyDeleteAccount(
    SyncCycle* cycle,
    base::OnceCallback<void(const SyncProtocolError&)> callback) {
  sync_pb::ClientToServerMessage message;
  InitClearServerDataContext(cycle, &message);
  SyncerProtoUtil::AddRequiredFieldsToClientToServerMessage(cycle, &message);

  sync_pb::ClientToServerResponse response;
  const SyncerError post_result = SyncerProtoUtil::PostClientToServerMessage(
      message, &response, cycle, nullptr);

  if (post_result.value() != SyncerError::SYNCER_OK) {
    VLOG(1) << "[BraveSync] " << __func__
            << " Post ClearServerData failed post_result="
            << post_result.value() << " " << post_result.ToString();
  }

  if (response.has_error_code()) {
    VLOG(1) << "[BraveSync] " << __func__
            << " response.error_code()=" << response.error_code();
  }

  if (response.has_error_message()) {
    VLOG(1) << "[BraveSync] " << __func__
            << " response.error_message()=" << response.error_message();
  }

  SyncProtocolError sync_protocol_error =
      SyncerProtoUtil::GetProtocolErrorFromResponse(response, cycle->context());
  sync_protocol_error.error_description = response.error_message();

  std::move(callback).Run(sync_protocol_error);
}

}  // namespace syncer
