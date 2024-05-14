/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/containers/contains.h"
#include "base/functional/callback_helpers.h"
#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_feature_list.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/browser/playlist/test/playlist_unittest_base.h"
#include "brave/components/playlist/browser/playlist_background_web_contentses.h"
#include "brave/components/playlist/browser/playlist_media_handler.h"
#include "brave/components/playlist/common/features.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "brave/components/playlist/common/playlist_render_frame_observer_helper.h"
#include "content/public/browser/web_contents.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"
#include "url/gurl.h"

namespace playlist {
namespace {
auto GetPlaylistItems() {
  return ExtractPlaylistItems(
      GURL(), base::Value::List().Append(
                  base::Value::Dict()
                      .Set("name", "")
                      .Set("pageTitle", "")
                      .Set("pageSrc", "")
                      .Set("mimeType", "")
                      .Set("src", "https://example.com/video.mp4")
                      .Set("srcIsMediaSourceObjectURL", false)));
}
}  // namespace

class PlaylistBackgroundWebContentsTest
    : public PlaylistUnitTestBase,
      public testing::WithParamInterface<bool> {};

TEST_P(PlaylistBackgroundWebContentsTest, ExtractPlaylistItemsInTheBackground) {
  const bool timeout = GetParam();

  base::RunLoop run_loop;
  base::MockCallback<PlaylistMediaHandler::OnceCallback> callback;
  EXPECT_CALL(callback, Run(testing::_, testing::Conditional(
                                            timeout, testing::IsEmpty(),
                                            testing::Not(testing::IsEmpty()))))
      .Times(1)
      .WillOnce([&run_loop] { run_loop.Quit(); });

  PlaylistBackgroundWebContentses background_web_contentses(
      browser_context(),
      PlaylistServiceFactory::GetForBrowserContext(browser_context()));
  background_web_contentses.Add(GURL("https://example.com"), callback.Get(),
                                base::Seconds(3));

  if (!timeout) {
    mojo::AssociatedRemote<mojom::PlaylistMediaResponder> remote;
    PlaylistMediaHandler::BindMediaResponderReceiver(
        background_web_contentses.web_contents().GetPrimaryMainFrame(),
        remote.BindNewEndpointAndPassDedicatedReceiver());
    remote->OnMediaDetected(GetPlaylistItems());
  }

  run_loop.Run();
}

INSTANTIATE_TEST_SUITE_P(Playlist,
                         PlaylistBackgroundWebContentsTest,
                         testing::Bool());

TEST_F(PlaylistBackgroundWebContentsTest, UserAgentOverride) {
  base::test::ScopedFeatureList scoped_feature_list(features::kPlaylistFakeUA);

  PlaylistBackgroundWebContentses background_web_contentses(
      browser_context(),
      PlaylistServiceFactory::GetForBrowserContext(browser_context()));
  background_web_contentses.Add(GURL("https://example.com"), base::DoNothing());

  const auto ua_string_override = background_web_contentses.web_contents()
                                      .GetUserAgentOverride()
                                      .ua_string_override;
  EXPECT_TRUE(base::Contains(ua_string_override, "iPhone"));
  EXPECT_FALSE(base::Contains(ua_string_override, "Chrome"));
}

}  // namespace playlist
