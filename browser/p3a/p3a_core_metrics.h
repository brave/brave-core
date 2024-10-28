/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_P3A_P3A_CORE_METRICS_H_
#define BRAVE_BROWSER_P3A_P3A_CORE_METRICS_H_

// The class below can be used on desktop only
// because BrowserListObserver is available on desktop only
// Brave.Core.LastTimeIncognitoUsed and
// Brave.Core.TorEverUsed don't work on Android

#include "build/build_config.h"

#if BUILDFLAG(IS_ANDROID)
#error This file should only be included on desktop.
#endif

#include "base/memory/raw_ptr.h"
#include "base/timer/timer.h"
#include "chrome/browser/ui/browser_list_observer.h"

class PrefService;
class PrefRegistrySimple;

namespace brave {

// BraveWindowTracker is under !OS_ANDROID guard because
// BrowserListObserver should only be only on desktop
// Brave.Uptime.BrowserOpenMinutes and Brave.Core.LastTimeIncognitoUsed
// don't work on Android
#if !BUILDFLAG(IS_ANDROID)
// Periodically records P3A stats (extracted from Local State) regarding the
// time when incognito windows were used.
// Used as a leaking singletone.
class BraveWindowTracker : public BrowserListObserver {
 public:
  explicit BraveWindowTracker(PrefService* local_state);
  BraveWindowTracker(const BraveWindowTracker&) = delete;
  BraveWindowTracker& operator=(const BraveWindowTracker&) = delete;
  ~BraveWindowTracker() override;

  static void CreateInstance(PrefService* local_state);

  static void RegisterPrefs(PrefRegistrySimple* registry);

 private:
  // BrowserListObserver:
  void OnBrowserAdded(Browser* browser) override;
  void OnBrowserSetLastActive(Browser* browser) override;

  void UpdateP3AValues() const;

  base::RepeatingTimer timer_;
  raw_ptr<PrefService, DanglingUntriaged> local_state_ = nullptr;
};
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace brave

#endif  // BRAVE_BROWSER_P3A_P3A_CORE_METRICS_H_
