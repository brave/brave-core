/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_VIEW_VIEWS_H_
#define BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_VIEW_VIEWS_H_

#include <memory>

#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"

class OmniboxEditController;
class OmniboxClient;
class LocationBarView;
namespace gfx {
class FontList;
}  // namespace gfx

class BraveOmniboxViewViews : public OmniboxViewViews {
 public:
  BraveOmniboxViewViews(OmniboxEditController* controller,
                        std::unique_ptr<OmniboxClient> client,
                        bool popup_window_mode,
                        LocationBarView* location_bar,
                        const gfx::FontList& font_list);

  BraveOmniboxViewViews(const BraveOmniboxViewViews&) = delete;
  BraveOmniboxViewViews& operator=(const BraveOmniboxViewViews&) = delete;
  ~BraveOmniboxViewViews() override;

  bool SelectedTextIsURL();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_VIEW_VIEWS_H_
