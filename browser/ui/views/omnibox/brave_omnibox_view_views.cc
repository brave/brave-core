/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/omnibox/brave_omnibox_view_views.h"
#include "ui/views/widget/widget.h"

BraveOmniboxViewViews::~BraveOmniboxViewViews() = default;

void BraveOmniboxViewViews::OnTemplateURLServiceChanged() {
  // Only update active window's omnibox placeholder text when search provider
  // is changed.
  // See ActiveWindowSearchProviderManager comment for more details.
  if (GetWidget()->IsActive())
    OmniboxViewViews::OnTemplateURLServiceChanged();
}
