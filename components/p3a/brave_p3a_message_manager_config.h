/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_MESSAGE_MANAGER_CONFIG_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_MESSAGE_MANAGER_CONFIG_H_

#include "base/time/time.h"
#include "url/gurl.h"

namespace brave {

struct MessageManagerConfig {
  // The average interval between uploading different values.
  base::TimeDelta average_upload_interval;
  bool randomize_upload_interval = true;
  // Interval between rotations, only used for testing from the command line.
  base::TimeDelta rotation_interval;

  GURL p3a_upload_server_url;
  GURL p2a_upload_server_url;

  bool ignore_server_errors = false;

  MessageManagerConfig();

  void LoadFromCommandLine();
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_MESSAGE_MANAGER_CONFIG_H_
