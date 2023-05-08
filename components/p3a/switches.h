/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_SWITCHES_H_
#define BRAVE_COMPONENTS_P3A_SWITCHES_H_

namespace p3a {
namespace switches {

// Interval between sending two values.
constexpr char kP3AUploadIntervalSeconds[] = "p3a-upload-interval-seconds";

// Avoid upload interval randomization.
constexpr char kP3ADoNotRandomizeUploadInterval[] =
    "p3a-do-not-randomize-upload-interval";

// Interval between restarting the uploading process for all gathered values.
constexpr char kP3ATypicalRotationIntervalSeconds[] =
    "p3a-rotation-interval-seconds";

// Interval between restarting the uploading process for all gathered values.
constexpr char kP3AExpressRotationIntervalSeconds[] =
    "p3a-express-rotation-interval-seconds";

// Interval between restarting the uploading process for all gathered values.
constexpr char kP3ASlowRotationIntervalSeconds[] =
    "p3a-slow-rotation-interval-seconds";

// For specifying a fake STAR epoch, for the purpose of
// triggering the transmission of encrypted measurements before they are
// due to be sent, for testing purposes.
constexpr char kP3AFakeStarEpoch[] = "p3a-fake-star-epoch";

// P3A cloud backend URL.
constexpr char kP3AJsonUploadUrl[] = "p3a-json-upload-url";
constexpr char kP3ACreativeUploadUrl[] = "p3a-creative-upload-url";
constexpr char kP2AJsonUploadUrl[] = "p2a-json-upload-url";
constexpr char kP3AConstellationUploadUrl[] = "p3a-constellation-upload-url";

constexpr char kP3ADisableStarAttestation[] = "p3a-disable-star-attestation";
// Do not try to resent values even if a cloud returned an HTTP error, just
// continue the normal process.
constexpr char kP3AIgnoreServerErrors[] = "p3a-ignore-server-errors";

constexpr char kP3AStarRandomnessHost[] = "p3a-star-randomness-host";

}  // namespace switches
}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_SWITCHES_H_
