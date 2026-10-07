// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/feature_list.h"
#include "build/build_config.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/core/events/mouse_event.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/html/canvas/html_canvas_element.h"
#include "third_party/blink/renderer/core/input/event_handler.h"
#include "third_party/blink/renderer/core/layout/hit_test_location.h"
#include "third_party/blink/renderer/core/layout/hit_test_request.h"
#include "third_party/blink/renderer/core/layout/hit_test_result.h"
#include "third_party/blink/renderer/core/layout/layout_box.h"
#include "third_party/blink/renderer/platform/geometry/physical_offset.h"

namespace blink {
namespace {

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
// Canvas apps such as games and 3D editors call preventDefault() on contextmenu
// to bind Shift + Right Click to their own input handling, so the override
// isn't wanted there. The canvas usually isn't the event target: apps overlay
// it with a transparent element that takes the input, so look at everything
// under the cursor rather than at the target's ancestors. Stop at opaque
// content, as ContextMenuController::GetContextMenuNodeWithImageContents()
// does when it penetrates to an image, so that a decorative canvas behind an
// ordinary page doesn't count.
// See https://github.com/brave/brave-browser/issues/56333.
bool IsOverCanvas(MouseEvent* event) {
  EventTarget* target = event->RawTarget();
  Node* target_node = target ? target->ToNode() : nullptr;
  LocalFrame* frame =
      target_node ? target_node->GetDocument().GetFrame() : nullptr;
  if (!frame) {
    return false;
  }

  HitTestLocation location(
      PhysicalOffset::FromPointFRound(event->AbsoluteLocation()));
  HitTestResult result = frame->GetEventHandler().HitTestResultAtLocation(
      location, HitTestRequest::kReadOnly | HitTestRequest::kPenetratingList |
                    HitTestRequest::kListBased);

  for (const auto& node : result.ListBasedTestResult()) {
    if (IsA<HTMLCanvasElement>(node.Get())) {
      return true;
    }
    // The hit test already places the cursor inside this box, so asking
    // whether the box is opaque across its whole area answers the question
    // without having to map the cursor into the box's coordinates. Erring
    // toward "not opaque" keeps looking for a canvas, which is the side that
    // leaves a page's own binding alone.
    const LayoutBox* box = node->GetLayoutBox();
    if (box &&
        box->BackgroundIsKnownToBeOpaqueInRect(box->PhysicalBorderBoxRect())) {
      return false;
    }
  }
  return false;
}
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)

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
    return context_mouse_event->shiftKey() &&
           !IsOverCanvas(context_mouse_event);
  }
  return false;
#else
  return false;
#endif
}

}  // namespace
}  // namespace blink

#include <third_party/blink/renderer/core/dom/events/event_dispatcher.cc>
