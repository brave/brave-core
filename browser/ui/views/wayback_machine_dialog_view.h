/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_WAYBACK_MACHINE_DIALOG_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_WAYBACK_MACHINE_DIALOG_VIEW_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_wayback_machine/wayback_machine_url_fetcher.h"
#include "ui/base/metadata/metadata_header_macros.h"
#include "ui/views/window/dialog_delegate.h"

namespace content {
class WebContents;
}  // namespace content

namespace views {
class Checkbox;
}  // namespace views

class WaybackMachineFetchButton;
class PrefService;

class WaybackMachineDialogView : public views::DialogDelegateView,
                                 public WaybackMachineURLFetcher::Client {
  METADATA_HEADER(WaybackMachineDialogView, views::DialogDelegateView)
 public:
  explicit WaybackMachineDialogView(content::WebContents* web_contents);
  ~WaybackMachineDialogView() override;

 private:
  // WaybackMachineURLFetcher::Client overrides:
  void OnWaybackURLFetched(const GURL& latest_wayback_url) override;

  views::Label* CreateLabel(const std::u16string& text);
  void UpdateChildrenVisibility(bool show_before_checking_views);
  void OnCheckboxUpdated();
  void OnFetchURLButtonPressed();
  void FetchWaybackURL();
  void LoadURL(const GURL& url);
  void UpdateDialogForWaybackNotAvailable();
  void OnWillCloseDialog();
  void OnCancel();

  views::View::Views views_visible_before_checking_;
  views::View::Views views_visible_after_checking_;
  raw_ptr<views::Checkbox> dont_ask_again_ = nullptr;
  raw_ptr<views::View> no_thanks_ = nullptr;
  raw_ptr<WaybackMachineFetchButton> fetch_url_button_ = nullptr;
  raw_ptr<content::WebContents> web_contents_ = nullptr;
  WaybackMachineURLFetcher wayback_machine_url_fetcher_;
  const raw_ptr<PrefService> pref_service_ = nullptr;
  bool wayback_url_fetch_requested_ = false;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_WAYBACK_MACHINE_DIALOG_VIEW_H_
