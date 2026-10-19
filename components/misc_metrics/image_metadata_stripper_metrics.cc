/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/image_metadata_stripper_metrics.h"

#include <array>

#include "base/check.h"
#include "base/location.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace image_metadata_stripper::metrics {

namespace {

constexpr base::TimeDelta kReportInterval = base::Days(7);

// 1-5, 6-10, 11-20, 21-30, 31+
constexpr int kCountBuckets[] = {5, 10, 20, 30};

// Maps a count stored in local state to the histogram it is reported under.
struct HistogramDetails {
  const char* histogram_name;
  const char* count_key;
};

constexpr auto kHistogramsToReport = std::to_array<HistogramDetails>({
    {
        kDownloadFBMDCountStrippedHistogramName,
        kDownloadStrippedPrefKey,
    },
    {
        kUploadFBMDCountStrippedHistogramName,
        kUploadStrippedPrefKey,
    },
});

}  // namespace

ImageMetadataStripperMetrics::ImageMetadataStripperMetrics(
    PrefService* local_state)
    : local_state_(local_state) {
  CHECK(local_state_);
  MaybeReport();
}

ImageMetadataStripperMetrics::~ImageMetadataStripperMetrics() = default;

// static
void ImageMetadataStripperMetrics::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kRootPrefKey, {});
  registry->RegisterTimePref(kLastRecordedTimePref, {});
}

void ImageMetadataStripperMetrics::RecordFbmd(StrippingClient client) {
  const char* count_key = nullptr;
  switch (client) {
    case StrippingClient::kDownloadManager:
      count_key = kDownloadStrippedPrefKey;
      break;
    case StrippingClient::kFileSelect:
    case StrippingClient::kDragDrop:
      count_key = kUploadStrippedPrefKey;
      break;
  }
  CHECK(count_key);

  ScopedDictPrefUpdate update(local_state_, kRootPrefKey);

  base::DictValue* fbmd = update->EnsureDict(kFbmdPrefKey);

  fbmd->Set(count_key, fbmd->FindInt(count_key).value_or(0) + 1);
}

void ImageMetadataStripperMetrics::MaybeReport() {
  const base::Time now = base::Time::Now();
  const base::Time last_recorded_time =
      local_state_->GetTime(kLastRecordedTimePref);

  // A last recorded time in the future (e.g. the clock was moved back) is
  // treated as an expired window, otherwise reporting would stall until the
  // clock catches up.
  const bool within_last_report_window =
      !last_recorded_time.is_null() && last_recorded_time <= now &&
      now - last_recorded_time < kReportInterval;

  // We are still in the previous window, so wait for it to end. The window
  // start must not be moved, otherwise every restart would push the report
  // further out.
  if (within_last_report_window) {
    report_timer_.Start(FROM_HERE, last_recorded_time + kReportInterval, this,
                        &ImageMetadataStripperMetrics::MaybeReport);
    return;
  }

  local_state_->SetTime(kLastRecordedTimePref, now);
  report_timer_.Start(FROM_HERE, now + kReportInterval, this,
                      &ImageMetadataStripperMetrics::MaybeReport);

  // First run would not have any data to emit.
  if (last_recorded_time.is_null()) {
    return;
  }

  const base::DictValue& root = local_state_->GetDict(kRootPrefKey);
  const base::DictValue* fbmd = root.FindDict(kFbmdPrefKey);
  if (!fbmd) {
    local_state_->ClearPref(kRootPrefKey);
    return;
  }
  for (const auto& details : kHistogramsToReport) {
    const int count = fbmd->FindInt(details.count_key).value_or(0);
    if (count > 0) {
      p3a_utils::RecordToHistogramBucket(details.histogram_name, kCountBuckets,
                                         count);
    }
  }
  local_state_->ClearPref(kRootPrefKey);
}

}  // namespace image_metadata_stripper::metrics
