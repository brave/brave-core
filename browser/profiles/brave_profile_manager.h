/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PROFILES_BRAVE_PROFILE_MANAGER_H_
#define BRAVE_BROWSER_PROFILES_BRAVE_PROFILE_MANAGER_H_

#include <string>

#include "chrome/browser/profiles/profile_manager.h"

class BraveProfileManager : public ProfileManager {
 public:
  explicit BraveProfileManager(const base::FilePath& user_data_dir);
  ~BraveProfileManager() override;

  // Returns the full path to be used for tor profiles.
  static base::FilePath GetTorProfilePath();

  static void InitTorProfileUserPrefs(Profile* profile);

  void InitProfileUserPrefs(Profile* profile) override;
  std::string GetLastUsedProfileName() override;
  void SetNonPersonalProfilePrefs(Profile* profile) override;
  bool IsAllowedProfilePath(const base::FilePath& path) const override;
  void AddProfileToStorage(Profile* profile) override;

  void OnProfileCreated(Profile* profile,
                        bool success,
                        bool is_new_profile) override;

  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

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
