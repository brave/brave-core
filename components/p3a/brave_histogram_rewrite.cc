/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/brave_histogram_rewrite.h"

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/statistics_recorder.h"

namespace brave {

namespace {

// Please keep this list sorted and synced with |DoHistogramBravezation|.
constexpr const char* kBravezationHistograms[] = {
    "Bookmarks.Count.OnProfileLoad",
    "DefaultBrowser.State",
    "Extensions.LoadExtension",
    "Tabs.TabCount",
    "Tabs.WindowCount",
};

// TODO(iefremov): Replace a bunch of 'if's with something more elegant.
// Records the given sample using the proper Brave way.
void DoHistogramBravezation(base::StringPiece histogram_name,
                            base::HistogramBase::Sample sample) {
  if ("Bookmarks.Count.OnProfileLoad" == histogram_name) {
    int answer = 0;
    if (0 <= sample && sample < 5)
      answer = 0;
    if (5 <= sample && sample < 20)
      answer = 1;
    if (20 <= sample && sample < 100)
      answer = 2;
    if (sample >= 100)
      answer = 3;

    UMA_HISTOGRAM_EXACT_LINEAR("Brave.Core.BookmarksCountOnProfileLoad", answer,
                               3);
    return;
  }

  if ("DefaultBrowser.State" == histogram_name) {
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
      NOTREACHED();
    }
    UMA_HISTOGRAM_BOOLEAN("Brave.Core.IsDefault", answer);
  }

  if ("Extensions.LoadExtension" == histogram_name) {
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

  if ("Tabs.TabCount" == histogram_name) {
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

  if ("Tabs.WindowCount" == histogram_name) {
    int answer = 0;
    if (0 <= sample && sample <= 1) {
      answer = 0;
    } else if (2 <= sample && sample <= 5) {
      answer = 1;
    } else {
      answer = 2;
    }

    UMA_HISTOGRAM_EXACT_LINEAR("Brave.Core.WindowCount", answer, 2);
    return;
  }
}

}  // namespace

void SetupHistogramsBraveization() {
  for (const char* histogram_name : kBravezationHistograms) {
    base::StatisticsRecorder::SetCallback(
        histogram_name,
        base::BindRepeating(&DoHistogramBravezation, histogram_name));
  }
}

}  // namespace brave
