/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/renderer_host/render_widget_host_delegate.h"

#define PreHandleKeyboardEvent                                           \
  PreHandleKeyboardEvent_UnUsed() {                                      \
    return KeyboardEventProcessingResult::NOT_HANDLED;                   \
  }                                                                      \
                                                                         \
 public:                                                                 \
  bool prehandle_mouse_event_called() const {                            \
    return prehandle_mouse_event_called_;                                \
  }                                                                      \
  void set_prehandle_mouse_event(bool handle) {                          \
    prehandle_mouse_event_ = handle;                                     \
  }                                                                      \
                                                                         \
 protected:                                                              \
  bool PreHandleMouseEvent(const blink::WebMouseEvent& event) override { \
    prehandle_mouse_event_called_ = true;                                \
    if (prehandle_mouse_event_) {                                        \
      return true;                                                       \
    }                                                                    \
    return false;                                                        \
  }                                                                      \
  bool prehandle_mouse_event_ = false;                                   \
  bool prehandle_mouse_event_called_ = false;                            \
  KeyboardEventProcessingResult PreHandleKeyboardEvent

#include <content/browser/renderer_host/render_widget_host_unittest.cc>

#undef PreHandleKeyboardEvent

namespace content {

TEST_F(RenderWidgetHostTest, PreHandleMouseEvent) {
  // Simulate the situation that the browser handled the mouse event during
  // pre-handle phrase.
  delegate_->set_prehandle_mouse_event(true);

  // Simulate a mouse event.
  SimulateMouseEvent(WebMouseEvent::Type::kMouseDown);

  EXPECT_TRUE(delegate_->prehandle_mouse_event_called());

  // Make sure the mouse event is not sent to the renderer.
  MockWidgetInputHandler::MessageVector dispatched_events =
      host_->mock_render_input_router()->GetAndResetDispatchedMessages();
  EXPECT_EQ(0u, dispatched_events.size());

  // Simulate the situation that the browser didn't handle the mouse event
  // during pre-handle phrase.
  delegate_->set_prehandle_mouse_event(false);

  // Simulate a mouse event.
  SimulateMouseEvent(WebMouseEvent::Type::kMouseUp);

  // Make sure the mouse event is sent to the renderer.
  dispatched_events =
      host_->mock_render_input_router()->GetAndResetDispatchedMessages();
  ASSERT_EQ(1u, dispatched_events.size());
  ASSERT_TRUE(dispatched_events[0]->ToEvent());
  EXPECT_EQ(WebMouseEvent::Type::kMouseUp,
            dispatched_events[0]->ToEvent()->Event()->Event().GetType());
}

}  // namespace content
