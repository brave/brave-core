// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/feature_list.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/event_type_names.h"
#include "third_party/blink/renderer/core/events/mouse_event.h"

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
