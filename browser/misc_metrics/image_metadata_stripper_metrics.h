/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */
#ifndef BRAVE_BROWSER_MISC_METRICS_IMAGE_METADATA_STRIPPER_METRICS_H_
#define BRAVE_BROWSER_MISC_METRICS_IMAGE_METADATA_STRIPPER_METRICS_H_

#include "base/memory/raw_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "brave/components/image_metadata_stripper/image_metadata_stripper.h"

class PrefRegistrySimple;
class PrefService;

namespace image_metadata_stripper {

namespace metrics {
// This helps in respecting the 7 day boundary from the last time the
// metrics was recorded. Metrics are recorded from inside the browser
// process which keeps track of a weekly timer in-memory. So, if the
// process dies and restarted again within the week mark, we wouldn't want to
// emit the histogram again. So, keeping this last track timer helps to keep
// that window tight across process restarts. See
// image_metadata_stripper_metrics.cc for implementation details.
inline constexpr char kLastRecordedTimePref[] =
    "brave.image_metadata_stripper.last_recorded_time";

// This is the main pref key mapping to a dictionary in the local state which
// will keep track of all actual the counts.
// "brave.image_metadata_stripper.metric_counts" : {
//   "fbmd": {
//     "download_stripped": value,
//     "upload_stripped": value,
//   },
// }
inline constexpr char kRootPrefKey[] =
    "brave.image_metadata_stripper.metric_counts";

// Nested under |kRootPrefKey| for FBMD stripping counts.
inline constexpr char kFbmdPrefKey[] = "fbmd";
inline constexpr char kDownloadStrippedPrefKey[] = "download_stripped";
inline constexpr char kUploadStrippedPrefKey[] = "upload_stripped";

// Histogram goes here. Keep them in sync with components/p3a/metric_names.h.
inline constexpr char kDownloadFBMDCountStrippedHistogramName[] =
    "Brave.Shields.DownloadFBMDStripped";
inline constexpr char kUploadFBMDCountStrippedHistogramName[] =
    "Brave.Shields.UploadFBMDStripped";

// Collects the FBMD metadata stripping counts and reports them to P3A once a
// week. Owned by the browser (e.g. ProcessMiscMetrics) and created on the UI
// thread.
class ImageMetadataStripperMetrics {
 public:
  // Schedules the first P3A report. Does not emit on a first-ever registration.
  explicit ImageMetadataStripperMetrics(PrefService* local_state);
  ~ImageMetadataStripperMetrics();

  ImageMetadataStripperMetrics(const ImageMetadataStripperMetrics&) = delete;
  ImageMetadataStripperMetrics& operator=(const ImageMetadataStripperMetrics&) =
      delete;

  // Registers the various prefs to store the histogram count.
  static void RegisterPrefs(PrefRegistrySimple* registry);

  // Records that an image carrying the FBMD marker was seen. |client| selects
  // the download or the upload flow. Must be called on the sequence that owns
  // this reporter (the UI thread).
  void RecordFbmd(StrippingClient client);

 private:
  // Reports histograms iff their corresponding count recorded in the
  // last week was non zero. This also schedules the next report.
  void MaybeReport();

  // The timer to help schedule the next reporting.
  base::WallClockTimer report_timer_;
  raw_ptr<PrefService> local_state_;
};

}  // namespace metrics

}  // namespace image_metadata_stripper

#endif  // BRAVE_BROWSER_MISC_METRICS_IMAGE_METADATA_STRIPPER_METRICS_H_
