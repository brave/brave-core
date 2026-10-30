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
#include "content/public/browser/render_widget_host_observer.h"

namespace blink {
class WebInputEvent;
class WebMouseEvent;
}  // namespace blink

namespace content {
class WebContents;
}  // namespace content

namespace gestures {

// Owns attachment to every widget reachable from a tab (incl. iframes);
// transforms mouse events into the page's root coordinate space first.
class GestureInputRegistrar final
    : public content::RenderWidgetHost::InputEventObserver,
      public content::RenderWidgetHostObserver {
 public:
  GestureInputRegistrar(
      content::RenderWidgetHost::MouseEventCallback mouse_event_callback,
      base::RepeatingCallback<void(const blink::WebInputEvent&,
                                   input::InputEventSource)>
          input_event_callback);

  GestureInputRegistrar(const GestureInputRegistrar&) = delete;
  GestureInputRegistrar& operator=(const GestureInputRegistrar&) = delete;

  ~GestureInputRegistrar() override;

  // Attaches to every widget reachable from `web_contents`'s current primary
  // page, unregistering any no longer reachable. Unregisters all if null.
  void UpdateObservedWidgets(content::WebContents* web_contents);

  // Unregisters `render_widget_host` specifically, e.g. from
  // `RenderViewDeleted`, which can't wait for a later resync.
  void UnregisterWidget(content::RenderWidgetHost& render_widget_host);
  void UnregisterAllWidgets();

  // Whether `render_widget_host` is currently observed, e.g. so
  // `RenderViewDeleted` can confirm it isn't already unregistered.
  bool IsObservingWidget(content::RenderWidgetHost& render_widget_host) const;

 private:
  // content::RenderWidgetHost::InputEventObserver:
  void OnInputEvent(const content::RenderWidgetHost& host,
                    const blink::WebInputEvent& event,
                    InputEventSource source) override;

  // content::RenderWidgetHostObserver:
  void RenderWidgetHostDestroyed(
      content::RenderWidgetHost* widget_host) override;

  // Bound per-widget so the transform below knows which widget's
  // coordinate space `event` is in.
  bool WidgetMouseEventCallback(content::RenderWidgetHost* render_widget_host,
                                const blink::WebMouseEvent& mouse_event);

  // Unregisters `render_widget_host`'s callback/observer. Doesn't touch
  // `observed_render_widget_hosts_`, callers remove their own entry there.
  void UnregisterFromWidget(content::RenderWidgetHost& render_widget_host);

  content::RenderWidgetHost::MouseEventCallback mouse_event_callback_;
  base::RepeatingCallback<void(const blink::WebInputEvent&,
                               input::InputEventSource)>
      input_event_callback_;

  std::vector<std::pair<raw_ptr<content::RenderWidgetHost>,
                        content::RenderWidgetHost::MouseEventCallback>>
      observed_render_widget_hosts_;
};

}  // namespace gestures

#endif  // BRAVE_BROWSER_UI_GESTURES_INPUT_GESTURE_INPUT_REGISTRAR_H_
