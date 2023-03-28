/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_icon.h"

#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/brave_tab.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/views/tabs/fake_tab_slot_controller.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/gfx/favicon_size.h"
#include "ui/gfx/scoped_canvas.h"

BraveTabIcon::BraveTabIcon(Tab* tab) : tab_(tab) {
  CHECK(base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
      << "This class is used only for vertical tab strip";
}

void BraveTabIcon::OnPaint(gfx::Canvas* canvas) {
  CHECK(tab_->controller()->GetBrowser());
  if (!tabs::utils::ShouldShowVerticalTabs(tab_->controller()->GetBrowser())) {
    TabIcon::OnPaint(canvas);
    return;
  }

  if (!tab_->data().pinned) {
    TabIcon::OnPaint(canvas);
    return;
  }

  // Enlarge tab icon if it's pinned tab in vertical tab strip.
  const auto center_point = GetLocalBounds().CenterPoint();
  constexpr auto kFaviconSizeForPinnedTab = 18.f;
  const auto scale = kFaviconSizeForPinnedTab / gfx::kFaviconSize;

  gfx::ScopedCanvas scoped_canvas(canvas);
  // Scale to desired size
  canvas->Translate({center_point.x(), center_point.y()});
  canvas->Scale(scale, scale);

  // Set back canvas to the lef top and adjust insets by the enlarged size.
  const auto delta =
      static_cast<int>(kFaviconSizeForPinnedTab - gfx::kFaviconSize);
  canvas->Translate({-center_point.x() + delta, -center_point.y() + delta});

  TabIcon::OnPaint(canvas);
}

BEGIN_METADATA(BraveTabIcon, TabIcon)
END_METADATA
