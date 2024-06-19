/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_DAY_ZERO_BROWSER_UI_EXPT_DAY_ZERO_BROWSER_UI_EXPT_MANAGER_H_
#define BRAVE_BROWSER_UI_DAY_ZERO_BROWSER_UI_EXPT_DAY_ZERO_BROWSER_UI_EXPT_MANAGER_H_

#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "base/timer/timer.h"
#include "chrome/browser/profiles/profile_manager_observer.h"

class PrefService;
class Profile;
class ProfileManager;

class DayZeroBrowserUIExptManager : public ProfileManagerObserver {
 public:
  explicit DayZeroBrowserUIExptManager(ProfileManager* profile_manater);
  ~DayZeroBrowserUIExptManager() override;
  DayZeroBrowserUIExptManager(const DayZeroBrowserUIExptManager&) = delete;
  DayZeroBrowserUIExptManager& operator=(const DayZeroBrowserUIExptManager&) =
      delete;

  // ProfileManagerObserver overrides:
  void OnProfileAdded(Profile* profile) override;
  void OnProfileManagerDestroying() override;

 private:
  void SetForDayZeroBrowserUI(Profile* profile);
  void ResetForDayZeroBrowserUI(Profile* profile);
  void ResetBrowserUIStateForAllProfiles();
  bool IsPassedOneDaySinceFirstRun() const;
  void StartResetTimer();

  // When fire, we'll reset browser UI to original.
  base::OneShotTimer reset_timer_;
  raw_ref<ProfileManager> profile_manager_;
  base::ScopedObservation<ProfileManager, ProfileManagerObserver> observation_{
      this};
};

#endif  // BRAVE_BROWSER_UI_DAY_ZERO_BROWSER_UI_EXPT_DAY_ZERO_BROWSER_UI_EXPT_MANAGER_H_
