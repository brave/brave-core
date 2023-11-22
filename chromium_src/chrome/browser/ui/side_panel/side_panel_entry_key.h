/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_SIDE_PANEL_SIDE_PANEL_ENTRY_KEY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_SIDE_PANEL_SIDE_PANEL_ENTRY_KEY_H_

#include "brave/components/sidebar/mobile_view_id.h"
#include "chrome/browser/ui/side_panel/side_panel_entry_id.h"

#define SidePanelEntryKey SidePanelEntryKey_Chromium

#include "src/chrome/browser/ui/side_panel/side_panel_entry_key.h"  // // IWYU pragma: export

#undef SidePanelEntryKey

class SidePanelEntryKey : public SidePanelEntryKey_Chromium {
 public:
  explicit SidePanelEntryKey(SidePanelEntryId id);
  SidePanelEntryKey(SidePanelEntryId id, extensions::ExtensionId extension_id);
  SidePanelEntryKey(SidePanelEntryId id, sidebar::MobileViewId mobile_view_id);
  ~SidePanelEntryKey() override;

  SidePanelEntryKey(const SidePanelEntryKey& other);
  SidePanelEntryKey& operator=(const SidePanelEntryKey& other);

  bool operator==(const SidePanelEntryKey& other) const;
  bool operator<(const SidePanelEntryKey& other) const;

  std::optional<sidebar::MobileViewId> mobile_view_id() const {
    return mobile_view_id_;
  }

 private:
  std::optional<sidebar::MobileViewId> mobile_view_id_;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_SIDE_PANEL_SIDE_PANEL_ENTRY_KEY_H_
