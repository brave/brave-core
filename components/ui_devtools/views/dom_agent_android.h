// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UI_DEVTOOLS_VIEWS_DOM_AGENT_ANDROID_H_
#define COMPONENTS_UI_DEVTOOLS_VIEWS_DOM_AGENT_ANDROID_H_

#include "components/ui_devtools/views/dom_agent_views.h"

namespace ui_devtools {

class DOMAgentAndroid : public DOMAgentViews {

 public:
  DOMAgentAndroid();

  ~DOMAgentAndroid() override;
  static DOMAgentAndroid* GetInstance() { return dom_agent_android_; }

  std::vector<UIElement*> CreateChildrenForRoot() override;

  std::unique_ptr<protocol::DOM::Node> BuildTreeForWindow(
      UIElement* window_element_root) override;

 private:
  static DOMAgentAndroid* dom_agent_android_;

  DISALLOW_COPY_AND_ASSIGN(DOMAgentAndroid);
};
}  // namespace ui_devtools

#endif  // COMPONENTS_UI_DEVTOOLS_VIEWS_DOM_AGENT_AURA_H_
