// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_context_menu_util.h"

#include <utility>

#include "brave/browser/ui/gestures/gesture_controller.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"

namespace gestures {

bool ConsumeSuppressContextMenuForWebContents(
    content::WebContents* web_contents,
    base::OnceClosure callback) {
  tabs::TabInterface* const tab =
      tabs::TabInterface::MaybeGetFromContents(web_contents);
  GestureController* const gesture_controller = GestureController::From(tab);
  if (!gesture_controller) {
    return false;
  }
  if (gesture_controller->ConsumeSuppressNextContextMenu()) {
    return true;
  }
  if (gesture_controller->IsDrawingGesture()) {
    // Deferred since some platforms show the context menu synchronously on
    // button-down, before the hold's outcome is even known.
    gesture_controller->DeferContextMenuShow(std::move(callback));
    return true;
  }
  return false;
}

}  // namespace gestures
