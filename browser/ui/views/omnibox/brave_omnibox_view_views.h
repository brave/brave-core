/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_VIEW_VIEWS_H_
#define BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_VIEW_VIEWS_H_

#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"

class BraveOmniboxViewViews : public OmniboxViewViews {
 public:
  using OmniboxViewViews::OmniboxViewViews;

  BraveOmniboxViewViews(const BraveOmniboxViewViews&) = delete;
  BraveOmniboxViewViews& operator=(const BraveOmniboxViewViews&) = delete;
  ~BraveOmniboxViewViews() override;

  // views::Textfield:
  void ExecuteCommand(int command_id, int event_flags) override;

  // views::TextfieldController:
  bool SelectedTextIsURL() override;
  bool IsCleanLinkCommand(int command_id) const override;
  void OnSanitizedCopy(ui::ClipboardBuffer clipboard_buffer) override;
  void UpdateContextMenu(ui::SimpleMenuModel* menu_contents) override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_VIEW_VIEWS_H_
