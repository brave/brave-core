// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/input/gesture_input_registrar_util.h"

#include <algorithm>

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"

namespace gestures {

std::vector<content::RenderWidgetHost*> CollectRenderWidgetHosts(
    content::WebContents* const web_contents) {
  std::vector<content::RenderWidgetHost*> render_widget_hosts;
  if (!web_contents || !web_contents->GetPrimaryMainFrame()) {
    return render_widget_hosts;
  }
  // Scoped to the primary main frame's own `ForEachRenderFrameHost`, rather
  // than `WebContents::ForEachRenderFrameHost`, since the latter also
  // includes back-forward-cached and prerendered pages, which aren't
  // visible or interactable right now.
  web_contents->GetPrimaryMainFrame()->ForEachRenderFrameHost(
      [&render_widget_hosts](content::RenderFrameHost* render_frame_host) {
        content::RenderWidgetHost* const render_widget_host =
            render_frame_host->GetRenderWidgetHost();
        // `GetRenderWidgetHost` resolves to the nearest local root's
        // widget, so same-process subframes naturally dedupe to their
        // shared ancestor's widget here.
        if (render_widget_host &&
            !std::ranges::contains(render_widget_hosts, render_widget_host)) {
          render_widget_hosts.push_back(render_widget_host);
        }
      });
  return render_widget_hosts;
}

}  // namespace gestures
