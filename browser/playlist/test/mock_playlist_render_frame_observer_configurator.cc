/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/playlist/test/mock_playlist_render_frame_observer_configurator.h"

#include <utility>

#include "mojo/public/cpp/bindings/pending_associated_receiver.h"

namespace playlist {
MockPlaylistRenderFrameObserverConfigurator::
    MockPlaylistRenderFrameObserverConfigurator() = default;

MockPlaylistRenderFrameObserverConfigurator::
    ~MockPlaylistRenderFrameObserverConfigurator() = default;

void MockPlaylistRenderFrameObserverConfigurator::BindReceiver(
    mojo::ScopedInterfaceEndpointHandle handle) {
  ASSERT_FALSE(receiver_);
  receiver_.Bind(
      mojo::PendingAssociatedReceiver<
          mojom::PlaylistRenderFrameObserverConfigurator>(std::move(handle)));
}
}  // namespace playlist
