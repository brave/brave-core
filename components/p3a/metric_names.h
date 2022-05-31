/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_METRIC_NAMES_H_
#define BRAVE_COMPONENTS_P3A_METRIC_NAMES_H_

#include <memory>
#include <vector>

#include "base/callback_forward.h"
#include "base/metrics/statistics_recorder.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_piece.h"

namespace brave::p3a {

// Register StatisticsRecorder observers for each P3A histogram.
//
// Although the list of reporting metric names is private, clients
// can use this to be notified whenever a measurement is updated.
typedef base::StatisticsRecorder::OnSampleCallback MetricObserverCallback;
typedef std::vector<
    std::unique_ptr<base::StatisticsRecorder::ScopedHistogramSampleObserver>>
    MetricObserverList;

// Take a function pointer and return a list of registered observers.
MetricObserverList RegisterMetricObservers(MetricObserverCallback callback);

// Check whether the named histogram should be reported.
bool IsValidMetric(base::StringPiece histogram_name);

}  // namespace brave::p3a

#endif  // BRAVE_COMPONENTS_P3A_METRIC_NAMES_H_
