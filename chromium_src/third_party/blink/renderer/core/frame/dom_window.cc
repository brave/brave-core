/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/renderer/core/frame/dom_window.h"

#include "src/third_party/blink/renderer/core/frame/dom_window.cc"

namespace blink {

LocalFrame* DOMWindow::GetDisconnectedFrame() const {
  // IncumbentDOMWindow is safe to call only when an active v8 context is
  // present.
  if (auto* isolate = v8::Isolate::TryGetCurrent();
      !isolate || !isolate->InContext()) {
    return nullptr;
  }

  v8::Isolate* isolate = window_proxy_manager_->GetIsolate();
  LocalDOMWindow* accessing_window = IncumbentDOMWindow(isolate);
  LocalFrame* accessing_frame = accessing_window->GetFrame();
  return accessing_frame;
}

}  // namespace blink
