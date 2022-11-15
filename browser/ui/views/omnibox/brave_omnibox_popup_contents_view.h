/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_POPUP_CONTENTS_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_POPUP_CONTENTS_VIEW_H_

#include "chrome/browser/ui/views/omnibox/omnibox_popup_contents_view.h"

class BraveOmniboxPopupContentsView : public OmniboxPopupContentsView {
 public:
  METADATA_HEADER(BraveOmniboxPopupContentsView);

  using OmniboxPopupContentsView::OmniboxPopupContentsView;
  ~BraveOmniboxPopupContentsView() override;

  // OmniboxPopupContentsView:
  gfx::Rect GetTargetBounds() const override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_POPUP_CONTENTS_VIEW_H_
