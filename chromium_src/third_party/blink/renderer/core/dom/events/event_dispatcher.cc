// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/feature_list.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/core/events/mouse_event.h"
#include "third_party/blink/renderer/core/html/canvas/html_canvas_element.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace blink {
namespace {

// Function to determine if contextmenu event should bypass preventDefault so
// that users can always open context menu by holding Shift key. This method is
// called inside EventDispatcher::DispatchEventPostProcess, so that
// Node::DefaultEventHandler() can be called.
bool ShouldBypassDefaultPreventedForContextMenu(Event* event) {
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
  if (!base::FeatureList::IsEnabled(
          blink::features::kForceContextMenuOnShiftRightClick)) {
    return false;
  }

  if (event->type() != event_type_names::kContextmenu) {
    return false;
  }
  if (auto* context_mouse_event = DynamicTo<MouseEvent>(event)) {
    // Don't force context menu when the target element is a <canvas> - pages
    // (e.g. games, drawing apps) legitimately suppress it on self-drawn
    // surfaces. <embed>/<object> (e.g. the PDF plugin) could be added to this
    // exception in the future if unwanted menu overrides are reported there -
    // intentionally excluded for now.
    if (auto* target = event->target() ? event->target()->ToNode() : nullptr) {
      if (IsA<HTMLCanvasElement>(target)) {
        return false;
      }
    }
    return context_mouse_event->shiftKey();
  }
  return false;
#else
  return false;
#endif
}

}  // namespace
}  // namespace blink

#include <third_party/blink/renderer/core/dom/events/event_dispatcher.cc>
