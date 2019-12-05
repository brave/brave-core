/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_CONTENTS_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_CONTENTS_VIEW_H_

#include <vector>

#include "brave/browser/brave_wayback_machine/wayback_machine_url_fetcher.h"
#include "ui/views/controls/button/button.h"

namespace content {
class WebContents;
}  // namespace content

namespace infobars {
class InfoBar;
}  // namespace infobars

namespace views {
class Label;
class Separator;
}  // namespace views

class GURL;

// Includes all view controls except close button that managed by InfoBarView.
class BraveWaybackMachineInfoBarContentsView
    : public views::View,
      public views::ButtonListener,
      public WaybackMachineURLFetcher::Client {
 public:
  BraveWaybackMachineInfoBarContentsView(
      infobars::InfoBar* infobar,
      content::WebContents* contents);
  ~BraveWaybackMachineInfoBarContentsView() override;

  BraveWaybackMachineInfoBarContentsView(
      const BraveWaybackMachineInfoBarContentsView&) = delete;
  BraveWaybackMachineInfoBarContentsView& operator=(
      const BraveWaybackMachineInfoBarContentsView&) = delete;

 private:
  using Labels = std::vector<views::Label*>;
  using Views = std::vector<views::View*>;

  // views::View overrides:
  void OnThemeChanged() override;
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  // WaybackMachineURLFetcher::Client overrides:
  void OnWaybackURLFetched(const GURL& latest_wayback_url) override;

  void InitializeChildren();
  views::Label* CreateLabel(const base::string16& text);
  SkColor GetColor(int id) const;
  void UpdateChildrenVisibility(bool show_before_checking_views);
  void FetchWaybackURL();
  void LoadURL(const GURL& url);

  // Used for labels theme changing all together.
  Labels labels_;
  views::Separator* separator_;

  Views views_visible_before_checking_;
  Views views_visible_after_checking_;

  infobars::InfoBar* infobar_;
  content::WebContents* contents_;
  WaybackMachineURLFetcher wayback_machine_url_fetcher_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_CONTENTS_VIEW_H_
