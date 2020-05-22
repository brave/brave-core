// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_UI_DEVTOOLS_VIEWS_OVERLAY_AGENT_ANDROID_H_
#define COMPONENTS_UI_DEVTOOLS_VIEWS_OVERLAY_AGENT_ANDROID_H_

#include "components/ui_devtools/views/overlay_agent_views.h"

namespace ui_devtools {

class DOMAgent;

class OverlayAgentAndroid : public OverlayAgentViews {
 public:
  OverlayAgentAndroid(DOMAgent* dom_agent);
  ~OverlayAgentAndroid() override;

  int FindElementIdTargetedByPoint(ui::LocatedEvent* event) const override;
  static OverlayAgentAndroid* GetInstance() { return overlay_agent_android_; }

 private:
  void InstallPreTargetHandler() override;
  void RemovePreTargetHandler() override;

  FRIEND_TEST_ALL_PREFIXES(OverlayAgentTest, HighlightWindow);
  FRIEND_TEST_ALL_PREFIXES(OverlayAgentTest, HighlightEmptyOrInvisibleWindow);

  static OverlayAgentAndroid* overlay_agent_android_;

  DISALLOW_COPY_AND_ASSIGN(OverlayAgentAndroid);
};

}  // namespace ui_devtools

#endif  // COMPONENTS_UI_DEVTOOLS_VIEWS_OVERLAY_AGENT_AURA_H_
