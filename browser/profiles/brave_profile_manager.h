/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PROFILES_BRAVE_PROFILE_MANAGER_H_
#define BRAVE_BROWSER_PROFILES_BRAVE_PROFILE_MANAGER_H_

#include <string>

#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_manager_observer.h"

class BraveProfileManager : public ProfileManager,
                            public ProfileManagerObserver {
 public:
  explicit BraveProfileManager(const base::FilePath& user_data_dir);
  ~BraveProfileManager() override;

  void InitProfileUserPrefs(Profile* profile) override;
  std::string GetLastUsedProfileName() override;
  void SetNonPersonalProfilePrefs(Profile* profile) override;
  bool IsAllowedProfilePath(const base::FilePath& path) const override;
  bool LoadProfileByPath(const base::FilePath& profile_path,
                         bool incognito,
                         ProfileLoadedCallback callback) override;

  // ProfileManagerObserver:
  void OnProfileAdded(Profile* profile) override;

 protected:
  void DoFinalInitForServices(Profile* profile,
                              bool go_off_the_record) override;

 private:
  void MigrateProfileNames();

  DISALLOW_COPY_AND_ASSIGN(BraveProfileManager);
};

class BraveProfileManagerWithoutInit : public BraveProfileManager {
 public:
  explicit BraveProfileManagerWithoutInit(const base::FilePath& user_data_dir);

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveProfileManagerWithoutInit);
};

#endif  // BRAVE_BROWSER_PROFILES_BRAVE_PROFILE_MANAGER_H_
