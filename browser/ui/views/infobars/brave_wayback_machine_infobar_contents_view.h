/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_CONTENTS_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_CONTENTS_VIEW_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_wayback_machine/wayback_machine_url_fetcher.h"
#include "ui/views/view.h"

namespace content {
class WebContents;
}  // namespace content

namespace views {
class ImageView;
class Label;
}  // namespace views

class BraveWaybackMachineInfoBarButtonContainer;
class GURL;
class PrefService;

// Includes all view controls except close button that managed by InfoBarView.
class BraveWaybackMachineInfoBarContentsView
    : public views::View,
      public WaybackMachineURLFetcher::Client {
 public:
  explicit BraveWaybackMachineInfoBarContentsView(
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

  // WaybackMachineURLFetcher::Client overrides:
  void OnWaybackURLFetched(const GURL& latest_wayback_url) override;

  void InitializeChildren();
  views::Label* CreateLabel(const std::u16string& text);
  SkColor GetColor(int id) const;
  void UpdateChildrenVisibility(bool show_before_checking_views);
  void FetchWaybackURL();
  void LoadURL(const GURL& url);
  void HideInfobar();

  void FetchURLButtonPressed();
  void DontAskButtonPressed();

  // Used for labels theme changing all together.
  Labels labels_;
  Views views_visible_before_checking_;
  Views views_visible_after_checking_;
  raw_ptr<content::WebContents> contents_ = nullptr;
  WaybackMachineURLFetcher wayback_machine_url_fetcher_;

  BraveWaybackMachineInfoBarButtonContainer* fetch_url_button_ = nullptr;
  views::View* dont_ask_button_ = nullptr;
  PrefService* pref_service_ = nullptr;
  views::ImageView* wayback_spot_graphic_ = nullptr;
  bool wayback_url_fetch_requested_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_BRAVE_WAYBACK_MACHINE_INFOBAR_CONTENTS_VIEW_H_
