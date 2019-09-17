/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/profiles/tor_unittest_profile_manager.h"

#include <memory>
#include <utility>

#include "base/files/file_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/tor/tor_profile_service.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/pref_service_mock_factory.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"

Profile* TorUnittestProfileManager::CreateProfileHelper(
    const base::FilePath& path) {
  if (!base::PathExists(path)) {
    if (!base::CreateDirectory(path))
      return nullptr;
  }
  return CreateProfile(path, nullptr).release();
}

std::unique_ptr<Profile> TorUnittestProfileManager::CreateProfileAsyncHelper(
    const base::FilePath& path,
    Delegate* delegate) {
  // ThreadTaskRunnerHandle::Get() is TestingProfile's "async" IOTaskRunner
  // (ref. TestingProfile::GetIOTaskRunner()).
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(base::IgnoreResult(&base::CreateDirectory), path));

  return CreateProfile(path, this);
}

void TorUnittestProfileManager::InitProfileUserPrefs(Profile* profile) {
  if (profile->GetPath() == BraveProfileManager::GetTorProfilePath()) {
    BraveProfileManager::InitTorProfileUserPrefs(profile);
  } else {
    ProfileManager::InitProfileUserPrefs(profile);
  }
}

std::unique_ptr<Profile> TorUnittestProfileManager::CreateProfile(
    const base::FilePath& path, Delegate* delegate) {
  TestingProfile::Builder profile_builder;
  sync_preferences::PrefServiceMockFactory factory;
  auto registry = base::MakeRefCounted<user_prefs::PrefRegistrySyncable>();
  std::unique_ptr<sync_preferences::PrefServiceSyncable> prefs(
      factory.CreateSyncable(registry.get()));
  RegisterUserProfilePrefs(registry.get());
  tor::TorProfileService::RegisterProfilePrefs(registry.get());
  profile_builder.SetPrefService(std::move(prefs));
  profile_builder.SetPath(path);
  profile_builder.SetDelegate(delegate);
  return profile_builder.Build();
}
