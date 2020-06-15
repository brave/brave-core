/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P2A_BRAVE_P2A_SWITCHES_H_
#define BRAVE_COMPONENTS_P2A_BRAVE_P2A_SWITCHES_H_

namespace brave {
namespace switches {

// Interval between sending two values.
constexpr char kP2AUploadIntervalSeconds[] = "p2a-upload-interval-seconds";

// Avoid upload interval randomization.
constexpr char kP2ADoNotRandomizeUploadInterval[] =
    "p2a-do-not-randomize-upload-interval";

// Interval between restarting the uploading process for all gathered values.
constexpr char kP2ARotationIntervalSeconds[] = "p2a-rotation-interval-seconds";

// P2A cloud backend URL.
constexpr char kP2AUploadServerUrl[] = "p2a-upload-server-url";

// Do not try to resent values even if a cloud returned an HTTP error, just
// continue the normal process.
constexpr char kP2AIgnoreServerErrors[] = "p2a-ignore-server-errors";

}  // namespace switches
}  // namespace brave

#endif  // BRAVE_COMPONENTS_P2A_BRAVE_P2A_SWITCHES_H_
