// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_GESTURES_INPUT_GESTURE_INPUT_REGISTRAR_H_
#define BRAVE_BROWSER_UI_GESTURES_INPUT_GESTURE_INPUT_REGISTRAR_H_

#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "components/input/input_event_source.h"
#include "content/public/browser/render_widget_host.h"

namespace blink {
class WebInputEvent;
class WebMouseEvent;
}  // namespace blink

namespace content {
class WebContents;
}  // namespace content

namespace gestures {

// Owns attachment to every widget reachable from a tab, including each
// out-of-process iframe's, for raw mouse/wheel/touch input (observed only).
// Mouse events are transformed into the page's root coordinate space before
// reaching `mouse_event_callback_`, so a gesture drawn over an out-of-process
// subframe (e.g. a PDF viewer's own embedded widget) lines up with the
// cursor instead of being offset by that widget's position within the page.
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
  // page, detaching from any no longer reachable. Detaches entirely if null.
  void UpdateObservedWidgets(content::WebContents* web_contents);

  // Detaches from `render_widget_host` specifically, e.g. from
  // `RenderViewDeleted`, which can't wait for a later resync.
  void UnregisterWidget(content::RenderWidgetHost& render_widget_host);
  void UnregisterAllWidgets();

  // Whether `render_widget_host` is currently attached to, e.g. so
  // `RenderViewDeleted` can confirm it isn't an already-detached widget.
  bool IsObservingWidget(content::RenderWidgetHost& render_widget_host) const;

 private:
  // content::RenderWidgetHost::InputEventObserver:
  void OnInputEvent(const content::RenderWidgetHost& host,
                    const blink::WebInputEvent& event,
                    InputEventSource source) override;

  // Bound per-widget with `render_widget_host`, so the transform below knows
  // which widget's coordinate space `event` is actually in. Registered with
  // `render_widget_host` in place of `mouse_event_callback_` directly.
  bool WidgetMouseEventCallback(content::RenderWidgetHost* render_widget_host,
                                const blink::WebMouseEvent& event);

  // Removes `render_widget_host`'s callback/observer registration. Doesn't
  // touch `observed_render_widget_hosts_`, callers remove their own entry.
  void DetachFromWidget(content::RenderWidgetHost& render_widget_host);

  content::RenderWidgetHost::MouseEventCallback mouse_event_callback_;
  base::RepeatingCallback<void(const blink::WebInputEvent&,
                               input::InputEventSource)>
      on_input_event_;

  // Tracked manually, rather than via `base::ScopedMultiSourceObservation`,
  // since that would only manage half of each registration pair. Each
  // widget's own `WidgetMouseEventCallback` binding is kept alongside it, the
  // exact callback instance that must be passed back to
  // `RemoveMouseEventCallback` to detach it.
  std::vector<std::pair<raw_ptr<content::RenderWidgetHost>,
                        content::RenderWidgetHost::MouseEventCallback>>
      observed_render_widget_hosts_;
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_INPUT_GESTURE_INPUT_REGISTRAR_H_
