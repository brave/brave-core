/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_UNITTEST_BASE_H_
#define BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_UNITTEST_BASE_H_

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "brave/components/playlist/core/common/features.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/test_renderer_host.h"

namespace content {
class BrowserContext;
}

namespace playlist {

class MediaDetectorComponentManager;
class PlaylistService;

// Owns a `PlaylistService` built directly (no keyed-service factory) on top of
// a `content::TestBrowserContext` whose user prefs are a
// `TestingPrefServiceSyncable`.
class PlaylistUnitTestBase : public content::RenderViewHostTestHarness {
 public:
  PlaylistUnitTestBase();
  ~PlaylistUnitTestBase() override;

 protected:
  PlaylistService* playlist_service() { return service_.get(); }

  // testing::Test:
  void SetUp() override;
  void TearDown() override;

  // content::RenderViewHostTestHarness:
  std::unique_ptr<content::BrowserContext> CreateBrowserContext() override;

  base::test::ScopedFeatureList feature_list_{features::kPlaylist};

 private:
  TestingPrefServiceSimple local_state_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<MediaDetectorComponentManager> detector_manager_;
  std::unique_ptr<PlaylistService> service_;
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_CONTENT_BROWSER_PLAYLIST_UNITTEST_BASE_H_
