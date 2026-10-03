/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_background_web_contents_helper.h"

#include <vector>

#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/browser/playlist/test/mock_playlist_render_frame_observer_configurator.h"
#include "brave/browser/playlist/test/playlist_unittest_base.h"
#include "brave/components/playlist/core/common/features.h"
#include "brave/components/playlist/core/common/mojom/playlist.mojom.h"
#include "content/public/browser/render_frame_host.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "url/gurl.h"

namespace playlist {
class PlaylistBackgroundWebContentsHelperTest : public PlaylistUnitTestBase {
 protected:
  MockPlaylistRenderFrameObserverConfigurator configurator_;
};

TEST_F(PlaylistBackgroundWebContentsHelperTest,
       EnsureFrameObserverIsInitialized) {
  main_rfh()->GetRemoteAssociatedInterfaces()->OverrideBinderForTesting(
      mojom::PlaylistRenderFrameObserverConfigurator::Name_,
      base::BindRepeating(
          &MockPlaylistRenderFrameObserverConfigurator::BindReceiver,
          base::Unretained(&configurator_)));

  PlaylistBackgroundWebContentsHelper::CreateForWebContents(
      web_contents(),
      PlaylistServiceFactory::GetForBrowserContext(browser_context()),
      base::BindOnce([](GURL, std::vector<mojom::PlaylistItemPtr>) {}));

  base::RunLoop run_loop;
  EXPECT_CALL(configurator_,
              AddMediaSourceAPISuppressor(testing::Not(testing::IsEmpty())))
      .Times(1);
  EXPECT_CALL(configurator_, AddMediaDetector(testing::Not(testing::IsEmpty())))
      .WillOnce([&run_loop](const std::string&) { run_loop.Quit(); });

  NavigateAndCommit(GURL("https://example.com"));
  run_loop.Run();
}

TEST_F(PlaylistBackgroundWebContentsHelperTest,
       UsesLegacyScriptsForYouTubeWithV2Enabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(features::kPlaylistServiceV2);

  main_rfh()->GetRemoteAssociatedInterfaces()->OverrideBinderForTesting(
      mojom::PlaylistRenderFrameObserverConfigurator::Name_,
      base::BindRepeating(
          &MockPlaylistRenderFrameObserverConfigurator::BindReceiver,
          base::Unretained(&configurator_)));

  PlaylistBackgroundWebContentsHelper::CreateForWebContents(
      web_contents(),
      PlaylistServiceFactory::GetForBrowserContext(browser_context()),
      base::BindOnce([](GURL, std::vector<mojom::PlaylistItemPtr>) {}));

  base::RunLoop run_loop;
  EXPECT_CALL(configurator_,
              AddMediaSourceAPISuppressor(testing::Not(testing::IsEmpty())))
      .Times(1);
  EXPECT_CALL(configurator_, AddMediaDetector(testing::Not(testing::IsEmpty())))
      .WillOnce([&run_loop](const std::string&) { run_loop.Quit(); });
  EXPECT_CALL(configurator_, ClearMediaScripts()).Times(0);

  NavigateAndCommit(GURL("https://www.youtube.com/watch?v=test"));
  run_loop.Run();
}

TEST_F(PlaylistBackgroundWebContentsHelperTest,
       ClearsLegacyScriptsForNonYouTubeWithV2Enabled) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(features::kPlaylistServiceV2);

  main_rfh()->GetRemoteAssociatedInterfaces()->OverrideBinderForTesting(
      mojom::PlaylistRenderFrameObserverConfigurator::Name_,
      base::BindRepeating(
          &MockPlaylistRenderFrameObserverConfigurator::BindReceiver,
          base::Unretained(&configurator_)));

  PlaylistBackgroundWebContentsHelper::CreateForWebContents(
      web_contents(),
      PlaylistServiceFactory::GetForBrowserContext(browser_context()),
      base::BindOnce([](GURL, std::vector<mojom::PlaylistItemPtr>) {}));

  base::RunLoop run_loop;
  EXPECT_CALL(configurator_, ClearMediaScripts()).WillOnce([&run_loop] {
    run_loop.Quit();
  });
  EXPECT_CALL(configurator_, AddMediaSourceAPISuppressor(testing::_)).Times(0);
  EXPECT_CALL(configurator_, AddMediaDetector(testing::_)).Times(0);

  NavigateAndCommit(GURL("https://example.com"));
  run_loop.Run();
}

}  // namespace playlist
