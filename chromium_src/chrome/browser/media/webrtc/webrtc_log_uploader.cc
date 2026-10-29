// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/media/webrtc/webrtc_log_uploader.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"

namespace {

constexpr char kLogStorageDisabledMsg[] = "WebRtc text log storage is disabled";

// Brave drops the logs it is handed, so it is also responsible for disposing
// of the RTP dump files that come with them. Must be called on the background
// sequence.
void DeleteRtpDumps(const WebRtcLogPaths& paths) {
  for (const base::FilePath& dump :
       {paths.incoming_rtp_dump, paths.outgoing_rtp_dump}) {
    if (!dump.empty()) {
      base::DeleteFile(dump);
    }
  }
}

}  // namespace

#include <chrome/browser/media/webrtc/webrtc_log_uploader.cc>
