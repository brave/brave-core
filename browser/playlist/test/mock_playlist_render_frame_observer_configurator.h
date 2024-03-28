/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PLAYLIST_TEST_MOCK_PLAYLIST_RENDER_FRAME_OBSERVER_CONFIGURATOR_H_
#define BRAVE_BROWSER_PLAYLIST_TEST_MOCK_PLAYLIST_RENDER_FRAME_OBSERVER_CONFIGURATOR_H_

#include <string>

#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "mojo/public/cpp/bindings/associated_receiver.h"
#include "mojo/public/cpp/bindings/scoped_interface_endpoint_handle.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace playlist {
class MockPlaylistRenderFrameObserverConfigurator
    : public mojom::PlaylistRenderFrameObserverConfigurator {
 public:
  MockPlaylistRenderFrameObserverConfigurator();
  ~MockPlaylistRenderFrameObserverConfigurator() override;

  MOCK_METHOD(void,
              AddMediaSourceAPISuppressor,
              (const std::string&),
              (override));

  MOCK_METHOD(void, AddMediaDetector, (const std::string&), (override));

  void BindReceiver(mojo::ScopedInterfaceEndpointHandle handle);

 private:
  mojo::AssociatedReceiver<mojom::PlaylistRenderFrameObserverConfigurator>
      receiver_{this};
};
}  // namespace playlist

#endif  // BRAVE_BROWSER_PLAYLIST_TEST_MOCK_PLAYLIST_RENDER_FRAME_OBSERVER_CONFIGURATOR_H_
