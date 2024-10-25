/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/histograms_braveizer.h"

#include "base/functional/bind.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/statistics_recorder.h"
#include "brave/components/p3a_utils/bucket.h"

namespace p3a {

namespace {

// Please keep this list sorted and synced with |DoHistogramBravezation|.
// clang-format off
constexpr const char* kBravezationHistograms[] = {
    "DefaultBrowser.State",
    "Extensions.LoadExtension",
    "Tabs.TabCount",
    "Tabs.TabCountPerLoad",
    "Tabs.WindowCount",
};
// clang-format on

}  // namespace

// static
scoped_refptr<p3a::HistogramsBraveizer> HistogramsBraveizer::Create() {
  auto histogram_braveizer = base::MakeRefCounted<p3a::HistogramsBraveizer>();
  histogram_braveizer->InitCallbacks();
  return histogram_braveizer;
}

HistogramsBraveizer::HistogramsBraveizer() = default;

HistogramsBraveizer::~HistogramsBraveizer() = default;

void HistogramsBraveizer::InitCallbacks() {
  for (const char* histogram_name : kBravezationHistograms) {
    histogram_sample_callbacks_.push_back(
        std::make_unique<
            base::StatisticsRecorder::ScopedHistogramSampleObserver>(
            histogram_name,
            base::BindRepeating(&HistogramsBraveizer::DoHistogramBravetization,
                                this)));
  }
}

// TODO(iefremov): Replace a bunch of 'if's with something more elegant.
// Records the given sample using the proper Brave way.
void HistogramsBraveizer::DoHistogramBravetization(
    const char* histogram_name,
    uint64_t name_hash,
    base::HistogramBase::Sample sample) {
  DCHECK(histogram_name);
  if (strcmp("DefaultBrowser.State", histogram_name) == 0) {
    int answer = 0;
    switch (sample) {
      case 0:  // Not default.
      case 1:  // Default.
        answer = sample;
        break;
      case 2:  // Unknown, merging to "Not default".
        answer = 0;
        break;
      case 3:  // Other mode is default, merging to "Default".
        answer = 1;
        break;
      default:
        return;
    }
    UMA_HISTOGRAM_BOOLEAN("Brave.Core.IsDefault", answer);
  }

  if (strcmp("Extensions.LoadExtension", histogram_name) == 0) {
    int answer = 0;
    if (sample == 1)
      answer = 1;
    else if (2 <= sample && sample <= 4)
      answer = 2;
    else if (sample >= 5)
      answer = 3;

    UMA_HISTOGRAM_EXACT_LINEAR("Brave.Core.NumberOfExtensions", answer, 3);
    return;
  }

  if (strcmp("Tabs.TabCount", histogram_name) == 0 ||
      strcmp("Tabs.TabCountPerLoad", histogram_name) == 0) {
    int answer = 0;
    if (0 <= sample && sample <= 1) {
      answer = 0;
    } else if (2 <= sample && sample <= 5) {
      answer = 1;
    } else if (6 <= sample && sample <= 10) {
      answer = 2;
    } else if (11 <= sample && sample <= 50) {
      answer = 3;
    } else {
      answer = 4;
    }

    UMA_HISTOGRAM_EXACT_LINEAR("Brave.Core.TabCount", answer, 4);
    return;
  }

  if (strcmp("Tabs.WindowCount", histogram_name) == 0) {
    int answer = 0;
    if (sample <= 0) {
      answer = 0;
    } else if (sample == 1) {
      answer = 1;
    } else if (2 <= sample && sample <= 5) {
      answer = 2;
    } else {
      answer = 3;
    }

    UMA_HISTOGRAM_EXACT_LINEAR("Brave.Core.WindowCount.2", answer, 3);
    return;
  }
}

}  // namespace p3a
