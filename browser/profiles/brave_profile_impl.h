/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PROFILES_BRAVE_PROFILE_IMPL_H_
#define BRAVE_BROWSER_PROFILES_BRAVE_PROFILE_IMPL_H_

#include "chrome/browser/profiles/profile_impl.h"

class BraveProfileImpl : public ProfileImpl {
 public:
  BraveProfileImpl(const base::FilePath& path,
      Delegate* delegate,
      CreateMode create_mode,
      scoped_refptr<base::SequencedTaskRunner> io_task_runner);

  ~BraveProfileImpl() override;
  // Profile override
  Profile* GetTorProfile() override;
  void DestroyTorProfile() override;
  bool HasTorProfile() override;
  bool IsSameProfile(Profile* profile) override;

 private:
  Profile* CreateTorProfile();

  std::unique_ptr<Profile> tor_profile_;

  DISALLOW_COPY_AND_ASSIGN(BraveProfileImpl);
};

#endif // BRAVE_BROWSER_PROFILES_BRAVE_PROFILE_IMPL_H_
