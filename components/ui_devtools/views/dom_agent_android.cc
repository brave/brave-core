// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/components/ui_devtools/views/dom_agent_android.h"

#include "base/stl_util.h"
#include "components/ui_devtools/views/widget_element.h"
#include "components/ui_devtools/views/window_element.h"

namespace ui_devtools {

namespace {
using ui_devtools::protocol::Array;
using ui_devtools::protocol::DOM::Node;
}  // namespace

DOMAgentAndroid* DOMAgentAndroid::dom_agent_android_ = nullptr;

DOMAgentAndroid::DOMAgentAndroid() {
  DCHECK(!dom_agent_android_);
  dom_agent_android_ = this;
}

DOMAgentAndroid::~DOMAgentAndroid() {
  dom_agent_android_ = nullptr;
}

std::vector<UIElement*> DOMAgentAndroid::CreateChildrenForRoot() {
  std::vector<UIElement*> children;
  return children;
}

std::unique_ptr<Node> DOMAgentAndroid::BuildTreeForWindow(
    UIElement* window_element_root) {
  return nullptr;
}

// static
std::unique_ptr<DOMAgentViews> DOMAgentViews::Create() {
  return std::make_unique<DOMAgentAndroid>();
}

}  // namespace ui_devtools
