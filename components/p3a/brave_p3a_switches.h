/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_BRAVE_P3A_SWITCHES_H_
#define BRAVE_COMPONENTS_P3A_BRAVE_P3A_SWITCHES_H_

namespace brave {
namespace switches {

// Enables data uploading.
constexpr char kP3AUploadEnabled[] = "p3a-upload-enabled";

// Interval between sending two values.
constexpr char kP3AUploadIntervalSeconds[] = "p3a-upload-interval-seconds";

// The upload interval divided by this value gives us a beginning of an interval
// within which we pick the actual random delay.
constexpr char kP3AUploadIntervalRandomizationDivisor[] =
    "p3a-upload-interval-randomization-divisor";

// Interval between restarting the uploading process for all gathered values.
constexpr char kP3ARotationIntervalSeconds[] = "p3a-rotation-interval-seconds";

// P3A cloud backend URL.
constexpr char kP3AUploadServerUrl[] = "p3a-upload-server-url";

// Do not try to resent values even if a cloud returned an HTTP error, just
// continue the normal process.
constexpr char kP3AIgnoreServerErrors[] = "p3a-ignore-server-errors";

}  // namespace switches
}  // namespace brave

#endif  // BRAVE_COMPONENTS_P3A_BRAVE_P3A_PREF_NAMES_H_

