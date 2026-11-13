/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_HELP_BUBBLE_SHIELDS_HELP_BUBBLE_CONTROLLER_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_HELP_BUBBLE_SHIELDS_HELP_BUBBLE_CONTROLLER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"
#include "ui/base/unowned_user_data/scoped_unowned_user_data.h"

class BraveHelpBubbleHostView;
class BrowserWindowInterface;

namespace views {
class View;
}  // namespace views

// Shows the onboarding help bubble that points to the Shields icon, and hides
// it when another tab is activated.
class ShieldsHelpBubbleController : public TabStripModelObserver {
 public:
  DECLARE_USER_DATA(ShieldsHelpBubbleController);

  explicit ShieldsHelpBubbleController(BrowserWindowInterface& browser);
  ShieldsHelpBubbleController(const ShieldsHelpBubbleController&) = delete;
  ShieldsHelpBubbleController& operator=(const ShieldsHelpBubbleController&) =
      delete;
  ~ShieldsHelpBubbleController() override;

  // Returns the instance owned by `browser`, or nullptr.
  static ShieldsHelpBubbleController* From(BrowserWindowInterface* browser);

  // Shows `text` in a help bubble on the Shields icon. Returns false when the
  // bubble is not shown, e.g. while the icon is hidden.
  bool Show(const std::string& text);

  bool IsShowingForTesting() const;

 private:
  // The location bar's Shields icon or, in web app windows, the title bar's
  // Shields button.
  views::View* GetShieldsIcon();

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;

  const raw_ref<BrowserWindowInterface> browser_;
  raw_ptr<BraveHelpBubbleHostView> host_view_ = nullptr;
  ui::ScopedUnownedUserData<ShieldsHelpBubbleController>
      scoped_unowned_user_data_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_HELP_BUBBLE_SHIELDS_HELP_BUBBLE_CONTROLLER_H_
