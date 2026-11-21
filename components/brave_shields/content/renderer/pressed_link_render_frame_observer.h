// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_RENDERER_PRESSED_LINK_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_RENDERER_PRESSED_LINK_RENDER_FRAME_OBSERVER_H_

#include <cstdint>
#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_shields/core/common/brave_shields.mojom.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

namespace brave_shields {

// Tells the browser which link the user pressed, so that a window opened for
// it isn't blocked as a popup.
class PressedLinkRenderFrameObserver
    : public content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<
          PressedLinkRenderFrameObserver> {
 public:
  PressedLinkRenderFrameObserver(content::RenderFrame* render_frame,
                                 int32_t isolated_world_id);
  PressedLinkRenderFrameObserver(const PressedLinkRenderFrameObserver&) =
      delete;
  PressedLinkRenderFrameObserver& operator=(
      const PressedLinkRenderFrameObserver&) = delete;

  void RunScriptsAtDocumentStart();

 private:
  ~PressedLinkRenderFrameObserver() override;

  // content::RenderFrameObserver:
  void OnDestruct() override;

  void OnLinkPressed(const std::string& url);

  const int32_t isolated_world_id_;
  // Must be associated, so that OnLinkPressed() reaches the browser before the
  // CreateNewWindow and BeginNavigation of the press that caused it.
  mojo::AssociatedRemote<mojom::BraveShieldsHost> brave_shields_host_;
  base::WeakPtrFactory<PressedLinkRenderFrameObserver> weak_ptr_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_RENDERER_PRESSED_LINK_RENDER_FRAME_OBSERVER_H_
