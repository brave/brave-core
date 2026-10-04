/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_MEDIA_SESSION_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_MEDIA_SESSION_METRICS_H_

#include "base/observer_list_types.h"
#include "base/time/time.h"

namespace misc_metrics {

// Observes media session activity to track the percentage of active browsing
// time during which media was playing. Reports metrics on a weekly basis.
class MediaSessionMetrics {
 public:
  class Observer : public base::CheckedObserver {
   public:
    // Called only while media is actively playing, once per tick, with the
    // duration of the tick interval that just elapsed.
    virtual void OnMediaPlaybackTick(base::TimeDelta tick_duration) = 0;
  };

  virtual ~MediaSessionMetrics() = default;

  virtual void AddObserver(Observer* observer) = 0;
  virtual void RemoveObserver(Observer* observer) = 0;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_MEDIA_SESSION_METRICS_H_
