/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_VIEW_VIEWS_H_
#define BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_VIEW_VIEWS_H_

#include "chrome/browser/ui/views/omnibox/omnibox_view_views.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class GURL;

class BraveOmniboxViewViews : public OmniboxViewViews {
 public:
  using OmniboxViewViews::OmniboxViewViews;

  BraveOmniboxViewViews(const BraveOmniboxViewViews&) = delete;
  BraveOmniboxViewViews& operator=(const BraveOmniboxViewViews&) = delete;
  ~BraveOmniboxViewViews() override;

  bool SelectedTextIsURL();
  void CleanAndCopySelectedURL();

 protected:
  absl::optional<GURL> GetURLToCopy();
  void CopySanitizedURL(const GURL& url);
#if BUILDFLAG(IS_WIN)
  // View overrides:
  bool AcceleratorPressed(const ui::Accelerator& accelerator) override;
  bool GetAcceleratorForCommandId(int command_id,
                                  ui::Accelerator* accelerator) const override;
#endif  // BUILDFLAG(IS_WIN)
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
  // ui::views::Textfield
  void ExecuteTextEditCommand(ui::TextEditCommand command) override;
#endif
  // ui::views::TextfieldController:
  void UpdateContextMenu(ui::SimpleMenuModel* menu_contents) override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_OMNIBOX_BRAVE_OMNIBOX_VIEW_VIEWS_H_
