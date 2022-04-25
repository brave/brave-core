/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_CYCLE_STATUS_CONTROLLER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_CYCLE_STATUS_CONTROLLER_H_

#define set_commit_result                                    \
  set_last_server_error_message(const std::string& message); \
                                                             \
 private:                                                    \
  std::string last_server_error_message_;                    \
                                                             \
 public:                                                     \
  std::string get_last_server_error_message() const;         \
  void set_commit_result

#include "src/components/sync/engine/cycle/status_controller.h"

#undef set_commit_result

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SYNC_ENGINE_CYCLE_STATUS_CONTROLLER_H_
