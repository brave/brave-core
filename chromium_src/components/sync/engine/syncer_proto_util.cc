/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define BRAVE_SYNCER_PROTO_UTIL_POST_CLIENT_TO_SERVER_MESSAGE \
  SaveServerErrorMessage(*response, cycle->mutable_status_controller());

#include "src/components/sync/engine/syncer_proto_util.cc"

#undef BRAVE_SYNCER_PROTO_UTIL_POST_CLIENT_TO_SERVER_MESSAGE

namespace syncer {

// static
void SyncerProtoUtil::SaveServerErrorMessage(
    const sync_pb::ClientToServerResponse& response,
    StatusController* status_controller) {
  if (response.has_error_message()) {
    status_controller->set_last_server_error_message(response.error_message());
  } else {
    status_controller->set_last_server_error_message(std::string());
  }
}

}  // namespace syncer
