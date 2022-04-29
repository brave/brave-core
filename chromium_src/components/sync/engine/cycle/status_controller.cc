/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "src/components/sync/engine/cycle/status_controller.cc"

namespace syncer {

const std::string& StatusController::get_last_server_error_message() const {
  return model_neutral_.last_server_error_message;
}

void StatusController::set_last_server_error_message(
    const std::string& message) {
  model_neutral_.last_server_error_message = message;
}

}  // namespace syncer
