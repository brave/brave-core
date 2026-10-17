// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_input_registrar.h"

#include <algorithm>
#include <utility>

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace gestures {

GestureInputRegistrar::GestureInputRegistrar(
    content::RenderWidgetHost::MouseEventCallback mouse_event_callback,
    base::RepeatingCallback<void(const blink::WebInputEvent&,
                                 input::InputEventSource)> on_input_event)
    : mouse_event_callback_(std::move(mouse_event_callback)),
      on_input_event_(std::move(on_input_event)) {}

GestureInputRegistrar::~GestureInputRegistrar() {
  UnregisterAll();
}

void GestureInputRegistrar::SyncWithFrameTree(
    content::WebContents* const web_contents) {
  std::vector<content::RenderWidgetHost*> current_widgets;
  if (web_contents && web_contents->GetPrimaryMainFrame()) {
    // Scoped to the primary main frame's own `ForEachRenderFrameHost`,
    // rather than `WebContents::ForEachRenderFrameHost`, since the latter
    // also includes back-forward-cached and prerendered pages, which aren't
    // visible or interactable right now.
    web_contents->GetPrimaryMainFrame()->ForEachRenderFrameHost(
        [&current_widgets](content::RenderFrameHost* render_frame_host) {
          content::RenderWidgetHost* const widget =
              render_frame_host->GetRenderWidgetHost();
          // `GetRenderWidgetHost` resolves to the nearest local root's
          // widget, so same-process subframes naturally dedupe to their
          // shared ancestor's widget here.
          if (widget && !std::ranges::contains(current_widgets, widget)) {
            current_widgets.emplace_back(widget);
          }
        });
  }

  std::erase_if(observed_widgets_, [&](content::RenderWidgetHost* widget) {
    if (std::ranges::contains(current_widgets, widget)) {
      return false;
    }
    DetachFromWidget(*widget);
    return true;
  });

  for (content::RenderWidgetHost* const widget : current_widgets) {
    if (std::ranges::contains(observed_widgets_, widget)) {
      continue;
    }
    observed_widgets_.emplace_back(widget);
    widget->AddMouseEventCallback(mouse_event_callback_);
    widget->AddInputEventObserver(this);
  }
}

void GestureInputRegistrar::UnregisterWidget(
    content::RenderWidgetHost& widget) {
  if (!std::ranges::contains(observed_widgets_, &widget)) {
    return;
  }
  DetachFromWidget(widget);
  std::erase(observed_widgets_, &widget);
}

void GestureInputRegistrar::UnregisterAll() {
  for (content::RenderWidgetHost* const widget : observed_widgets_) {
    DetachFromWidget(*widget);
  }
  observed_widgets_.clear();
}

void GestureInputRegistrar::DetachFromWidget(
    content::RenderWidgetHost& widget) {
  widget.RemoveMouseEventCallback(mouse_event_callback_);
  widget.RemoveInputEventObserver(this);
}

bool GestureInputRegistrar::IsObservingWidget(
    content::RenderWidgetHost& widget) const {
  return std::ranges::contains(observed_widgets_, &widget);
}

void GestureInputRegistrar::OnInputEvent(
    const content::RenderWidgetHost& /*host*/,
    const blink::WebInputEvent& event,
    InputEventSource source) {
  on_input_event_.Run(event, source);
}

}  // namespace gestures
