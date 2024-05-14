/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PLAYLIST_TEST_PLAYLIST_UNITTEST_BASE_H_
#define BRAVE_BROWSER_PLAYLIST_TEST_PLAYLIST_UNITTEST_BASE_H_

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "brave/components/playlist/common/features.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/test_renderer_host.h"

namespace content {
class BrowserContext;
}

namespace playlist {
class PlaylistUnitTestBase : public content::RenderViewHostTestHarness {
 public:
  PlaylistUnitTestBase();
  ~PlaylistUnitTestBase() override;

 protected:
  // testing::Test:
  void SetUp() override;
  void TearDown() override;

  // content::RenderViewHostTestHarness:
  std::unique_ptr<content::BrowserContext> CreateBrowserContext() override;

  TestingPrefServiceSimple local_state_;
  base::test::ScopedFeatureList feature_list_{features::kPlaylist};
};
}  // namespace playlist

#endif  // BRAVE_BROWSER_PLAYLIST_TEST_PLAYLIST_UNITTEST_BASE_H_
