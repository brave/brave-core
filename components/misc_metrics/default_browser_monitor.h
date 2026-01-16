/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_DEFAULT_BROWSER_MONITOR_H_
#define BRAVE_COMPONENTS_MISC_METRICS_DEFAULT_BROWSER_MONITOR_H_

#include <memory>
#include <optional>

#include "base/observer_list.h"
#include "build/build_config.h"

#if !BUILDFLAG(IS_ANDROID)
#include "base/memory/weak_ptr.h"
#include "base/timer/wall_clock_timer.h"
#endif

namespace misc_metrics {

inline constexpr char kDefaultBrowserHistogramName[] = "Brave.Core.IsDefault";
inline constexpr char kDefaultBrowserDailyHistogramName[] =
    "Brave.Core.IsDefaultDaily";

// Periodically checks if the browser is the default browser and reports the
// relevant metrics via P3A.
class DefaultBrowserMonitor {
 public:
  class Observer : public base::CheckedObserver {
   public:
    // Called when the default browser status changes.
    virtual void OnDefaultBrowserStatusChanged() = 0;
  };

#if !BUILDFLAG(IS_ANDROID)
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual bool IsDefaultBrowser() = 0;
    virtual bool IsFirstRun() = 0;
  };

  explicit DefaultBrowserMonitor(std::unique_ptr<Delegate> delegate);
#else
  DefaultBrowserMonitor();
#endif
  ~DefaultBrowserMonitor();

  DefaultBrowserMonitor(const DefaultBrowserMonitor&) = delete;
  DefaultBrowserMonitor& operator=(const DefaultBrowserMonitor&) = delete;

#if !BUILDFLAG(IS_ANDROID)
  void Start();
#endif

  // Updates the default browser status, reports metrics, and notifies observers
  // if the status changed. On desktop, this is called automatically by the
  // monitor. On Android, this should be called when the status is determined.
  void OnDefaultBrowserStateReceived(bool is_default);

  // Returns the cached default browser status. Returns nullopt if status
  // hasn't been determined yet.
  std::optional<bool> GetCachedDefaultStatus() const;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

 private:
#if !BUILDFLAG(IS_ANDROID)
  void CheckDefaultBrowserState();

  std::unique_ptr<Delegate> delegate_;
  base::WallClockTimer timer_;
#endif

  std::optional<bool> cached_default_status_;
  base::ObserverList<Observer> observers_;

#if !BUILDFLAG(IS_ANDROID)
  base::WeakPtrFactory<DefaultBrowserMonitor> weak_factory_{this};
#endif
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_DEFAULT_BROWSER_MONITOR_H_
