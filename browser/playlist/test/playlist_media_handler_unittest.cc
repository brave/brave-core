/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <type_traits>
#include <utility>

#include "base/run_loop.h"
#include "base/test/mock_callback.h"
#include "base/values.h"
#include "brave/browser/playlist/test/playlist_unittest_base.h"
#include "brave/components/playlist/browser/playlist_media_handler.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "brave/components/playlist/common/playlist_render_frame_observer_helper.h"
#include "mojo/public/cpp/bindings/associated_remote.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
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

template <typename>
class PlaylistMediaHandlerTest : public PlaylistUnitTestBase {};

using CallbackTypes = testing::Types<PlaylistMediaHandler::OnceCallback,
                                     PlaylistMediaHandler::RepeatingCallback>;
TYPED_TEST_SUITE(PlaylistMediaHandlerTest, CallbackTypes);

TYPED_TEST(PlaylistMediaHandlerTest, Callbacks) {
  base::RunLoop run_loop;

  testing::InSequence in_sequence;
  base::MockCallback<TypeParam> callback;
  if constexpr (std::is_same_v<TypeParam,
                               PlaylistMediaHandler::RepeatingCallback>) {
    EXPECT_CALL(callback, Run(testing::_, testing::Not(testing::IsEmpty())))
        .Times(1);
  }
  EXPECT_CALL(callback, Run(testing::_, testing::Not(testing::IsEmpty())))
      .Times(1)
      .WillOnce([&run_loop] { run_loop.Quit(); });

  PlaylistMediaHandler::CreateForWebContents(
      content::RenderViewHostTestHarness::web_contents(), callback.Get());
  content::RenderViewHostTestHarness::NavigateAndCommit(
      GURL("https://example.com"));

  mojo::AssociatedRemote<mojom::PlaylistMediaResponder> remote;
  PlaylistMediaHandler::BindMediaResponderReceiver(
      content::RenderViewHostTestHarness::main_rfh(),
      remote.BindNewEndpointAndPassDedicatedReceiver());
  remote->OnMediaDetected(GetPlaylistItems());
  remote->OnMediaDetected(GetPlaylistItems());

  run_loop.Run();
}

}  // namespace playlist
