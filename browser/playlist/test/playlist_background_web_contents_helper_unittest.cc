/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <vector>

#include "base/functional/bind.h"
#include "base/test/task_environment.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/browser/playlist/test/mock_playlist_render_frame_observer_configurator.h"
#include "brave/browser/playlist/test/playlist_unittest_base.h"
#include "brave/components/playlist/browser/playlist_background_web_contents_helper.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
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

  EXPECT_CALL(configurator_,
              AddMediaSourceAPISuppressor(testing::Not(testing::IsEmpty())))
      .Times(1);
  EXPECT_CALL(configurator_, AddMediaDetector(testing::Not(testing::IsEmpty())))
      .Times(1);

  NavigateAndCommit(GURL("https://example.com"));
  task_environment()->RunUntilIdle();
}

}  // namespace playlist
