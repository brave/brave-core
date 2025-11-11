/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_PREF_NAMES_H_
#define BRAVE_COMPONENTS_P3A_PREF_NAMES_H_

namespace p3a {

inline constexpr char kP3AEnabled[] = "brave.p3a.enabled";
inline constexpr char kP3ANoticeAcknowledged[] =
    "brave.p3a.notice_acknowledged";
inline constexpr char kActivationDatesDictPref[] = "p3a.activation_dates";
inline constexpr char kRemoteMetricStorageDictPref[] =
    "p3a.remote_metric_storage";

// Metric log store preference names
inline constexpr char kTypicalJsonLogPrefName[] = "p3a.logs";    // DEPRECATED
inline constexpr char kSlowJsonLogPrefName[] = "p3a.logs_slow";  // DEPRECATED
inline constexpr char kExpressJsonLogPrefName[] =
    "p3a.logs_express";  // DEPRECATED
inline constexpr char kTypicalConstellationPrepPrefName[] =
    "p3a.logs_constellation_prep";
inline constexpr char kSlowConstellationPrepPrefName[] =
    "p3a.logs_constellation_prep_slow";
inline constexpr char kExpressConstellationPrepPrefName[] =
    "p3a.logs_constellation_prep_express";

// Rotation scheduler preference names
inline constexpr char kLastSlowJsonRotationTimeStampPref[] =
    "p3a.last_slow_rotation_timestamp";  // DEPRECATED
inline constexpr char kLastTypicalJsonRotationTimeStampPref[] =
    "p3a.last_rotation_timestamp";  // DEPRECATED
inline constexpr char kLastExpressJsonRotationTimeStampPref[] =
    "p3a.last_express_rotation_timestamp";  // DEPRECATED
inline constexpr char kLastTypicalConstellationRotationTimeStampPref[] =
    "p3a.last_constellation_rotation_timestamp";
inline constexpr char kLastSlowConstellationRotationTimeStampPref[] =
    "p3a.last_slow_constellation_rotation_timestamp";
inline constexpr char kLastExpressConstellationRotationTimeStampPref[] =
    "p3a.last_express_constellation_rotation_timestamp";

// P3A service preference names
inline constexpr char kDynamicMetricsDictPref[] = "p3a.dynamic_metrics";

// Star randomness meta preference names
inline constexpr char kApprovedCertFPPrefName[] = "brave.p3a.approved_cert_fp";
inline constexpr char kRandomnessMetaDictPrefName[] =
    "brave.p3a.randomness_meta";

// Constellation log store preference names
inline constexpr char kTypicalConstellationLogsPrefName[] =
    "p3a.constellation_logs";
inline constexpr char kSlowConstellationLogsPrefName[] =
    "p3a.constellation_logs_slow";
inline constexpr char kExpressV2ConstellationLogsPrefName[] =
    "p3a.constellation_logs_express_v2";

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_PREF_NAMES_H_
