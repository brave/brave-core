/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_LOCATION_BAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_LOCATION_BAR_BRAVE_LOCATION_BAR_VIEW_H_

#include "chrome/browser/ui/views/location_bar/location_bar_view.h"

class BraveActionsContainer;
enum class OmniboxTint;

// The purposes of this subclass are to:
// - Add the BraveActionsContainer to the location bar
class BraveLocationBarView : public LocationBarView {
  public:
    using LocationBarView::LocationBarView;
    void Init() override;
    void Layout() override;
    void Update(const content::WebContents* contents) override;
    void OnChanged() override;
    gfx::Size CalculatePreferredSize() const override;
    void OnThemeChanged() override;

  private:
    void UpdateBookmarkStarVisibility() override;
    OmniboxTint GetTint() override;
    BraveActionsContainer* brave_actions_ = nullptr;

    DISALLOW_COPY_AND_ASSIGN(BraveLocationBarView);
};

#endif
