/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_unittest_base.h"

#include <utility>

#include "base/time/time.h"
#include "brave/components/playlist/content/browser/media_detector_component_manager.h"
#include "brave/components/playlist/content/browser/playlist_p3a.h"
#include "brave/components/playlist/content/browser/playlist_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/test/test_browser_context.h"

namespace playlist {

PlaylistUnitTestBase::PlaylistUnitTestBase() = default;

PlaylistUnitTestBase::~PlaylistUnitTestBase() = default;

void PlaylistUnitTestBase::SetUp() {
  content::RenderViewHostTestHarness::SetUp();

  PlaylistP3A::RegisterLocalStatePrefs(local_state_.registry());

  detector_manager_ = std::make_unique<MediaDetectorComponentManager>(nullptr);
  service_ = std::make_unique<PlaylistService>(
      browser_context(), &local_state_, detector_manager_.get(),
      /*delegate=*/nullptr, base::Time::Now());
}

void PlaylistUnitTestBase::TearDown() {
  // WebContents helpers reference the service, which uses the browser context.
  DeleteContents();
  service_.reset();
  detector_manager_.reset();
  content::RenderViewHostTestHarness::TearDown();
}

std::unique_ptr<content::BrowserContext>
PlaylistUnitTestBase::CreateBrowserContext() {
  PlaylistService::RegisterProfilePrefs(prefs_.registry());
  auto context = std::make_unique<content::TestBrowserContext>();
  user_prefs::UserPrefs::Set(context.get(), &prefs_);
  return context;
}

}  // namespace playlist
