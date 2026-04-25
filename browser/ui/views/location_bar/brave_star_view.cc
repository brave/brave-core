/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/location_bar/brave_star_view.h"

#include "ui/base/metadata/metadata_impl_macros.h"

void BraveStarView::UpdateImpl() {
  SetVisible(false);
}

void BraveStarView::OnBubbleWidgetChanged(views::Widget* widget) {}

BEGIN_METADATA(BraveStarView)
END_METADATA
