/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/spacer_view.h"

#include "ui/gfx/geometry/size.h"
#include "ui/views/layout/flex_layout_types.h"
#include "ui/views/view.h"
#include "ui/views/view_class_properties.h"

namespace brave_ads {

views::View* CreateFlexibleSpacerView(int spacing) {
  views::View* view = new views::View;

  const gfx::Size size(spacing, 1);
  view->SetPreferredSize(size);

  const views::FlexSpecification flex_specification =
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kUnbounded);
  view->SetProperty(views::kFlexBehaviorKey, flex_specification);

  return view;
}

views::View* CreateFixedSizeSpacerView(int spacing) {
  views::View* view = new views::View;

  const gfx::Size size(spacing, 1);
  view->SetPreferredSize(size);

  const views::FlexSpecification flex_specification =
      views::FlexSpecification(views::MinimumFlexSizeRule::kPreferred,
                               views::MaximumFlexSizeRule::kPreferred);
  view->SetProperty(views::kFlexBehaviorKey, flex_specification);

  return view;
}

}  // namespace brave_ads
