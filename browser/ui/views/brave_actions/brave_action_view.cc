/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_actions/brave_action_view.h"

#include <memory>

#include "brave/browser/ui/brave_actions/constants.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/chrome_layout_provider.h"
#include "chrome/browser/ui/views/toolbar/toolbar_action_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_ink_drop_util.h"
#include "third_party/skia/include/core/SkPath.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/views/controls/button/menu_button.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"

void BraveActionView::OnBoundsChanged(const gfx::Rect& previous_bounds) {
  // Set the highlight path for the toolbar button,
  // making it inset so that the badge can show outside it in the
  // fake margin on the right that we are creating.
  gfx::Insets highlight_insets(0, 0, 0, brave_actions::kBraveActionRightMargin);
  gfx::Rect rect(size());
  rect.Inset(highlight_insets);
  const int radii = ChromeLayoutProvider::Get()->GetCornerRadiusMetric(
      views::EMPHASIS_MAXIMUM, rect.size());

  auto path = std::make_unique<SkPath>();
  path->addRoundRect(gfx::RectToSkRect(rect), radii, radii);
  SetProperty(views::kHighlightPathKey, path.release());

  MenuButton::OnBoundsChanged(previous_bounds);
}
