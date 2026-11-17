/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_ANIMATIONS_WEB_PANEL_ANIMATIONS_H_
#define BRAVE_BROWSER_UI_VIEWS_ANIMATIONS_WEB_PANEL_ANIMATIONS_H_

#include "chrome/browser/ui/animation/browser_animation_provider.h"
#include "chrome/browser/ui/animation/browser_animation_types.h"
#include "ui/base/identifier/unique_identifier.h"
#include "ui/base/interaction/safe_castable.h"

// Declares the open/close motion for the sidebar web panel hosted in
// BraveMultiContentsView. `kPanelWidth` runs 0.0 -> 1.0 as a fraction of the
// panel's target width; the layout multiplies the reserved strip by it.
//
// Deliberately a separate group from SidePanelAnimations::kSidePanel even
// though the two panels share the same space: the persisted `kPanelWidth`
// value would otherwise collide between them.
class WebPanelAnimations : public CachingBrowserAnimationProvider {
 public:
  DECLARE_SAFE_CAST_TARGET()

  WebPanelAnimations();
  ~WebPanelAnimations() override;

  DECLARE_CLASS_BROWSER_ANIMATION_GROUP(kWebPanel);
  DECLARE_CLASS_BROWSER_ANIMATION_MOTION(kOpen);
  DECLARE_CLASS_BROWSER_ANIMATION_MOTION(kClose);
  DECLARE_CLASS_BROWSER_ANIMATION_SEQUENCE(kPanelWidth);

  // CachingBrowserAnimationProvider:
  GroupInfos GenerateAnimations() const override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_ANIMATIONS_WEB_PANEL_ANIMATIONS_H_
