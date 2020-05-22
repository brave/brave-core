// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/components/ui_devtools/views/overlay_agent_android.h"

#include "components/ui_devtools/dom_agent.h"
#include "components/ui_devtools/views/window_element.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"
#include "ui/wm/core/window_util.h"

namespace ui_devtools {

OverlayAgentAndroid* OverlayAgentAndroid::overlay_agent_android_ = nullptr;

OverlayAgentAndroid::OverlayAgentAndroid(DOMAgent* dom_agent)
    : OverlayAgentViews(dom_agent) {
  DCHECK(!overlay_agent_android_);
  overlay_agent_android_ = this;
}

OverlayAgentAndroid::~OverlayAgentAndroid() {
  overlay_agent_android_ = nullptr;
}

void OverlayAgentAndroid::InstallPreTargetHandler() {
}

void OverlayAgentAndroid::RemovePreTargetHandler() {
}

int OverlayAgentAndroid::FindElementIdTargetedByPoint(
    ui::LocatedEvent* event) const {
  return 0;
}

// static
std::unique_ptr<OverlayAgentViews> OverlayAgentViews::Create(
    DOMAgent* dom_agent) {
  return std::make_unique<OverlayAgentAndroid>(dom_agent);
}

}  // namespace ui_devtools
