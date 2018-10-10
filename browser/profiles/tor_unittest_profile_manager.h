/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PROFILES_TOR_UNITTEST_PROFILE_MANAGER_H_
#define BRAVE_BROWSER_PROFILES_TOR_UNITTEST_PROFILE_MANAGER_H_

#include "chrome/browser/profiles/profile_manager.h"

class TorUnittestProfileManager : public ProfileManagerWithoutInit {
 public:
  explicit TorUnittestProfileManager(const base::FilePath& user_data_dir)
      : ProfileManagerWithoutInit(user_data_dir) {}
  ~TorUnittestProfileManager() override = default;

 protected:
  Profile* CreateProfileHelper(const base::FilePath& path) override;

  Profile* CreateProfileAsyncHelper(const base::FilePath& path,
                                    Delegate* delegate) override;

  void InitProfileUserPrefs(Profile* profile) override;

 private:
  Profile* CreateTorProfile(const base::FilePath& path, Delegate* delegate);
};

#endif  // BRAVE_BROWSER_PROFILES_TOR_UNITTEST_PROFILE_MANAGER_H_
