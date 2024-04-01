/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/playlist/test/playlist_unittest_base.h"

#include <utility>

#include "brave/browser/playlist/playlist_service_factory.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"

namespace playlist {
PlaylistUnitTestBase::PlaylistUnitTestBase() = default;

PlaylistUnitTestBase::~PlaylistUnitTestBase() = default;

void PlaylistUnitTestBase::SetUp() {
  content::RenderViewHostTestHarness::SetUp();

  RegisterLocalState(local_state_.registry());
  TestingBrowserProcess::GetGlobal()->SetLocalState(&local_state_);
}

void PlaylistUnitTestBase::TearDown() {
  TestingBrowserProcess::GetGlobal()->SetLocalState(nullptr);

  content::RenderViewHostTestHarness::TearDown();
}

std::unique_ptr<content::BrowserContext>
PlaylistUnitTestBase::CreateBrowserContext() {
  auto prefs = std::make_unique<sync_preferences::TestingPrefServiceSyncable>();
  RegisterUserProfilePrefs(prefs->registry());

  // `PlaylistServiceFactory` needs to be present in the dependency graph,
  // so that `PlaylistServiceFactory::RegisterProfilePrefs()` is triggered by
  // `DependencyManager::RegisterPrefsForServices()`
  PlaylistServiceFactory::GetInstance();

  return TestingProfile::Builder().SetPrefService(std::move(prefs)).Build();
}
}  // namespace playlist
