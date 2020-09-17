/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PROFILES_BRAVE_UNITTEST_PROFILE_MANAGER_H_
#define BRAVE_BROWSER_PROFILES_BRAVE_UNITTEST_PROFILE_MANAGER_H_

#include <memory>

#include "brave/browser/profiles/brave_profile_manager.h"

class BraveUnittestProfileManager : public BraveProfileManagerWithoutInit {
 public:
  explicit BraveUnittestProfileManager(const base::FilePath& user_data_dir)
      : BraveProfileManagerWithoutInit(user_data_dir) {}
  ~BraveUnittestProfileManager() override = default;

 protected:
  std::unique_ptr<Profile> CreateProfileHelper(
      const base::FilePath& path) override;

  std::unique_ptr<Profile> CreateProfileAsyncHelper(
      const base::FilePath& path,
      Delegate* delegate) override;

  void InitProfileUserPrefs(Profile* profile) override;

 private:
  std::unique_ptr<Profile> CreateProfile(const base::FilePath& path,
                                         Delegate* delegate);
};

#endif  // BRAVE_BROWSER_PROFILES_BRAVE_UNITTEST_PROFILE_MANAGER_H_
