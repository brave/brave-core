// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_GESTURE_INPUT_REGISTRAR_H_
#define BRAVE_BROWSER_UI_GESTURES_GESTURE_INPUT_REGISTRAR_H_

#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "components/input/input_event_source.h"
#include "content/public/browser/render_widget_host.h"

namespace blink {
class WebInputEvent;
}  // namespace blink

namespace content {
class WebContents;
}  // namespace content

namespace gestures {

// Owns attachment to every widget reachable from a tab's current primary
// page for raw mouse, wheel, and touch input: the main frame's widget, plus
// each out-of-process iframe's own widget (e.g. a sponsored rich-media
// background), which otherwise has its own `RenderWidgetHost` the main
// frame's never sees input from. Re-syncs whenever the frame tree could
// have changed, and detaches everything on destruction. Input is observed,
// never consumed, via `AddMouseEventCallback`, which always returns false,
// and a non-consuming `InputEventObserver` for wheel/touch.
class GestureInputRegistrar final
    : public content::RenderWidgetHost::InputEventObserver {
 public:
  GestureInputRegistrar(
      content::RenderWidgetHost::MouseEventCallback mouse_event_callback,
      base::RepeatingCallback<void(const blink::WebInputEvent&,
                                   input::InputEventSource)> on_input_event);

  GestureInputRegistrar(const GestureInputRegistrar&) = delete;
  GestureInputRegistrar& operator=(const GestureInputRegistrar&) = delete;

  ~GestureInputRegistrar() override;

  // Attaches to every widget reachable from `web_contents`'s current primary
  // page, detaching from any widget no longer reachable. A no-op for widgets
  // whose attachment doesn't change; multiple frames sharing one widget (e.g.
  // same-process subframes) are attached to it only once. Detaches entirely if
  // `web_contents` is null.
  void SyncWithFrameTree(content::WebContents* web_contents);

  // Detaches from `widget` specifically, e.g. from `RenderViewDeleted`, where
  // the widget is about to become invalid right now and can't wait for a
  // subsequent `SyncWithFrameTree` resync to notice. A no-op if not currently
  // attached to it.
  void UnregisterWidget(content::RenderWidgetHost& widget);
  void UnregisterAll();

  // Whether `widget` is one of the ones currently attached to, for callers
  // (e.g. `RenderViewDeleted`) that need to confirm a `RenderWidgetHost` about
  // to become invalid is actually in use, not an unrelated, already-detached
  // one.
  bool IsObservingWidget(content::RenderWidgetHost& widget) const;

 private:
  // content::RenderWidgetHost::InputEventObserver:
  void OnInputEvent(const content::RenderWidgetHost& host,
                    const blink::WebInputEvent& event,
                    InputEventSource source) override;

  // Removes `widget`'s callback/observer registration. Doesn't touch
  // `observed_widgets_`; callers remove their own entry however suits their
  // own loop (`std::erase`, `std::erase_if`, or a full-container iteration).
  void DetachFromWidget(content::RenderWidgetHost& widget);

  content::RenderWidgetHost::MouseEventCallback mouse_event_callback_;
  base::RepeatingCallback<void(const blink::WebInputEvent&,
                               input::InputEventSource)>
      on_input_event_;

  // Not owned. `base::ScopedMultiSourceObservation` would only manage the
  // `AddInputEventObserver`/`RemoveInputEventObserver` half of each widget's
  // registration, not the separate, differently-shaped
  // `AddMouseEventCallback`/`RemoveMouseEventCallback` pair also kept in
  // lock-step with it here, nor the diff-against-the-previous-set enumeration
  // `SyncWithFrameTree` needs; tracked manually instead so both registrations
  // and the diff share one source of truth.
  std::vector<raw_ptr<content::RenderWidgetHost>> observed_widgets_;
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_GESTURE_INPUT_REGISTRAR_H_
