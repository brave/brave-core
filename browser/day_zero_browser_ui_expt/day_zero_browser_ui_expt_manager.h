/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_DAY_ZERO_BROWSER_UI_EXPT_DAY_ZERO_BROWSER_UI_EXPT_MANAGER_H_
#define BRAVE_BROWSER_DAY_ZERO_BROWSER_UI_EXPT_DAY_ZERO_BROWSER_UI_EXPT_MANAGER_H_

#include <memory>
#include <optional>

#include "base/memory/raw_ref.h"
#include "base/scoped_observation.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "chrome/browser/profiles/profile_manager_observer.h"

class Profile;
class ProfileManager;

class DayZeroBrowserUIExptManager : public ProfileManagerObserver {
 public:
  static std::unique_ptr<DayZeroBrowserUIExptManager> Create(
      ProfileManager* profile_manager);

  ~DayZeroBrowserUIExptManager() override;
  DayZeroBrowserUIExptManager(const DayZeroBrowserUIExptManager&) = delete;
  DayZeroBrowserUIExptManager& operator=(const DayZeroBrowserUIExptManager&) =
      delete;

  // ProfileManagerObserver overrides:
  void OnProfileAdded(Profile* profile) override;
  void OnProfileManagerDestroying() override;

 private:
  friend class DayZeroBrowserUIExptTest;

  // |mock_first_run_time| only for testing.
  DayZeroBrowserUIExptManager(
      ProfileManager* profile_manager,
      std::optional<base::Time> mock_first_run_time = std::nullopt);

  void SetForDayZeroBrowserUI(Profile* profile);
  void ResetForDayZeroBrowserUI(Profile* profile);
  void ResetBrowserUIStateForAllProfiles();
  void StartResetTimer();
  base::Time GetFirstRunTime() const;

  // When fire, we'll reset browser UI to original.
  base::OneShotTimer reset_timer_;
  raw_ref<ProfileManager> profile_manager_;
  std::optional<base::Time> first_run_time_for_testing_;
  base::ScopedObservation<ProfileManager, ProfileManagerObserver> observation_{
      this};
};

#endif  // BRAVE_BROWSER_DAY_ZERO_BROWSER_UI_EXPT_DAY_ZERO_BROWSER_UI_EXPT_MANAGER_H_
