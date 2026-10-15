/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/image_metadata_stripper/metrics_reporter.h"

#include <array>

#include "base/check.h"
#include "base/location.h"
#include "base/notreached.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/p3a_utils/bucket.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace image_metadata_stripper::metrics {

namespace {

constexpr base::TimeDelta kReportInterval = base::Days(1);

// 1-5, 6-10, 11-20, 21-30, 31+
constexpr int kCountBuckets[] = {5, 10, 20, 30};

// Maps a count stored in local state to the histogram it is reported under.
struct HistogramDetails {
  const char* histogram_name;
  const char* download_or_upload_key;
  const char* metadata_key;
  const char* count_key;
};

constexpr auto kHistogramsToReport = std::to_array<HistogramDetails>({
    {
        kDownloadFBMDCountStrippedHistogramName,
        kDownloadPrefKey,
        kFBMDMetadataPrefKey,
        kStrippedPrefKey,
    },
    {
        kUploadFBMDCountStrippedHistogramName,
        kUploadPrefKey,
        kFBMDMetadataPrefKey,
        kStrippedPrefKey,
    },
});

const char* GetFlowKey(const StrippingClient client) {
  switch (client) {
    case StrippingClient::kDownloadManager:
      return kDownloadPrefKey;
    case StrippingClient::kFileSelect:
    case StrippingClient::kDragDrop:
      return kUploadPrefKey;
  }
  NOTREACHED();
}

}  // namespace

MetricsReporter::MetricsReporter(PrefService* local_state)
    : local_state_(local_state) {
  CHECK(local_state_);
  MaybeReport();
}

MetricsReporter::~MetricsReporter() = default;

// static
void MetricsReporter::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterDictionaryPref(kRootPrefKey, {});
  registry->RegisterTimePref(kLastRecordedTimePref, {});
}

void MetricsReporter::MaybeRecordFbmd(StrippingClient client,
                                      bool was_stripped) {
  ScopedDictPrefUpdate update(local_state_, kRootPrefKey);
  base::DictValue* metadata =
      update->EnsureDict(GetFlowKey(client))->EnsureDict(kFBMDMetadataPrefKey);

  if (was_stripped) {
    metadata->Set(kStrippedPrefKey,
                  metadata->FindInt(kStrippedPrefKey).value_or(0) + 1);
  }
}

void MetricsReporter::MaybeReport() {
  base::Time now = base::Time::Now();
  base::Time last_recorded_time = local_state_->GetTime(kLastRecordedTimePref);

  // The browser process was restarted within the reporting window, so only
  // re-schedule the timer for the remainder of that window.
  if (!last_recorded_time.is_null() &&
      now - last_recorded_time < kReportInterval) {
    report_timer_.Start(FROM_HERE, last_recorded_time + kReportInterval, this,
                        &MetricsReporter::MaybeReport);
    return;
  }

  // On the first ever run there is no previous window to report on.
  if (!last_recorded_time.is_null()) {
    const base::DictValue& root = local_state_->GetDict(kRootPrefKey);
    for (const auto& details : kHistogramsToReport) {
      const base::DictValue* flow =
          root.FindDict(details.download_or_upload_key);
      if (!flow) {
        continue;
      }
      const base::DictValue* metadata = flow->FindDict(details.metadata_key);
      if (!metadata) {
        continue;
      }
      const int count = metadata->FindInt(details.count_key).value_or(0);
      if (count > 0) {
        p3a_utils::RecordToHistogramBucket(details.histogram_name,
                                           kCountBuckets, count);
      }
    }

    local_state_->ClearPref(kRootPrefKey);
  }

  local_state_->SetTime(kLastRecordedTimePref, now);
  report_timer_.Start(FROM_HERE, now + kReportInterval, this,
                      &MetricsReporter::MaybeReport);
}

}  // namespace image_metadata_stripper::metrics
