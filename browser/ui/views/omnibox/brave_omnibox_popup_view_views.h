/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_POPUP_VIEW_VIEWS_H_
#define BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_POPUP_VIEW_VIEWS_H_

#include "chrome/browser/ui/views/omnibox/omnibox_popup_view_views.h"

class BraveOmniboxPopupViewViews : public OmniboxPopupViewViews {
  METADATA_HEADER(BraveOmniboxPopupViewViews, OmniboxPopupViewViews)
 public:

  using OmniboxPopupViewViews::OmniboxPopupViewViews;
  ~BraveOmniboxPopupViewViews() override;

  int GetLocationBarViewWidth() const;

  // OmniboxPopupViewViews:
  gfx::Rect GetTargetBounds() const override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_POPUP_VIEW_VIEWS_H_
