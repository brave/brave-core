/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_VIEW_VIEWS_H_
#define BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_VIEW_VIEWS_H_

#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"

class BraveOmniboxViewViews : public OmniboxViewViews {
 public:
  using OmniboxViewViews::OmniboxViewViews;
  ~BraveOmniboxViewViews() override;

  BraveOmniboxViewViews(const BraveOmniboxViewViews&) = delete;
  BraveOmniboxViewViews& operator=(const BraveOmniboxViewViews&) = delete;

  // OmniboxViewViews overrides:
  void OnTemplateURLServiceChanged() override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_VIEW_VIEWS_H_
